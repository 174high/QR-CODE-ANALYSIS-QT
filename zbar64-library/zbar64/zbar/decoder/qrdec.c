/*Copyright (C) 2008-2009  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <config.h>
#include "qrcode.h"
#include "error.h"


/* collection of finder lines */
typedef struct qr_finder_lines {
    qr_finder_line* lines;
    int nlines, clines;
} qr_finder_lines;


struct qr_reader {
    /*The GF(256) representation used in Reed-Solomon decoding.*/
 //   rs_gf256  gf;
    /*The random number generator used by RANSAC.*/
 //   isaac_ctx isaac;
    /* current finder state, horizontal and vertical lines */
    qr_finder_lines finder_lines[2];
};


/*Frees a client reader handle.*/
void _zbar_qr_destroy(qr_reader* reader)
{
    
    zprintf(1, "max finder lines = %dx%d\n",
        reader->finder_lines[0].clines,
        reader->finder_lines[1].clines);
    if (reader->finder_lines[0].lines)
        free(reader->finder_lines[0].lines);
    if (reader->finder_lines[1].lines)
        free(reader->finder_lines[1].lines);
    
    free(reader);
    
}

int _zbar_qr_found_line(qr_reader* reader,
    int dir,
    const qr_finder_line* line)
{
    /* minimally intrusive brute force version */
    qr_finder_lines* lines = &reader->finder_lines[dir];

    if (lines->nlines >= lines->clines) {
        lines->clines *= 2;
        lines->lines = realloc(lines->lines,
            ++lines->clines * sizeof(*lines->lines));
    }

    memcpy(lines->lines + lines->nlines++, line, sizeof(*line));

    return(0);
}
