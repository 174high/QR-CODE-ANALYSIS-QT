#include <config.h>
#include <stdlib.h>     /* malloc, free, abs */
#include <stddef.h>
#include <string.h>     /* memset */

#include <zbar.h>
#include "debug.h"
#include "decoder.h"


#ifndef ZBAR_FIXED
# define ZBAR_FIXED 5
#endif
#define ROUND (1 << (ZBAR_FIXED - 1))


/* FIXME add runtime config API for these */
#ifndef ZBAR_SCANNER_THRESH_MIN
# define ZBAR_SCANNER_THRESH_MIN  4
#endif


#ifndef ZBAR_SCANNER_THRESH_INIT_WEIGHT
# define ZBAR_SCANNER_THRESH_INIT_WEIGHT .44
#endif
#define THRESH_INIT ((unsigned)((ZBAR_SCANNER_THRESH_INIT_WEIGHT       \
                                 * (1 << (ZBAR_FIXED + 1)) + 1) / 2))

#ifndef ZBAR_SCANNER_THRESH_FADE
# define ZBAR_SCANNER_THRESH_FADE 8
#endif

#ifndef ZBAR_SCANNER_EWMA_WEIGHT
# define ZBAR_SCANNER_EWMA_WEIGHT .78
#endif
#define EWMA_WEIGHT ((unsigned)((ZBAR_SCANNER_EWMA_WEIGHT              \
                                 * (1 << (ZBAR_FIXED + 1)) + 1) / 2))

/* scanner state */
struct zbar_scanner_s {
    zbar_decoder_t* decoder; /* associated bar width decoder */
    unsigned y1_min_thresh; /* minimum threshold */

    unsigned x;             /* relative scan position of next sample */
    int y0[4];              /* short circular buffer of average intensities */

    int y1_sign;            /* slope at last crossing */
    unsigned y1_thresh;     /* current slope threshold */

    unsigned cur_edge;      /* interpolated position of tracking edge */
    unsigned last_edge;     /* interpolated position of last located edge */
    unsigned width;         /* last element width */
};

zbar_scanner_t* zbar_scanner_create(zbar_decoder_t* dcode)
{
    zbar_scanner_t* scn = malloc(sizeof(zbar_scanner_t));
    scn->decoder = dcode;
    scn->y1_min_thresh = ZBAR_SCANNER_THRESH_MIN;
    zbar_scanner_reset(scn);
    return(scn);
}

void zbar_scanner_destroy(zbar_scanner_t* scn)
{
    free(scn);
}

zbar_symbol_type_t zbar_scanner_reset(zbar_scanner_t* scn)
{
    //printf("zbar_scanner_reset size = 0x%x , offset = 0x%x result=0x%x \r\n", sizeof(zbar_scanner_t), offsetof(zbar_scanner_t, x), sizeof(zbar_scanner_t) - offsetof(zbar_scanner_t, x));
    memset(&scn->x, 0, sizeof(zbar_scanner_t) - offsetof(zbar_scanner_t, x));
    scn->y1_thresh = scn->y1_min_thresh;
    if (scn->decoder)
        zbar_decoder_reset(scn->decoder);
    return(ZBAR_NONE);
}

unsigned zbar_scanner_get_edge(const zbar_scanner_t* scn,
    unsigned offset,
    int prec)
{
    unsigned edge = scn->last_edge - offset - (1 << ZBAR_FIXED) - ROUND;
    prec = ZBAR_FIXED - prec;
    if (prec > 0)
        return(edge >> prec);
    else if (!prec)
        return(edge);
    else
        return(edge << -prec);
}

zbar_symbol_type_t zbar_decode_width(zbar_decoder_t* dcode,
    unsigned w)
{
    zbar_symbol_type_t tmp, sym = ZBAR_NONE;

    dcode->w[dcode->idx & (DECODE_WINDOW - 1)] = w;
    dbprintf(1, "    decode[%x]: w=%d (%g)\n", dcode->idx, w, (w / 32.));
    
    /* update shared character width */
    dcode->s6 -= get_width(dcode, 7);
    dcode->s6 += get_width(dcode, 1);

    /* each decoder processes width stream in parallel */
#ifdef ENABLE_QRCODE
   if (TEST_CFG(dcode->qrf.config, ZBAR_CFG_ENABLE) &&
       (tmp = _zbar_find_qr(dcode)) > ZBAR_PARTIAL)
       sym = tmp; 
#endif
#ifdef ENABLE_EAN
    if ((dcode->ean.enable) &&
        (tmp = _zbar_decode_ean(dcode)))
        sym = tmp;
#endif
#ifdef ENABLE_CODE39
    if (TEST_CFG(dcode->code39.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_code39(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_CODE93
    if (TEST_CFG(dcode->code93.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_code93(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_CODE128
    if (TEST_CFG(dcode->code128.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_code128(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_DATABAR
    if (TEST_CFG(dcode->databar.config | dcode->databar.config_exp,
        ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_databar(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_CODABAR
    if (TEST_CFG(dcode->codabar.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_codabar(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_I25
    if (TEST_CFG(dcode->i25.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_i25(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif
#ifdef ENABLE_PDF417
    if (TEST_CFG(dcode->pdf417.config, ZBAR_CFG_ENABLE) &&
        (tmp = _zbar_decode_pdf417(dcode)) > ZBAR_PARTIAL)
        sym = tmp;
#endif

    dcode->idx++;
    dcode->type = sym;
    if (sym) {
        if (dcode->lock && sym > ZBAR_PARTIAL && sym != ZBAR_QRCODE)
            release_lock(dcode, sym);
        if (dcode->handler)
            dcode->handler(dcode);
    }
    return(sym);
}


unsigned zbar_scanner_get_width(const zbar_scanner_t* scn)
{
    return(scn->width);
}

zbar_symbol_type_t zbar_scanner_new_scan(zbar_scanner_t* scn)
{
     zbar_symbol_type_t edge = ZBAR_NONE;
    while (scn->y1_sign) {
        zbar_symbol_type_t tmp = zbar_scanner_flush(scn);
        if (tmp < 0 || tmp > edge)
            edge = tmp;
    }

    /* reset scanner and associated decoder */
    memset(&scn->x, 0, sizeof(zbar_scanner_t) - offsetof(zbar_scanner_t, x));
    scn->y1_thresh = scn->y1_min_thresh;
    if (scn->decoder)
        zbar_decoder_new_scan(scn->decoder);
    return(edge);
}

static __inline zbar_symbol_type_t process_edge(zbar_scanner_t* scn,
    int y1)
{
    if (!scn->y1_sign)
        scn->last_edge = scn->cur_edge = (1 << ZBAR_FIXED) + ROUND;
    else if (!scn->last_edge)
        scn->last_edge = scn->cur_edge;

    scn->width = scn->cur_edge - scn->last_edge;
    dbprintf(1, " sgn=%d cur=%d.%d w=%d (%s)\n",
        scn->y1_sign, scn->cur_edge >> ZBAR_FIXED,
        scn->cur_edge & ((1 << ZBAR_FIXED) - 1), scn->width,
        ((y1 > 0) ? "SPACE" : "BAR"));
    scn->last_edge = scn->cur_edge;
    
#if DEBUG_SVG > 1
    svg_path_moveto(SVG_ABS, scn->last_edge - (1 << ZBAR_FIXED) - ROUND, 0);
#endif

    /* pass to decoder */
    if (scn->decoder)
       return(zbar_decode_width(scn->decoder, scn->width));
    return(ZBAR_PARTIAL);
}


__inline zbar_symbol_type_t zbar_scanner_flush(zbar_scanner_t* scn)
{
    unsigned x;
    if (!scn->y1_sign)
        return(ZBAR_NONE);

    x = (scn->x << ZBAR_FIXED) + ROUND;

    if (scn->cur_edge != x || scn->y1_sign > 0) {
        zbar_symbol_type_t edge = process_edge(scn, -scn->y1_sign);
        dbprintf(1, "flush0:");
        scn->cur_edge = x;
        scn->y1_sign = -scn->y1_sign;
        return(edge);
    }

    scn->y1_sign = scn->width = 0;
    if (scn->decoder)
        return(zbar_decode_width(scn->decoder, 0));

    return(ZBAR_PARTIAL);
}

static __inline unsigned calc_thresh(zbar_scanner_t* scn)
{
    /* threshold 1st to improve noise rejection */
    unsigned dx, thresh = scn->y1_thresh;
    unsigned long t;
    if ((thresh <= scn->y1_min_thresh) || !scn->width) {
        dbprintf(1, " tmin=%d", scn->y1_min_thresh);
        return(scn->y1_min_thresh);
    }
    /* slowly return threshold to min */
    dx = (scn->x << ZBAR_FIXED) - scn->last_edge;
    t = thresh * dx;
    t /= scn->width;
    t /= ZBAR_SCANNER_THRESH_FADE;
    dbprintf(1, " thr=%d t=%ld x=%d last=%d.%d (%d)",
        thresh, t, scn->x, scn->last_edge >> ZBAR_FIXED,
        scn->last_edge & ((1 << ZBAR_FIXED) - 1), dx);
    if (thresh > t) {
        thresh -= t;
        if (thresh > scn->y1_min_thresh)
            return(thresh);
    }
    scn->y1_thresh = scn->y1_min_thresh;
    return(scn->y1_min_thresh);
}

zbar_symbol_type_t zbar_scan_y(zbar_scanner_t* scn,
    int y)
{
    /* FIXME calc and clip to max y range... */
    /* retrieve short value history */
    register int x = scn->x;
    register int y0_1 = scn->y0[(x - 1) & 3];
    register int y0_0 = y0_1;
    register int y0_2, y0_3, y1_1, y2_1, y2_2;
    zbar_symbol_type_t edge;
    if (x) {
        /* update weighted moving average */
        y0_0 += ((int)((y - y0_1) * EWMA_WEIGHT)) >> ZBAR_FIXED;
        scn->y0[x & 3] = y0_0;
    }
    else
        y0_0 = y0_1 = scn->y0[0] = scn->y0[1] = scn->y0[2] = scn->y0[3] = y;
    y0_2 = scn->y0[(x - 2) & 3];
    y0_3 = scn->y0[(x - 3) & 3];
    /* 1st differential @ x-1 */
    y1_1 = y0_1 - y0_2;
    {
        register int y1_2 = y0_2 - y0_3;
        if ((abs(y1_1) < abs(y1_2)) &&
            ((y1_1 >= 0) == (y1_2 >= 0)))
            y1_1 = y1_2;
    }

    /* 2nd differentials @ x-1 & x-2 */
    y2_1 = y0_0 - (y0_1 * 2) + y0_2;
    y2_2 = y0_1 - (y0_2 * 2) + y0_3;

    dbprintf(1, "scan: x=%d y=%d y0=%d y1=%d y2=%d",
        x, y, y0_1, y1_1, y2_1);

    edge = ZBAR_NONE;
    /* 2nd zero-crossing is 1st local min/max - could be edge */
    if ((!y2_1 ||
        ((y2_1 > 0) ? y2_2 < 0 : y2_2 > 0)) &&
        (calc_thresh(scn) <= abs(y1_1)))
    {
        /* check for 1st sign change */
        char y1_rev = (scn->y1_sign > 0) ? y1_1 < 0 : y1_1 > 0;
        if (y1_rev)
            /* intensity change reversal - finalize previous edge */
            edge = process_edge(scn, y1_1);

        if (y1_rev || (abs(scn->y1_sign) < abs(y1_1))) {
            int d;
            scn->y1_sign = y1_1;

            /* adaptive thresholding */
            /* start at multiple of new min/max */
            scn->y1_thresh = (abs(y1_1) * THRESH_INIT + ROUND) >> ZBAR_FIXED;
            dbprintf(1, "\tthr=%d", scn->y1_thresh);
            if (scn->y1_thresh < scn->y1_min_thresh)
                scn->y1_thresh = scn->y1_min_thresh;

            /* update current edge */
            d = y2_1 - y2_2;
            scn->cur_edge = 1 << ZBAR_FIXED;
            if (!d)
                scn->cur_edge >>= 1;
            else if (y2_1)
                /* interpolate zero crossing */
                scn->cur_edge -= ((y2_1 << ZBAR_FIXED) + 1) / d;
            scn->cur_edge += x << ZBAR_FIXED;
            dbprintf(1, "\n");
        }
    }
    else
        dbprintf(1, "\n");
    /* FIXME add fall-thru pass to decoder after heuristic "idle" period
       (eg, 6-8 * last width) */
    scn->x = x + 1;
    return(edge);
}
