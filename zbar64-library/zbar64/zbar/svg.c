/*------------------------------------------------------------------------
 *  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "svg.h"

static const char svg_head[] =
"<?xml version='1.0'?>\n"
"<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN'"
" 'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>\n"
"<svg version='1.1' id='top' width='8in' height='8in'"
" preserveAspectRatio='xMidYMid' overflow='visible'"
" viewBox='%g,%g %g,%g' xmlns:xlink='http://www.w3.org/1999/xlink'"
" xmlns='http://www.w3.org/2000/svg'>\n"
"<defs><style type='text/css'><![CDATA[\n"
"* { image-rendering: optimizeSpeed }\n"
"image { opacity: .9 }\n"
"path, line, circle { fill: none; stroke-width: .5;"
" stroke-linejoin: round; stroke-linecap: butt;"
" stroke-opacity: .666; fill-opacity: .666 }\n"
/*"#hedge line, #vedge line { stroke: #34f }\n"*/
"#hedge, #vedge { stroke: yellow }\n"
"#target * { fill: none; stroke: #f24 }\n"
"#mdot * { fill: #e2f; stroke: none }\n"
"#ydot * { fill: yellow; stroke: none }\n"
"#cross * { stroke: #44f }\n"
".hedge { marker: url(#hedge) }\n"
".vedge { marker: url(#vedge) }\n"
".scanner .hedge, .scanner .vedge { stroke-width: 8 }\n"
".finder .hedge, .finder .vedge { /*stroke: #a0c;*/ stroke-width: .9 }\n"
".cluster { stroke: #a0c; stroke-width: 1; stroke-opacity: .45 }\n"
".cluster.valid { stroke: #c0a; stroke-width: 1; stroke-opacity: .666 }\n"
".h.cluster { marker: url(#vedge) }\n"
".v.cluster { marker: url(#hedge) }\n"
".centers { marker: url(#target); stroke-width: 3 }\n"
".edge-pts { marker: url(#ydot); stroke-width: .5 }\n"
".align { marker: url(#mdot); stroke-width: 1.5 }\n"
".sampling-grid { stroke-width: .75; marker: url(#cross) }\n"
"]]></style>\n"
"<marker id='hedge' overflow='visible'><line x1='-2' x2='2'/></marker>\n"
"<marker id='vedge' overflow='visible'><line y1='-2' y2='2'/></marker>\n"
"<marker id='ydot' overflow='visible'><circle r='2'/></marker>\n"
"<marker id='mdot' overflow='visible'><circle r='2'/></marker>\n"
"<marker id='cross' overflow='visible'><path d='M-2,0h4 M0,-2v4'/></marker>\n"
"<marker id='target' overflow='visible'><path d='M-4,0h8 M0,-4v8'/><circle r='2'/></marker>\n"
"</defs>\n";

static FILE* svg = NULL;


