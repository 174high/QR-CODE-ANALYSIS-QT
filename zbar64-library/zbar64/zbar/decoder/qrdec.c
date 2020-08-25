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

/*The number of bits in an int.
  Note the cast to (int): this prevents this value from "promoting" whole
   expressions to an (unsigned) size_t.*/
#define QR_INT_BITS    ((int)sizeof(int)*CHAR_BIT)
#define QR_INT_LOGBITS (QR_ILOG(QR_INT_BITS))

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

void qr_code_data_list_init(qr_code_data_list* _qrlist) {
    _qrlist->qrdata = NULL;
    _qrlist->nqrdata = _qrlist->cqrdata = 0;
}

/*Returns the cross product of the three points, which is positive if they are
   in CCW order (in a right-handed coordinate system), and 0 if they're
   colinear.*/
static int qr_point_ccw(const qr_point _p0,
    const qr_point _p1, const qr_point _p2) {
    return (_p1[0] - _p0[0]) * (_p2[1] - _p0[1]) - (_p1[1] - _p0[1]) * (_p2[0] - _p0[0]);
}

static unsigned qr_point_distance2(const qr_point _p1, const qr_point _p2) {
    return (_p1[0] - _p2[0]) * (_p1[0] - _p2[0]) + (_p1[1] - _p2[1]) * (_p1[1] - _p2[1]);
}



/*An affine homography.
  This maps from the image (at subpel resolution) to a square domain with
   power-of-two sides (of res bits) and back.*/
struct qr_aff {
    int fwd[2][2];
    int inv[2][2];
    int x0;
    int y0;
    int res;
    int ires;
};


/*A full homography.
  Like the affine homography, this maps from the image (at subpel resolution)
   to a square domain with power-of-two sides (of res bits) and back.*/
struct qr_hom {
    int fwd[3][2];
    int inv[3][2];
    int fwd22;
    int inv22;
    int x0;
    int y0;
    int res;
};


/*All the information we've collected about a finder pattern in the current
   configuration.*/
struct qr_finder {
    /*The module size along each axis (in the square domain).*/
    int                size[2];
    /*The version estimated from the module size along each axis.*/
    int                eversion[2];
    /*The list of classified edge points for each edge.*/
    qr_finder_edge_pt* edge_pts[4];
    /*The number of edge points classified as belonging to each edge.*/
    int                nedge_pts[4];
    /*The number of inliers found after running RANSAC on each edge.*/
    int                ninliers[4];
    /*The center of the finder pattern (in the square domain).*/
    qr_point           o;
    /*The finder center information from the original image.*/
    qr_finder_center* c;
};

static void qr_aff_init(qr_aff* _aff,
    const qr_point _p0, const qr_point _p1, const qr_point _p2, int _res) {
    int det;
    int ires;
    int dx1;
    int dy1;
    int dx2;
    int dy2;
    /*det is ensured to be positive by our caller.*/
    dx1 = _p1[0] - _p0[0];
    dx2 = _p2[0] - _p0[0];
    dy1 = _p1[1] - _p0[1];
    dy2 = _p2[1] - _p0[1];
    det = dx1 * dy2 - dy1 * dx2;
    ires = QR_MAXI((qr_ilog(abs(det)) >> 1) - 2, 0);
    _aff->fwd[0][0] = dx1;
    _aff->fwd[0][1] = dx2;
    _aff->fwd[1][0] = dy1;
    _aff->fwd[1][1] = dy2;
    _aff->inv[0][0] = QR_DIVROUND(dy2 << _res, det >> ires);
    _aff->inv[0][1] = QR_DIVROUND(-dx2 << _res, det >> ires);
    _aff->inv[1][0] = QR_DIVROUND(-dy1 << _res, det >> ires);
    _aff->inv[1][1] = QR_DIVROUND(dx1 << _res, det >> ires);
    _aff->x0 = _p0[0];
    _aff->y0 = _p0[1];
    _aff->res = _res;
    _aff->ires = ires;
}

/*Map from the image (at subpel resolution) into the square domain.*/
static void qr_aff_unproject(qr_point _q, const qr_aff* _aff,
    int _x, int _y) {
    _q[0] = _aff->inv[0][0] * (_x - _aff->x0) + _aff->inv[0][1] * (_y - _aff->y0)
        + (1 << _aff->ires >> 1) >> _aff->ires;
    _q[1] = _aff->inv[1][0] * (_x - _aff->x0) + _aff->inv[1][1] * (_y - _aff->y0)
        + (1 << _aff->ires >> 1) >> _aff->ires;
}

static int qr_cmp_edge_pt(const void* _a, const void* _b) {
    const qr_finder_edge_pt* a;
    const qr_finder_edge_pt* b;
    a = (const qr_finder_edge_pt*)_a;
    b = (const qr_finder_edge_pt*)_b;
    return ((a->edge > b->edge) - (a->edge < b->edge) << 1) +
        (a->extent > b->extent) - (a->extent < b->extent);
}

static void qr_point_translate(qr_point _point, int _dx, int _dy) {
    _point[0] += _dx;
    _point[1] += _dy;
}

/*Computes the index of the edge each edge point belongs to, and its (signed)
   distance along the corresponding axis from the center of the finder pattern
   (in the square domain).
  The resulting list of edge points is sorted by edge index, with ties broken
   by extent.*/
static void qr_finder_edge_pts_aff_classify(qr_finder* _f, const qr_aff* _aff) {
    qr_finder_center* c;
    int               i;
    int               e;
    c = _f->c;
    for (e = 0; e < 4; e++)_f->nedge_pts[e] = 0;
    for (i = 0; i < c->nedge_pts; i++) {
        qr_point q;
        int      d;
        qr_aff_unproject(q, _aff, c->edge_pts[i].pos[0], c->edge_pts[i].pos[1]);
        qr_point_translate(q, -_f->o[0], -_f->o[1]);
        d = abs(q[1]) > abs(q[0]);
        e = d << 1 | (q[d] >= 0);
        _f->nedge_pts[e]++;
        c->edge_pts[i].edge = e;
        c->edge_pts[i].extent = q[d];
    }
    qsort(c->edge_pts, c->nedge_pts, sizeof(*c->edge_pts), qr_cmp_edge_pt);
    _f->edge_pts[0] = c->edge_pts;
    for (e = 1; e < 4; e++)_f->edge_pts[e] = _f->edge_pts[e - 1] + _f->nedge_pts[e - 1];
}

/*TODO: Perhaps these thresholds should be on the module size instead?
  Unfortunately, I'd need real-world images of codes with larger versions to
   see if these thresholds are still effective, but such versions aren't used
   often.*/

   /*The amount that the estimated version numbers are allowed to differ from the
      real version number and still be considered valid.*/
#define QR_SMALL_VERSION_SLACK (1)
      /*Since cell phone cameras can have severe radial distortion, the estimated
         version for larger versions can be off by larger amounts.*/
#define QR_LARGE_VERSION_SLACK (3)

         /*Estimates the size of a module after classifying the edge points.
           _width:  The distance between UL and UR in the square domain.
           _height: The distance between UL and DL in the square domain.*/
static int qr_finder_estimate_module_size_and_version(qr_finder* _f,
    int _width, int _height) {
    qr_point offs;
    int      sums[4];
    int      nsums[4];
    int      usize;
    int      nusize;
    int      vsize;
    int      nvsize;
    int      uversion;
    int      vversion;
    int      e;
    offs[0] = offs[1] = 0;
    for (e = 0; e < 4; e++)if (_f->nedge_pts[e] > 0) {
        qr_finder_edge_pt* edge_pts;
        int                sum;
        int                mean;
        int                n;
        int                i;
        /*Average the samples for this edge, dropping the top and bottom 25%.*/
        edge_pts = _f->edge_pts[e];
        n = _f->nedge_pts[e];
        sum = 0;
        for (i = (n >> 2); i < n - (n >> 2); i++)sum += edge_pts[i].extent;
        n = n - ((n >> 2) << 1);
        mean = QR_DIVROUND(sum, n);
        offs[e >> 1] += mean;
        sums[e] = sum;
        nsums[e] = n;
    }
    else nsums[e] = sums[e] = 0;
    /*If we have samples on both sides of an axis, refine our idea of where the
       unprojected finder center is located.*/
    if (_f->nedge_pts[0] > 0 && _f->nedge_pts[1] > 0) {
        _f->o[0] -= offs[0] >> 1;
        sums[0] -= offs[0] * nsums[0] >> 1;
        sums[1] -= offs[0] * nsums[1] >> 1;
    }
    if (_f->nedge_pts[2] > 0 && _f->nedge_pts[3] > 0) {
        _f->o[1] -= offs[1] >> 1;
        sums[2] -= offs[1] * nsums[2] >> 1;
        sums[3] -= offs[1] * nsums[3] >> 1;
    }
    /*We must have _some_ samples along each axis... if we don't, our transform
       must be pretty severely distorting the original square (e.g., with
       coordinates so large as to cause overflow).*/
    nusize = nsums[0] + nsums[1];
    if (nusize <= 0)return -1;
    /*The module size is 1/3 the average edge extent.*/
    nusize *= 3;
    usize = sums[1] - sums[0];
    usize = ((usize << 1) + nusize) / (nusize << 1);
    if (usize <= 0)return -1;
    /*Now estimate the version directly from the module size and the distance
       between the finder patterns.
      This is done independently using the extents along each axis.
      If either falls significantly outside the valid range (1 to 40), reject the
       configuration.*/
    uversion = (_width - 8 * usize) / (usize << 2);
    if (uversion < 1 || uversion>40 + QR_LARGE_VERSION_SLACK)return -1;
    /*Now do the same for the other axis.*/
    nvsize = nsums[2] + nsums[3];
    if (nvsize <= 0)return -1;
    nvsize *= 3;
    vsize = sums[3] - sums[2];
    vsize = ((vsize << 1) + nvsize) / (nvsize << 1);
    if (vsize <= 0)return -1;
    vversion = (_height - 8 * vsize) / (vsize << 2);
    if (vversion < 1 || vversion>40 + QR_LARGE_VERSION_SLACK)return -1;
    /*If the estimated version using extents along one axis is significantly
       different than the estimated version along the other axis, then the axes
       have significantly different scalings (relative to the grid).
      This can happen, e.g., when we have multiple adjacent QR codes, and we've
       picked two finder patterns from one and the third finder pattern from
       another, e.g.:
        X---DL UL---X
        |....   |....
        X....  UR....
      Such a configuration might even pass any other geometric checks if we
       didn't reject it here.*/
    if (abs(uversion - vversion) > QR_LARGE_VERSION_SLACK)return -1;
    _f->size[0] = usize;
    _f->size[1] = vsize;
    /*We intentionally do not compute an average version from the sizes along
       both axes.
      In the presence of projective distortion, one of them will be much more
       accurate than the other.*/
    _f->eversion[0] = uversion;
    _f->eversion[1] = vversion;
    return 0;
}


/*Searches for an arrangement of these three finder centers that yields a valid
   configuration.
  _c: On input, the three finder centers to consider in any order.
  Return: The detected version number, or a negative value on error.*/
static int qr_reader_try_configuration(qr_reader* _reader,
    qr_code_data* _qrdata, const unsigned char* _img, int _width, int _height,
    qr_finder_center* _c[3]) {
    int      ci[7];
    unsigned maxd;
    int      ccw;
    int      i0;
    int      i;
  
    /*Sort the points in counter-clockwise order.*/
    ccw = qr_point_ccw(_c[0]->pos, _c[1]->pos, _c[2]->pos);
    /*Colinear points can't be the corners of a quadrilateral.*/
    if (!ccw)return -1;
    /*Include a few extra copies of the cyclical list to avoid mods.*/
    ci[6] = ci[3] = ci[0] = 0;
    ci[4] = ci[1] = 1 + (ccw < 0);
    ci[5] = ci[2] = 2 - (ccw < 0);
    /*Assume the points farthest from each other are the opposite corners, and
       find the top-left point.*/
    maxd = qr_point_distance2(_c[1]->pos, _c[2]->pos);
    i0 = 0;
    for (i = 1; i < 3; i++) {
        unsigned d;
        d = qr_point_distance2(_c[ci[i + 1]]->pos, _c[ci[i + 2]]->pos);
        if (d > maxd) {
            i0 = i;
            maxd = d;
        }
    }

    /*However, try all three possible orderings, just to be sure (a severely
         skewed projection could move opposite corners closer than adjacent).*/
    for (i = i0; i < i0 + 3; i++) {
        qr_aff    aff;
        qr_hom    hom;
        qr_finder ul;
        qr_finder ur;
        qr_finder dl;
        qr_point  bbox[4];
        int       res;
        int       ur_version;
        int       dl_version;
        int       fmt_info;
        ul.c = _c[ci[i]];
        ur.c = _c[ci[i + 1]];
        dl.c = _c[ci[i + 2]]; 
        /*Estimate the module size and version number from the two opposite corners.
          The module size is not constant in the image, so we compute an affine
           projection from the three points we have to a square domain, and
           estimate it there.
          Although it should be the same along both axes, we keep separate
           estimates to account for any remaining projective distortion.*/
        res = QR_INT_BITS - 2 - QR_FINDER_SUBPREC - qr_ilog(QR_MAXI(_width, _height) - 1);
        qr_aff_init(&aff, ul.c->pos, ur.c->pos, dl.c->pos, res);
        qr_aff_unproject(ur.o, &aff, ur.c->pos[0], ur.c->pos[1]);
        qr_finder_edge_pts_aff_classify(&ur, &aff);
        if (qr_finder_estimate_module_size_and_version(&ur, 1 << res, 1 << res) < 0)continue;
        qr_aff_unproject(dl.o, &aff, dl.c->pos[0], dl.c->pos[1]);
        qr_finder_edge_pts_aff_classify(&dl, &aff);
        if (qr_finder_estimate_module_size_and_version(&dl, 1 << res, 1 << res) < 0)continue;  
        /*If the estimated versions are significantly different, reject the
           configuration.*/
        if (abs(ur.eversion[1] - dl.eversion[0]) > QR_LARGE_VERSION_SLACK)continue;
        qr_aff_unproject(ul.o, &aff, ul.c->pos[0], ul.c->pos[1]);
        qr_finder_edge_pts_aff_classify(&ul, &aff);
        if (qr_finder_estimate_module_size_and_version(&ul, 1 << res, 1 << res) < 0 ||
            abs(ul.eversion[1] - ur.eversion[1]) > QR_LARGE_VERSION_SLACK ||
            abs(ul.eversion[0] - dl.eversion[0]) > QR_LARGE_VERSION_SLACK) {
            continue;
        }





    }

    return -1;
}

void qr_reader_match_centers(qr_reader* _reader, qr_code_data_list* _qrlist,
    qr_finder_center* _centers, int _ncenters,
    const unsigned char* _img, int _width, int _height) {
    /*The number of centers should be small, so an O(n^3) exhaustive search of
       which ones go together should be reasonable.*/
    unsigned char* mark;
    int            nfailures_max;
    int            nfailures;
    int            i;
    int            j;
    int            k;
    mark = (unsigned char*)calloc(_ncenters, sizeof(*mark));
    nfailures_max = QR_MAXI(8192, _width * _height >> 9);
    nfailures = 0;
    for (i = 0; i < _ncenters; i++) {
        /*TODO: We might be able to accelerate this step significantly by
           considering the remaining finder centers in a more intelligent order,
           based on the first finder center we just chose.*/
        for (j = i + 1; !mark[i] && j < _ncenters; j++) {
            for (k = j + 1; !mark[j] && k < _ncenters; k++)if (!mark[k]) {
                qr_finder_center* c[3];
                qr_code_data      qrdata;
                int               version;
                c[0] = _centers + i;
                c[1] = _centers + j;
                c[2] = _centers + k;
                version = qr_reader_try_configuration(_reader, &qrdata,
                    _img, _width, _height, c);
                if (version >= 0) {
                    int ninside;
                    int l;
                    /*Add the data to the list.*/
         //           qr_code_data_list_add(_qrlist, &qrdata);
                    /*Convert the bounding box we're returning to the user to normal
                       image coordinates.*/
                    for (l = 0; l < 4; l++) {
                        _qrlist->qrdata[_qrlist->nqrdata - 1].bbox[l][0] >>= QR_FINDER_SUBPREC;
                        _qrlist->qrdata[_qrlist->nqrdata - 1].bbox[l][1] >>= QR_FINDER_SUBPREC;
                    }
                    /*Mark these centers as used.*/
                    mark[i] = mark[j] = mark[k] = 1;
                    /*Find any other finder centers located inside this code.*/
                    for (l = ninside = 0; l < _ncenters; l++)if (!mark[l]) {
          //              if (qr_point_ccw(qrdata.bbox[0], qrdata.bbox[1], _centers[l].pos) >= 0 &&
         //                   qr_point_ccw(qrdata.bbox[1], qrdata.bbox[3], _centers[l].pos) >= 0 &&
          //                  qr_point_ccw(qrdata.bbox[3], qrdata.bbox[2], _centers[l].pos) >= 0 &&
         //                   qr_point_ccw(qrdata.bbox[2], qrdata.bbox[0], _centers[l].pos) >= 0) {
       //                     mark[l] = 2;
         //                   ninside++;
          //              }
                    }
                    if (ninside >= 3) {
                        /*We might have a "Double QR": a code inside a code.
                          Copy the relevant centers to a new array and do a search confined
                           to that subset.*/
                        qr_finder_center* inside;
                        inside = (qr_finder_center*)malloc(ninside * sizeof(*inside));
                        for (l = ninside = 0; l < _ncenters; l++) {
                            if (mark[l] == 2)*&inside[ninside++] = *&_centers[l];
                        }
                        qr_reader_match_centers(_reader, _qrlist, inside, ninside,
                            _img, _width, _height);
                        free(inside);
                    }
                    /*Mark _all_ such centers used: codes cannot partially overlap.*/
                    for (l = 0; l < _ncenters; l++)if (mark[l] == 2)mark[l] = 1;
                    nfailures = 0;
                }
                else if (++nfailures > nfailures_max) {
                    /*Give up.
                      We're unlikely to find a valid code in all this clutter, and we
                       could spent quite a lot of time trying.*/
                    i = j = k = _ncenters;
                }
            }
        }
    }
    free(mark);
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
         qr_code_data_list_init(&qrlist);
         
          qr_reader_match_centers(reader, &qrlist, centers, ncenters,
              bin, img->width, img->height);
          /*
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