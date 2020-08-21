/*Copyright (C) 1991-1995  Henry Minsky (hqm@ua.com, hqm@ai.mit.edu)
  Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <stdlib.h>
#include <string.h>
#include "rs.h"


/*Reed-Solomon encoder and decoder.
  Original implementation (C) Henry Minsky (hqm@ua.com, hqm@ai.mit.edu),
   Universal Access 1991-1995.
  Updates by Timothy B. Terriberry (C) 2008-2009:
   - Properly reject codes when error-locator polynomial has repeated roots or
      non-trivial irreducible factors.
   - Removed the hard-coded parity size and performed general API cleanup.
   - Allow multiple representations of GF(2**8), since different standards use
      different irreducible polynomials.
   - Allow different starting indices for the generator polynomial, since
      different standards use different values.
   - Greatly reduced the computation by eliminating unnecessary operations.
   - Explicitly solve for the roots of low-degree polynomials instead of using
      an exhaustive search.
     This is another major speed boost when there are few errors.*/


     /*Galois Field arithmetic in GF(2**8).*/

void rs_gf256_init(rs_gf256* _gf, unsigned _ppoly) {
    unsigned p;
    int      i;
    /*Initialize the table of powers of a primtive root, alpha=0x02.*/
    p = 1;
    for (i = 0; i < 256; i++) {
          _gf->exp[i] = _gf->exp[i + 255] = p; 
          p = ((p << 1) ^ (-(p >> 7) & _ppoly)) & 0xFF;
    }  
    /*Invert the table to recover the logs.*/
    for (i = 0; i < 255; i++)_gf->log[_gf->exp[i]] = i;
    /*Note that we rely on the fact that _gf->log[0]=0 below.*/
    _gf->log[0] = 0;
}


