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
#include "util.h"
#include "binarize.h"
#include "image.h"


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



/*Clusters adjacent lines into groups that are large enough to be crossing a
   finder pattern (relative to their length).
  _clusters:  The buffer in which to store the clusters found.
  _neighbors: The buffer used to store the lists of lines in each cluster.
  _lines:     The list of lines to cluster.
              Horizontal lines must be sorted in ascending order by Y
               coordinate, with ties broken by X coordinate.
              Vertical lines must be sorted in ascending order by X coordinate,
               with ties broken by Y coordinate.
  _nlines:    The number of lines in the set of lines to cluster.
  _v:         0 for horizontal lines, or 1 for vertical lines.
  Return: The number of clusters.*/
static int qr_finder_cluster_lines(qr_finder_cluster* _clusters,
    qr_finder_line** _neighbors, qr_finder_line* _lines, int _nlines, int _v) {
    unsigned char* mark;
    qr_finder_line** neighbors;
    int              nneighbors;
    int              nclusters;
    int              i;
    /*TODO: Kalman filters!*/
    mark = (unsigned char*)calloc(_nlines, sizeof(*mark));
    neighbors = _neighbors;
    nclusters = 0;
    for (i = 0; i < _nlines - 1; i++)if (!mark[i]) {
        int len;
        int j;
        nneighbors = 1;
        neighbors[0] = _lines + i;
        len = _lines[i].len;
        for (j = i + 1; j < _nlines; j++)if (!mark[j]) {
            const qr_finder_line* a;
            const qr_finder_line* b;
            int                   thresh;
            a = neighbors[nneighbors - 1];
            b = _lines + j;
            /*The clustering threshold is proportional to the size of the lines,
               since minor noise in large areas can interrupt patterns more easily
               at high resolutions.*/
            thresh = a->len + 7 >> 2;
            if (abs(a->pos[1 - _v] - b->pos[1 - _v]) > thresh)break;
            if (abs(a->pos[_v] - b->pos[_v]) > thresh)continue;
            if (abs(a->pos[_v] + a->len - b->pos[_v] - b->len) > thresh)continue;
            if (a->boffs > 0 && b->boffs > 0 &&
                abs(a->pos[_v] - a->boffs - b->pos[_v] + b->boffs) > thresh) {
                continue;
            }
            if (a->eoffs > 0 && b->eoffs > 0 &&
                abs(a->pos[_v] + a->len + a->eoffs - b->pos[_v] - b->len - b->eoffs) > thresh) {
                continue;
            }
            neighbors[nneighbors++] = _lines + j;
            len += b->len;
        }
        /*We require at least three lines to form a cluster, which eliminates a
           large number of false positives, saving considerable decoding time.
          This should still be sufficient for 1-pixel codes with no noise.*/
        if (nneighbors < 3)continue;
        /*The expected number of lines crossing a finder pattern is equal to their
           average length.
          We accept the cluster if size is at least 1/3 their average length (this
           is a very small threshold, but was needed for some test images).*/
        len = ((len << 1) + nneighbors) / (nneighbors << 1);
        if (nneighbors * (5 << QR_FINDER_SUBPREC) >= len) {
            _clusters[nclusters].lines = neighbors;
            _clusters[nclusters].nlines = nneighbors;
            for (j = 0; j < nneighbors; j++)mark[neighbors[j] - _lines] = 1;
            neighbors += nneighbors;
            nclusters++;
        }
    }
    free(mark);
    return nclusters;
}

static int qr_finder_vline_cmp(const void* _a, const void* _b) {
    const qr_finder_line* a;
    const qr_finder_line* b;
    a = (const qr_finder_line*)_a;
    b = (const qr_finder_line*)_b;
    return ((a->pos[0] > b->pos[0]) - (a->pos[0] < b->pos[0]) << 1) +
        (a->pos[1] > b->pos[1]) - (a->pos[1] < b->pos[1]);
}


static int qr_finder_center_cmp(const void* _a, const void* _b) {
    const qr_finder_center* a;
    const qr_finder_center* b;
    a = (const qr_finder_center*)_a;
    b = (const qr_finder_center*)_b;
    return ((b->nedge_pts > a->nedge_pts) - (b->nedge_pts < a->nedge_pts) << 2) +
        ((a->pos[1] > b->pos[1]) - (a->pos[1] < b->pos[1]) << 1) +
        (a->pos[0] > b->pos[0]) - (a->pos[0] < b->pos[0]);
}

/*Determine if a horizontal line crosses a vertical line.
  _hline: The horizontal line.
  _vline: The vertical line.
  Return: A non-zero value if the lines cross, or zero if they do not.*/
static int qr_finder_lines_are_crossing(const qr_finder_line* _hline,
    const qr_finder_line* _vline) {
    return
        _hline->pos[0] <= _vline->pos[0] && _vline->pos[0] < _hline->pos[0] + _hline->len &&
        _vline->pos[1] <= _hline->pos[1] && _hline->pos[1] < _vline->pos[1] + _vline->len;
}

/*Adds the coordinates of the edge points from the lines contained in the
   given list of clusters to the list of edge points for a finder center.
  Only the edge point position is initialized.
  The edge label and extent are set by qr_finder_edge_pts_aff_classify()
   or qr_finder_edge_pts_hom_classify().
  _edge_pts:   The buffer in which to store the edge points.
  _nedge_pts:  The current number of edge points in the buffer.
  _neighbors:  The list of lines in the cluster.
  _nneighbors: The number of lines in the list of lines in the cluster.
  _v:          0 for horizontal lines and 1 for vertical lines.
  Return: The new total number of edge points.*/
static int qr_finder_edge_pts_fill(qr_finder_edge_pt* _edge_pts, int _nedge_pts,
    qr_finder_cluster** _neighbors, int _nneighbors, int _v) {
    int i;
    for (i = 0; i < _nneighbors; i++) {
        qr_finder_cluster* c;
        int                j;
        c = _neighbors[i];
        for (j = 0; j < c->nlines; j++) {
            qr_finder_line* l;
            l = c->lines[j];
            if (l->boffs > 0) {
                _edge_pts[_nedge_pts].pos[0] = l->pos[0];
                _edge_pts[_nedge_pts].pos[1] = l->pos[1];
                _edge_pts[_nedge_pts].pos[_v] -= l->boffs;
                _nedge_pts++;
            }
            if (l->eoffs > 0) {
                _edge_pts[_nedge_pts].pos[0] = l->pos[0];
                _edge_pts[_nedge_pts].pos[1] = l->pos[1];
                _edge_pts[_nedge_pts].pos[_v] += l->len + l->eoffs;
                _nedge_pts++;
            }
        }
    }
    return _nedge_pts;
}


/*Finds horizontal clusters that cross corresponding vertical clusters,
   presumably corresponding to a finder center.
  _center:     The buffer in which to store putative finder centers.
  _edge_pts:   The buffer to use for the edge point lists for each finder
                center.
  _hclusters:  The clusters of horizontal lines crossing finder patterns.
  _nhclusters: The number of horizontal line clusters.
  _vclusters:  The clusters of vertical lines crossing finder patterns.
  _nvclusters: The number of vertical line clusters.
  Return: The number of putative finder centers.*/
static int qr_finder_find_crossings(qr_finder_center* _centers,
    qr_finder_edge_pt* _edge_pts, qr_finder_cluster* _hclusters, int _nhclusters,
    qr_finder_cluster* _vclusters, int _nvclusters) {
    qr_finder_cluster** hneighbors;
    qr_finder_cluster** vneighbors;
    unsigned char* hmark;
    unsigned char* vmark;
    int                 ncenters;
    int                 i;
    int                 j;
    hneighbors = (qr_finder_cluster**)malloc(_nhclusters * sizeof(*hneighbors));
    vneighbors = (qr_finder_cluster**)malloc(_nvclusters * sizeof(*vneighbors));
    hmark = (unsigned char*)calloc(_nhclusters, sizeof(*hmark));
    vmark = (unsigned char*)calloc(_nvclusters, sizeof(*vmark));
    ncenters = 0;
    /*TODO: This may need some re-working.
      We should be finding groups of clusters such that _all_ horizontal lines in
       _all_ horizontal clusters in the group cross _all_ vertical lines in _all_
       vertical clusters in the group.
      This is equivalent to finding the maximum bipartite clique in the
       connectivity graph, which requires linear progamming to solve efficiently.
      In principle, that is easy to do, but a realistic implementation without
       floating point is a lot of work (and computationally expensive).
      Right now we are relying on a sufficient border around the finder patterns
       to prevent false positives.*/
    for (i = 0; i < _nhclusters; i++)if (!hmark[i]) {
        qr_finder_line* a;
        qr_finder_line* b;
        int             nvneighbors;
        int             nedge_pts;
        int             y;
        a = _hclusters[i].lines[_hclusters[i].nlines >> 1];
        y = nvneighbors = 0;
        for (j = 0; j < _nvclusters; j++)if (!vmark[j]) {
            b = _vclusters[j].lines[_vclusters[j].nlines >> 1];
            if (qr_finder_lines_are_crossing(a, b)) {
                vmark[j] = 1;
                y += (b->pos[1] << 1) + b->len;
                if (b->boffs > 0 && b->eoffs > 0)y += b->eoffs - b->boffs;
                vneighbors[nvneighbors++] = _vclusters + j;
            }
        }
        if (nvneighbors > 0) {
            qr_finder_center* c;
            int               nhneighbors;
            int               x;
            x = (a->pos[0] << 1) + a->len;
            if (a->boffs > 0 && a->eoffs > 0)x += a->eoffs - a->boffs;
            hneighbors[0] = _hclusters + i;
            nhneighbors = 1;
            j = nvneighbors >> 1;
            b = vneighbors[j]->lines[vneighbors[j]->nlines >> 1];
            for (j = i + 1; j < _nhclusters; j++)if (!hmark[j]) {
                a = _hclusters[j].lines[_hclusters[j].nlines >> 1];
                if (qr_finder_lines_are_crossing(a, b)) {
                    hmark[j] = 1;
                    x += (a->pos[0] << 1) + a->len;
                    if (a->boffs > 0 && a->eoffs > 0)x += a->eoffs - a->boffs;
                    hneighbors[nhneighbors++] = _hclusters + j;
                }
            }
            c = _centers + ncenters++;
            c->pos[0] = (x + nhneighbors) / (nhneighbors << 1);
            c->pos[1] = (y + nvneighbors) / (nvneighbors << 1);
            c->edge_pts = _edge_pts;
            nedge_pts = qr_finder_edge_pts_fill(_edge_pts, 0,
                hneighbors, nhneighbors, 0);
            nedge_pts = qr_finder_edge_pts_fill(_edge_pts, nedge_pts,
                vneighbors, nvneighbors, 1);
            c->nedge_pts = nedge_pts;
            _edge_pts += nedge_pts;
        }
    }
    free(vmark);
    free(hmark);
    free(vneighbors);
    free(hneighbors);
    /*Sort the centers by decreasing numbers of edge points.*/
    qsort(_centers, ncenters, sizeof(*_centers), qr_finder_center_cmp);
    return ncenters;
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
    nhclusters = qr_finder_cluster_lines(hclusters, hneighbors, hlines, nhlines, 0);
    /*We need vertical lines to be sorted by X coordinate, with ties broken by Y
       coordinate, for clustering purposes.
      We scan the image in the opposite order for cache efficiency, so sort the
       lines we found here.*/
    qsort(vlines, nvlines, sizeof(*vlines), qr_finder_vline_cmp);
    vneighbors = (qr_finder_line**)malloc(nvlines * sizeof(*vneighbors));
    /*We require more than one line per cluster, so there are at most nvlines/2.*/
    vclusters = (qr_finder_cluster*)malloc((nvlines >> 1) * sizeof(*vclusters));
    nvclusters = qr_finder_cluster_lines(vclusters, vneighbors, vlines, nvlines, 1);
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
        centers = (qr_finder_center*)malloc(
            QR_MINI(nhclusters, nvclusters) * sizeof(*centers));
            ncenters = qr_finder_find_crossings(centers, edge_pts,
            hclusters, nhclusters, vclusters, nvclusters);
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


static __inline void qr_svg_centers(const qr_finder_center* centers,
    int ncenters)
{
    int i, j;
    svg_path_start("centers", 1, 0, 0);
    for (i = 0; i < ncenters; i++)
        svg_path_moveto(SVG_ABS, centers[i].pos[0], centers[i].pos[1]);
    svg_path_end();

    svg_path_start("edge-pts", 1, 0, 0);
    for (i = 0; i < ncenters; i++) {
        const qr_finder_center* cen = centers + i;
        for (j = 0; j < cen->nedge_pts; j++)
            svg_path_moveto(SVG_ABS,
                cen->edge_pts[j].pos[0], cen->edge_pts[j].pos[1]);
    }
    svg_path_end();
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

      ncenters = qr_finder_centers_locate(&centers, &edge_pts, reader, 0, 0);
      
      zprintf(14, "%dx%d finders, %d centers:\n",
          reader->finder_lines[0].nlines,
          reader->finder_lines[1].nlines,
          ncenters);
      qr_svg_centers(centers, ncenters);

      if (ncenters >= 3) {
          void* bin = qr_binarize(img->data, img->width, img->height);

          qr_code_data_list qrlist;
     /*     qr_code_data_list_init(&qrlist);

          qr_reader_match_centers(reader, &qrlist, centers, ncenters,
              bin, img->width, img->height);

          if (qrlist.nqrdata > 0)
              nqrdata = qr_code_data_list_extract_text(&qrlist, iscn, img);

          qr_code_data_list_clear(&qrlist);
          free(bin); */
      }
      svg_group_end();

      if (centers)
          free(centers);
      if (edge_pts)
          free(edge_pts);
      return(nqrdata); 
}