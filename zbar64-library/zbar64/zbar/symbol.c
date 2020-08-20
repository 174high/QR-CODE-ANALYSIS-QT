
#include <zbar.h>
#include "symbol.h"


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

