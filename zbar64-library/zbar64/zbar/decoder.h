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
#ifndef _DECODER_H_
#define _DECODER_H_

#include <config.h>

#include <zbar.h>

#include "debug.h"

#ifdef ENABLE_QRCODE
# include "decoder/qr_finder.h"
#endif

 /* size of bar width history (implementation assumes power of two) */
#ifndef DECODE_WINDOW
# define DECODE_WINDOW  16
#endif

 /* initial data buffer allocation */
#ifndef BUFFER_MIN
# define BUFFER_MIN   0x20
#endif

#define CFG(dcode, cfg) ((dcode).configs[(cfg) - ZBAR_CFG_MIN_LEN])
#define TEST_CFG(config, cfg) (((config) >> (cfg)) & 1)
#define MOD(mod) (1 << (mod))

/* symbology independent decoder state */
struct zbar_decoder_s {
    unsigned char idx;                  /* current width index */
    unsigned w[DECODE_WINDOW];          /* window of last N bar widths */
    zbar_symbol_type_t type;            /* type of last decoded data */
    zbar_symbol_type_t lock;            /* buffer lock */
    unsigned modifiers;                 /* symbology modifier */
    int direction;                      /* direction of last decoded data */
    unsigned s6;                        /* 6-element character width */

    /* everything above here is automatically reset */
    unsigned buf_alloc;                 /* dynamic buffer allocation */
    unsigned buflen;                    /* binary data length */
    unsigned char* buf;                 /* decoded characters */
    void* userdata;                     /* application data */
    zbar_decoder_handler_t* handler;    /* application callback */

    /* symbology specific state */
#ifdef ENABLE_EAN
    ean_decoder_t ean;                  /* EAN/UPC parallel decode attempts */
#endif
#ifdef ENABLE_I25
    i25_decoder_t i25;                  /* Interleaved 2 of 5 decode state */
#endif
#ifdef ENABLE_DATABAR
    databar_decoder_t databar;          /* DataBar decode state */
#endif
#ifdef ENABLE_CODABAR
    codabar_decoder_t codabar;          /* Codabar decode state */
#endif
#ifdef ENABLE_CODE39
    code39_decoder_t code39;            /* Code 39 decode state */
#endif
#ifdef ENABLE_CODE93
    code93_decoder_t code93;            /* Code 93 decode state */
#endif
#ifdef ENABLE_CODE128
    code128_decoder_t code128;          /* Code 128 decode state */
#endif
#ifdef ENABLE_PDF417
    pdf417_decoder_t pdf417;            /* PDF417 decode state */
#endif
#ifdef ENABLE_QRCODE
    qr_finder_t qrf;                    /* QR Code finder state */
#endif
};

/* retrieve i-th previous element width */
static __inline unsigned get_width(const zbar_decoder_t* dcode,
    unsigned char offset)
{
    return(dcode->w[(dcode->idx - offset) & (DECODE_WINDOW - 1)]);
}

/* check and release shared state lock */
static __inline char release_lock(zbar_decoder_t* dcode,
    zbar_symbol_type_t req)
{
    zassert(dcode->lock == req, 1, "lock=%d req=%d\n",
        dcode->lock, req);
    dcode->lock = 0;
    return(0);
}

#endif