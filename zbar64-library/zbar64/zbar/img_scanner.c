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

#define TEST_CFG(iscn, cfg) (((iscn)->config >> ((cfg) - ZBAR_CFG_POSITION)) & 1)

#ifndef NO_STATS
# define STAT(x) iscn->stat_##x++
#else
# define STAT(...)
# define dump_stats(...)
#endif


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


#ifdef ENABLE_QRCODE
extern qr_finder_line* _zbar_decoder_get_qr_finder_line(zbar_decoder_t*);

# define QR_FIXED(v, rnd) ((((v) << 1) + (rnd)) << (QR_FINDER_SUBPREC - 1))
# define PRINT_FIXED(val, prec) \
    ((val) >> (prec)),         \
        (1000 * ((val) & ((1 << (prec)) - 1)) / (1 << (prec)))

static __inline void qr_handler(zbar_image_scanner_t* iscn)
{
    unsigned u;
    int vert;
    qr_finder_line* line = _zbar_decoder_get_qr_finder_line(iscn->dcode);
    assert(line);
    u = zbar_scanner_get_edge(iscn->scn, line->pos[0],
        QR_FINDER_SUBPREC);
    line->boffs = u - zbar_scanner_get_edge(iscn->scn, line->boffs,
        QR_FINDER_SUBPREC);
    line->len = zbar_scanner_get_edge(iscn->scn, line->len,
        QR_FINDER_SUBPREC);
    line->eoffs = zbar_scanner_get_edge(iscn->scn, line->eoffs,
        QR_FINDER_SUBPREC) - line->len;
    line->len -= u;

    u = QR_FIXED(iscn->umin, 0) + iscn->du * u;
    if (iscn->du < 0) {
        int tmp = line->boffs;
        line->boffs = line->eoffs;
        line->eoffs = tmp;
        u -= line->len;
    }
    vert = !iscn->dx;
    line->pos[vert] = u;
    line->pos[!vert] = QR_FIXED(iscn->v, 1);
    
    _zbar_qr_found_line(iscn->qr, vert, line); 
}
#endif

__inline zbar_symbol_t*
_zbar_image_scanner_alloc_sym(zbar_image_scanner_t* iscn,
    zbar_symbol_type_t type,
    int datalen)
{
    /* recycle old or alloc new symbol */
    zbar_symbol_t* sym = NULL;
    int i;
    for (i = 0; i < RECYCLE_BUCKETS - 1; i++)
        if (datalen <= 1 << (i * 2))
            break;
    
    for (; i > 0; i--)
        if ((sym = iscn->recycle[i].head)) {
            STAT(sym_recycle[i]);
            break;
        }

    if (sym) {
        iscn->recycle[i].head = sym->next;
        sym->next = NULL;
        assert(iscn->recycle[i].nsyms);
        iscn->recycle[i].nsyms--;
    }
    else {
        sym = calloc(1, sizeof(zbar_symbol_t));
        STAT(sym_new);
    }
    
    /* init new symbol */
    sym->type = type;
    sym->quality = 1;
    sym->npts = 0;
    sym->orient = ZBAR_ORIENT_UNKNOWN;
    sym->cache_count = 0;
    sym->time = iscn->time;
    assert(!sym->syms);

    if (datalen > 0) {
        sym->datalen = datalen - 1;
        if (sym->data_alloc < datalen) {
            if (sym->data)
                free(sym->data);
            sym->data_alloc = datalen;
            sym->data = malloc(datalen);
        }
    }
    else {
        if (sym->data)
            free(sym->data);
        sym->data = NULL;
        sym->datalen = sym->data_alloc = 0;
    }
    return(sym); 
}


static void symbol_handler(zbar_decoder_t* dcode)
{
      zbar_image_scanner_t* iscn = zbar_decoder_get_userdata(dcode);
      zbar_symbol_type_t type = zbar_decoder_get_type(dcode);
      int x = 0, y = 0, dir;
      const char* data;
      unsigned datalen;
      zbar_symbol_t* sym;

  #ifdef ENABLE_QRCODE
      if (type == ZBAR_QRCODE) {
          qr_handler(iscn);
          return;
      }
  #else
      assert(type != ZBAR_QRCODE);
  #endif
     
      if (TEST_CFG(iscn, ZBAR_CFG_POSITION)) { 
      /* tmp position fixup */
       int w = zbar_scanner_get_width(iscn->scn);
       int u = iscn->umin + iscn->du * zbar_scanner_get_edge(iscn->scn, w, 0);
       if (iscn->dx) {
           x = u;
           y = iscn->v;
       }
       else {
           x = iscn->v;
           y = u;
       } 
   }
   

   /* FIXME debug flag to save/display all PARTIALs */
   if (type <= ZBAR_PARTIAL) {
       zprintf(256, "partial symbol @(%d,%d)\n", x, y);
       return;
   }

   data = zbar_decoder_get_data(dcode);
   datalen = zbar_decoder_get_data_length(dcode); 

   /* FIXME need better symbol matching */
   for (sym = iscn->syms->head; sym; sym = sym->next)
       if (sym->type == type &&
           sym->datalen == datalen &&
           !memcmp(sym->data, data, datalen)) {
           sym->quality++;
           zprintf(224, "dup symbol @(%d,%d): dup %s: %.20s\n",
               x, y, zbar_get_symbol_name(type), data);
           if (TEST_CFG(iscn, ZBAR_CFG_POSITION))  
           /* add new point to existing set */
           /* FIXME should be polygon */
             sym_add_point(sym, x, y);
           return;
    }

   
 sym = _zbar_image_scanner_alloc_sym(iscn, type, datalen + 1);
 //sym->configs = zbar_decoder_get_configs(dcode, type);
 //sym->modifiers = zbar_decoder_get_modifiers(dcode); 

 /* FIXME grab decoder buffer */
// memcpy(sym->data, data, datalen + 1);

 /* initialize first point */
/* if (TEST_CFG(iscn, ZBAR_CFG_POSITION)) {
     zprintf(192, "new symbol @(%d,%d): %s: %.20s\n",
         x, y, zbar_get_symbol_name(type), data);
     sym_add_point(sym, x, y);
 }

 dir = zbar_decoder_get_direction(dcode);
 if (dir)
     sym->orient = (iscn->dy != 0) + ((iscn->du ^ dir) & 2);

 _zbar_image_scanner_add_sym(iscn, sym);

 */
}


zbar_image_scanner_t* zbar_image_scanner_create(void)
{
	printf("zbar_image_scanner_create  in\r\n");
    zbar_image_scanner_t* iscn = calloc(1, sizeof(zbar_image_scanner_t));
    if (!iscn)
        return(NULL);
    iscn->dcode = zbar_decoder_create();
    iscn->scn = zbar_scanner_create(iscn->dcode);
    if (!iscn->dcode || !iscn->scn) {
        zbar_image_scanner_destroy(iscn);
        return(NULL);
    }

    zbar_decoder_set_userdata(iscn->dcode, iscn);
    zbar_decoder_set_handler(iscn->dcode, symbol_handler);

    printf("zbar_image_scanner_create  out\r\n");
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
            zbar_symbol_set_ref(iscn->syms, -1);
        else
            _zbar_symbol_set_free(iscn->syms);
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
            _zbar_symbol_free(sym);
        }
    }
#ifdef ENABLE_QRCODE
    if(iscn->qr) {
        _zbar_qr_destroy(iscn->qr);
        iscn->qr = NULL;
    }
#endif
    
    free(iscn);
}

