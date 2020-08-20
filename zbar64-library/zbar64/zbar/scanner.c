#include <config.h>
#include <stdlib.h>     /* malloc, free, abs */
#include <stddef.h>
#include <string.h>     /* memset */

#include <zbar.h>

#ifndef ZBAR_FIXED
# define ZBAR_FIXED 5
#endif
#define ROUND (1 << (ZBAR_FIXED - 1))


/* FIXME add runtime config API for these */
#ifndef ZBAR_SCANNER_THRESH_MIN
# define ZBAR_SCANNER_THRESH_MIN  4
#endif


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


unsigned zbar_scanner_get_width(const zbar_scanner_t* scn)
{
    return(scn->width);
}
