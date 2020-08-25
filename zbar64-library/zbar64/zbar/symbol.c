
#include <zbar.h>
#include "symbol.h"


const char* zbar_get_symbol_name(zbar_symbol_type_t sym)
{
    switch (sym & ZBAR_SYMBOL) {
    case ZBAR_EAN2: return("EAN-2");
    case ZBAR_EAN5: return("EAN-5");
    case ZBAR_EAN8: return("EAN-8");
    case ZBAR_UPCE: return("UPC-E");
    case ZBAR_ISBN10: return("ISBN-10");
    case ZBAR_UPCA: return("UPC-A");
    case ZBAR_EAN13: return("EAN-13");
    case ZBAR_ISBN13: return("ISBN-13");
    case ZBAR_COMPOSITE: return("COMPOSITE");
    case ZBAR_I25: return("I2/5");
    case ZBAR_DATABAR: return("DataBar");
    case ZBAR_DATABAR_EXP: return("DataBar-Exp");
    case ZBAR_CODABAR: return("Codabar");
    case ZBAR_CODE39: return("CODE-39");
    case ZBAR_CODE93: return("CODE-93");
    case ZBAR_CODE128: return("CODE-128");
    case ZBAR_PDF417: return("PDF417");
    case ZBAR_QRCODE: return("QR-Code");
    default: return("UNKNOWN");
    }
}

int _zbar_get_symbol_hash(zbar_symbol_type_t sym)
{
    static const signed char hash[0x20] = {
        0x00, 0x01, 0x10, 0x11,   -1, 0x11, 0x16, 0x0c,
    0x05, 0x06, 0x08,   -1, 0x04, 0x03, 0x07, 0x12,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x02,
      -1, 0x00, 0x12, 0x0c, 0x0b, 0x1d, 0x0a, 0x00,
    };
    int g0 = hash[sym & 0x1f];
    int g1 = hash[~(sym >> 4) & 0x1f];
    assert(g0 >= 0 && g1 >= 0);
    if (g0 < 0 || g1 < 0)
        return(0);
    return((g0 + g1) & 0x1f);
}

void zbar_symbol_set_ref(const zbar_symbol_set_t* syms,
    int delta)
{
    zbar_symbol_set_t* ncsyms = (zbar_symbol_set_t*)syms;
    if (!_zbar_refcnt(&ncsyms->refcnt, delta) && delta <= 0)
        _zbar_symbol_set_free(ncsyms);
}

__inline void _zbar_symbol_set_free(zbar_symbol_set_t* syms)
{
    zbar_symbol_t* sym, * next;
    for (sym = syms->head; sym; sym = next) {
        next = sym->next;
        sym->next = NULL;
        _zbar_symbol_refcnt(sym, -1);
    }
    syms->head = NULL;
    free(syms);
}

void _zbar_symbol_free(zbar_symbol_t* sym)
{
    if (sym->syms) {
        zbar_symbol_set_ref(sym->syms, -1);
        sym->syms = NULL;
    }
    if (sym->pts)
        free(sym->pts);
    if (sym->data_alloc && sym->data)
        free(sym->data);
    free(sym);
}

zbar_symbol_set_t* _zbar_symbol_set_create()
{
    zbar_symbol_set_t* syms = calloc(1, sizeof(*syms));
    _zbar_refcnt(&syms->refcnt, 1);
    return(syms);
}

const char* zbar_symbol_get_data(const zbar_symbol_t* sym)
{
    return(sym->data);
}

const zbar_symbol_t* zbar_symbol_next(const zbar_symbol_t* sym)
{
    return((sym) ? sym->next : NULL);
}

const zbar_symbol_t*
zbar_symbol_set_first_symbol(const zbar_symbol_set_t* syms)
{
    zbar_symbol_t* sym = syms->tail;
    if (sym)
        return(sym->next);
    return(syms->head);
}

unsigned zbar_symbol_get_loc_size(const zbar_symbol_t* sym)
{
    return(sym->npts);
}

int zbar_symbol_get_loc_x(const zbar_symbol_t* sym,
    unsigned idx)
{
    if (idx < sym->npts)
        return(sym->pts[idx].x);
    else
        return(-1);
}

int zbar_symbol_get_loc_y(const zbar_symbol_t* sym,
    unsigned idx)
{
    if (idx < sym->npts)
        return(sym->pts[idx].y);
    else
        return(-1);
}