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
#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdio.h>
#include <stdlib.h>

extern int _zbar_verbosity;

#ifdef _WIN32
# define ZFLUSH fflush(stderr);
#else
# define ZFLUSH
#endif

#ifdef ZNO_MESSAGES

# ifdef __GNUC__
/* older versions of gcc (< 2.95) require a named varargs parameter */
#  define zprintf(args...)
# else
/* unfortunately named vararg parameter is a gcc-specific extension */
#  define zprintf(...)
# endif

#else

# ifdef __GNUC__
#  define zprintf(level, format, args...) do {                          \
        if(_zbar_verbosity >= level) {                                  \
            fprintf(stderr, "%s: " format, __func__ , ##args);          \
            ZFLUSH                                                      \
        }                                                               \
    } while(0)
# else
#  define zprintf(level, format, ...) do {                              \
        if(_zbar_verbosity >= level) {                                  \
            fprintf(stderr, "%s: " format, __func__ , ##__VA_ARGS__);   \
            ZFLUSH                                                      \
        }                                                               \
    } while(0)
# endif

#endif










#endif