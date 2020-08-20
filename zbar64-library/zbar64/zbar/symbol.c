#include "symbol.h"


void zbar_symbol_set_ref(const zbar_symbol_set_t* syms,
    int delta)
{
    zbar_symbol_set_t* ncsyms = (zbar_symbol_set_t*)syms;
 //   if (!_zbar_refcnt(&ncsyms->refcnt, delta) && delta <= 0)
 //       _zbar_symbol_set_free(ncsyms);
}
