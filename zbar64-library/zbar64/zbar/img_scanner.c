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
#include <config.h>
#include <stdio.h>

#include <stdlib.h>     /* malloc, free */
#include <zbar.h>
#include "error.h"

#include "symbol.h"

#ifdef ENABLE_QRCODE
# include "qrcode.h"
#endif

#define RECYCLE_BUCKETS     5

typedef struct recycle_bucket_s {
    int nsyms;
    zbar_symbol_t* head;
} recycle_bucket_t;

/* image scanner state */
struct zbar_image_scanner_s {
    zbar_scanner_t* scn;        /* associated linear intensity scanner */
    zbar_decoder_t* dcode;      /* associated symbol decoder */
#ifdef ENABLE_QRCODE
    qr_reader* qr;              /* QR Code 2D reader */
#endif

    const void* userdata;       /* application data */
    /* user result callback */
    //zbar_image_data_handler_t* handler;

    unsigned long time;         /* scan start time */
    //zbar_image_t* img;          /* currently scanning image *root* */
    int dx, dy, du, umin, v;    /* current scan direction */
    zbar_symbol_set_t* syms;    /* previous decode results */
    /* recycled symbols in 4^n size buckets */
    recycle_bucket_t recycle[RECYCLE_BUCKETS];

    int enable_cache;           /* current result cache state */
    //zbar_symbol_t* cache;       /* inter-image result cache entries */

    /* configuration settings */
    unsigned config;            /* config flags */
    unsigned ean_config;
    //int configs[NUM_SCN_CFGS];  /* int valued configurations */
    //int sym_configs[1][NUM_SYMS]; /* per-symbology configurations */

#ifndef NO_STATS
    int stat_syms_new;
    int stat_iscn_syms_inuse, stat_iscn_syms_recycle;
    int stat_img_syms_inuse, stat_img_syms_recycle;
    int stat_sym_new;
    int stat_sym_recycle[RECYCLE_BUCKETS];
#endif
};

zbar_image_scanner_t* zbar_image_scanner_create(void)
{
	printf("testing .....\r\n");
    zbar_image_scanner_t* iscn = calloc(1, sizeof(zbar_image_scanner_t));
    if (!iscn)
        return(NULL);
    iscn->dcode = zbar_decoder_create();
    iscn->scn = zbar_scanner_create(iscn->dcode);
    if (!iscn->dcode || !iscn->scn) {
        zbar_image_scanner_destroy(iscn);
        return(NULL);
    }

    return(iscn);
}

#ifndef NO_STATS
static __inline void dump_stats(const zbar_image_scanner_t* iscn)
{
      int i;
      zprintf(1, "symbol sets allocated   = %-4d\n", iscn->stat_syms_new);
      zprintf(1, "    scanner syms in use = %-4d\trecycled  = %-4d\n",
        iscn->stat_iscn_syms_inuse, iscn->stat_iscn_syms_recycle);
      zprintf(1, "    image syms in use   = %-4d\trecycled  = %-4d\n",
        iscn->stat_img_syms_inuse, iscn->stat_img_syms_recycle);
      zprintf(1, "symbols allocated       = %-4d\n", iscn->stat_sym_new);
      for (i = 0; i < RECYCLE_BUCKETS; i++)
        zprintf(1, "     recycled[%d]        = %-4d\n",
            i, iscn->stat_sym_recycle[i]);
  
}
#endif

void zbar_image_scanner_destroy (zbar_image_scanner_t *iscn)
{
    
    int i;
    dump_stats(iscn);
    if(iscn->syms) {
         if (iscn->syms->refcnt)
            ;
 //           zbar_symbol_set_ref(iscn->syms, -1);
 //       else
 //           _zbar_symbol_set_free(iscn->syms);
        iscn->syms = NULL;
    }
    if(iscn->scn)
        zbar_scanner_destroy(iscn->scn);
    iscn->scn = NULL;
    if(iscn->dcode)
        zbar_decoder_destroy(iscn->dcode);
    iscn->dcode = NULL;
    for(i = 0; i < RECYCLE_BUCKETS; i++) {
        zbar_symbol_t *sym, *next;
        for(sym = iscn->recycle[i].head; sym; sym = next) {
            next = sym->next;
        //    _zbar_symbol_free(sym);
        }
    }
#ifdef ENABLE_QRCODE
    if(iscn->qr) {
    //    _zbar_qr_destroy(iscn->qr);
        iscn->qr = NULL;
    }
#endif
    
    free(iscn);
}
