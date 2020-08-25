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

#include "error.h"
#include "image.h"
#include <zbar.h>
#include "refcnt.h"
#include "symbol.h"

zbar_image_t* zbar_image_create()
{
    zbar_image_t* img = calloc(1, sizeof(zbar_image_t));
    _zbar_refcnt_init();
    _zbar_image_refcnt(img, 1);
    img->srcidx = -1;
    return(img);
}

void _zbar_image_free(zbar_image_t* img)
{
    if (img->syms) {
        zbar_symbol_set_ref(img->syms, -1);
        img->syms = NULL;
    }
    free(img);
}

void zbar_image_destroy(zbar_image_t* img)
{
    _zbar_image_refcnt(img, -1);
}

void zbar_image_set_format(zbar_image_t* img,
    unsigned long fmt)
{
    img->format = fmt;
}

void zbar_image_set_size(zbar_image_t* img,
    unsigned w,
    unsigned h)
{
    img->crop_x = img->crop_y = 0;
    img->width = img->crop_w = w;
    img->height = img->crop_h = h;
}

void zbar_image_set_data(zbar_image_t* img,
    const void* data,
    unsigned long len,
    zbar_image_cleanup_handler_t* cleanup)
{
    zbar_image_free_data(img);
    img->data = data;
    img->datalen = len;
    img->cleanup = cleanup;
}

__inline void zbar_image_free_data(zbar_image_t* img)
{
    if (!img)
        return;
    if (img->src) {
        zbar_image_t* newimg;
        /* replace video image w/new copy */
        assert(img->refcnt); /* FIXME needs lock */
        newimg = zbar_image_create();
        memcpy(newimg, img, sizeof(zbar_image_t));
        /* recycle video image */
        newimg->cleanup(newimg);
        /* detach old image from src */
        img->cleanup = NULL;
        img->src = NULL;
        img->srcidx = -1;
    }
    else if (img->cleanup && img->data) {
        if (img->cleanup != zbar_image_free_data) {
            /* using function address to detect this case is a bad idea;
             * windows link libraries add an extra layer of indirection...
             * this works around that problem (bug #2796277)
             */
            zbar_image_cleanup_handler_t* cleanup = img->cleanup;
            img->cleanup = zbar_image_free_data;
            cleanup(img);
        }
        else
            free((void*)img->data);
    }
    img->data = NULL;
}

const zbar_symbol_t* zbar_image_first_symbol(const zbar_image_t* img)
{
    return((img->syms) ? img->syms->head : NULL);
}



