/*Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <config.h>
#include "qrcode.h"
#include "error.h"
#include "isaac.h"
#include "rs.h"
#include "qrdec.h"
#include "svg.h"

typedef int qr_line[3];

typedef struct qr_finder_cluster qr_finder_cluster;
typedef struct qr_finder_edge_pt  qr_finder_edge_pt;
typedef struct qr_finder_center   qr_finder_center;

typedef struct qr_aff qr_aff;
typedef struct qr_hom qr_hom;

typedef struct qr_finder qr_finder;

typedef struct qr_hom_cell      qr_hom_cell;
typedef struct qr_sampling_grid qr_sampling_grid;
typedef struct qr_pack_buf      qr_pack_buf;


/*A cluster of lines crossing a finder pattern (all in the same direction).*/
struct qr_finder_cluster {
    /*Pointers to the lines crossing the pattern.*/
    qr_finder_line** lines;
    /*The number of lines in the cluster.*/
    int              nlines;
};


/*A point on the edge of a finder pattern.
  These are obtained from the endpoints of the lines crossing this particular
   pattern.*/
struct qr_finder_edge_pt {
    /*The location of the edge point.*/
    qr_point pos;
    /*A label classifying which edge this belongs to:
      0: negative u edge (left)
      1: positive u edge (right)
      2: negative v edge (top)
      3: positive v edge (bottom)*/
    int      edge;
    /*The (signed) perpendicular distance of the edge point from a line parallel
       to the edge passing through the finder center, in (u,v) coordinates.
      This is also re-used by RANSAC to store inlier flags.*/
    int      extent;
};


/*The center of a finder pattern obtained from the crossing of one or more
   clusters of horizontal finder lines with one or more clusters of vertical
   finder lines.*/
struct qr_finder_center {
    /*The estimated location of the finder center.*/
    qr_point           pos;
    /*The list of edge points from the crossing lines.*/
    qr_finder_edge_pt* edge_pts;
    /*The number of edge points from the crossing lines.*/
    int                nedge_pts;
};


/* collection of finder lines */
typedef struct qr_finder_lines {
    qr_finder_line* lines;
    int nlines, clines;
} qr_finder_lines;


struct qr_reader {
    /*The GF(256) representation used in Reed-Solomon decoding.*/
     rs_gf256  gf;
    /*The random number generator used by RANSAC.*/
    isaac_ctx isaac;
    /* current finder state, horizontal and vertical lines */
    qr_finder_lines finder_lines[2];
};


/*Frees a client reader handle.*/
void _zbar_qr_destroy(qr_reader* reader)
{
    
    zprintf(1, "max finder lines = %dx%d\n",
        reader->finder_lines[0].clines,
        reader->finder_lines[1].clines);
    if (reader->finder_lines[0].lines)
        free(reader->finder_lines[0].lines);
    if (reader->finder_lines[1].lines)
        free(reader->finder_lines[1].lines);
    
    free(reader);
    
}

int _zbar_qr_found_line(qr_reader* reader,
    int dir,
    const qr_finder_line* line)
{
    /* minimally intrusive brute force version */
    qr_finder_lines* lines = &reader->finder_lines[dir];

    if (lines->nlines >= lines->clines) {
        lines->clines *= 2;
        lines->lines = realloc(lines->lines,
            ++lines->clines * sizeof(*lines->lines));
    }

    memcpy(lines->lines + lines->nlines++, line, sizeof(*line));

    return(0);
}

/*Initializes a client reader handle.*/
static void qr_reader_init(qr_reader* reader)
{
    /*time_t now;
      now=time(NULL);
      isaac_init(&_reader->isaac,&now,sizeof(now));*/
      isaac_init(&reader->isaac, NULL, 0);
      rs_gf256_init(&reader->gf, QR_PPOLY);
}

/*Locates a set of putative finder centers in the image.
  First we search for horizontal and vertical lines that have
   (dark:light:dark:light:dark) runs with size ratios of roughly (1:1:3:1:1).
  Then we cluster them into groups such that each subsequent pair of endpoints
   is close to the line before it in the cluster.
  This will locate many line clusters that don't cross a finder pattern, but
   qr_finder_find_crossings() will filter most of them out.
  Where horizontal and vertical clusters cross, a prospective finder center is
   returned.
  _centers:  Returns a pointer to a freshly-allocated list of finder centers.
             This must be freed by the caller.
  _edge_pts: Returns a pointer to a freshly-allocated list of edge points
              around those centers.
             This must be freed by the caller.
  _img:      The binary image to search.
  _width:    The width of the image.
  _height:   The height of the image.
  Return: The number of putative finder centers located.*/
static int qr_finder_centers_locate(qr_finder_center** _centers,
    qr_finder_edge_pt** _edge_pts, qr_reader* reader,
    int _width, int _height) {
    qr_finder_line* hlines = reader->finder_lines[0].lines;
    int                 nhlines = reader->finder_lines[0].nlines;
    qr_finder_line* vlines = reader->finder_lines[1].lines;
    int                 nvlines = reader->finder_lines[1].nlines;

    qr_finder_line** hneighbors;
    qr_finder_cluster* hclusters;
    int                 nhclusters;
    qr_finder_line** vneighbors;
    qr_finder_cluster* vclusters;
    int                 nvclusters;
    int                 ncenters;

    /*Cluster the detected lines.*/
    hneighbors = (qr_finder_line**)malloc(nhlines * sizeof(*hneighbors));
    /*We require more than one line per cluster, so there are at most nhlines/2.*/
    hclusters = (qr_finder_cluster*)malloc((nhlines >> 1) * sizeof(*hclusters));
 //   nhclusters = qr_finder_cluster_lines(hclusters, hneighbors, hlines, nhlines, 0);
    /*We need vertical lines to be sorted by X coordinate, with ties broken by Y
       coordinate, for clustering purposes.
      We scan the image in the opposite order for cache efficiency, so sort the
       lines we found here.*/
 //   qsort(vlines, nvlines, sizeof(*vlines), qr_finder_vline_cmp);
    vneighbors = (qr_finder_line**)malloc(nvlines * sizeof(*vneighbors));
    /*We require more than one line per cluster, so there are at most nvlines/2.*/
    vclusters = (qr_finder_cluster*)malloc((nvlines >> 1) * sizeof(*vclusters));
 //   nvclusters = qr_finder_cluster_lines(vclusters, vneighbors, vlines, nvlines, 1);
    /*Find line crossings among the clusters.*/
    if (nhclusters >= 3 && nvclusters >= 3) {
        qr_finder_edge_pt* edge_pts;
        qr_finder_center* centers;
        int                 nedge_pts;
        int                 i;
        nedge_pts = 0;
        for (i = 0; i < nhclusters; i++)nedge_pts += hclusters[i].nlines;
        for (i = 0; i < nvclusters; i++)nedge_pts += vclusters[i].nlines;
        nedge_pts <<= 1;
        edge_pts = (qr_finder_edge_pt*)malloc(nedge_pts * sizeof(*edge_pts));
//        centers = (qr_finder_center*)malloc(
//            QR_MINI(nhclusters, nvclusters) * sizeof(*centers));
 //       ncenters = qr_finder_find_crossings(centers, edge_pts,
 //           hclusters, nhclusters, vclusters, nvclusters);
        *_centers = centers;
        *_edge_pts = edge_pts;
    }
    else ncenters = 0;
    free(vclusters);
    free(vneighbors);
    free(hclusters);
    free(hneighbors);
    return ncenters;
}


/*Allocates a client reader handle.*/
qr_reader* _zbar_qr_create(void)
{
    qr_reader* reader = (qr_reader*)calloc(1, sizeof(*reader));
    qr_reader_init(reader);
    return(reader);
}

/* reset finder state between scans */
void _zbar_qr_reset(qr_reader* reader)
{
    reader->finder_lines[0].nlines = 0;
    reader->finder_lines[1].nlines = 0;
}

int _zbar_qr_decode(qr_reader* reader,
    zbar_image_scanner_t* iscn,
    zbar_image_t* img)
{
      int nqrdata = 0, ncenters;
      qr_finder_edge_pt* edge_pts = NULL;
      qr_finder_center* centers = NULL;

      if (reader->finder_lines[0].nlines < 9 ||
          reader->finder_lines[1].nlines < 9)
          return(0);

      svg_group_start("finder", 0, 1. / (1 << QR_FINDER_SUBPREC), 0, 0, 0);
/*
      ncenters = qr_finder_centers_locate(&centers, &edge_pts, reader, 0, 0);

      zprintf(14, "%dx%d finders, %d centers:\n",
          reader->finder_lines[0].nlines,
          reader->finder_lines[1].nlines,
          ncenters);
      qr_svg_centers(centers, ncenters);

      if (ncenters >= 3) {
          void* bin = qr_binarize(img->data, img->width, img->height);

          qr_code_data_list qrlist;
          qr_code_data_list_init(&qrlist);

          qr_reader_match_centers(reader, &qrlist, centers, ncenters,
              bin, img->width, img->height);

          if (qrlist.nqrdata > 0)
              nqrdata = qr_code_data_list_extract_text(&qrlist, iscn, img);

          qr_code_data_list_clear(&qrlist);
          free(bin);
      }
      svg_group_end();

      if (centers)
          free(centers);
      if (edge_pts)
          free(edge_pts);
      return(nqrdata); */
}