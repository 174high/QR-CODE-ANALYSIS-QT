/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <zbar.h>
#include "refcnt.h"
#include <stdint.h>

struct zbar_image_s {
    uint32_t format;            /* fourcc image format code */
    unsigned width, height;     /* image size */
    const void* data;           /* image sample data */
    unsigned long datalen;      /* allocated/mapped size of data */
    unsigned crop_x, crop_y;    /* crop rectangle */
    unsigned crop_w, crop_h;
    void* userdata;             /* user specified data associated w/image */

    /* cleanup handler */
    zbar_image_cleanup_handler_t* cleanup;
    refcnt_t refcnt;            /* reference count */
    zbar_video_t* src;          /* originator */
    int srcidx;                 /* index used by originator */
    zbar_image_t* next;         /* internal image lists */

    unsigned seq;               /* page/frame sequence number */
    zbar_symbol_set_t* syms;    /* decoded result set */
};

extern void _zbar_image_free(zbar_image_t*);


static __inline void _zbar_image_refcnt(zbar_image_t* img,
    int delta)
{
    if (!_zbar_refcnt(&img->refcnt, delta) && delta <= 0) {
        if (img->cleanup)
            img->cleanup(img);
        if (!img->src)
            _zbar_image_free(img);
    }
}












#endif
