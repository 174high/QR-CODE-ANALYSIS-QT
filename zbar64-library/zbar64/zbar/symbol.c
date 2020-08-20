
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

