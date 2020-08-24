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
#include <stdint.h>
#include <zbar.h>


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

typedef enum errsev_e {
    SEV_FATAL = -2,           /* application must terminate */
    SEV_ERROR = -1,           /* might be able to recover and continue */
    SEV_OK = 0,
    SEV_WARNING = 1,           /* unexpected condition */
    SEV_NOTE = 2,           /* fyi */
} errsev_t;


typedef enum errmodule_e {
    ZBAR_MOD_PROCESSOR,
    ZBAR_MOD_VIDEO,
    ZBAR_MOD_WINDOW,
    ZBAR_MOD_IMAGE_SCANNER,
    ZBAR_MOD_UNKNOWN,
} errmodule_t;

/** error codes. */
typedef enum zbar_error_e {
    ZBAR_OK = 0,                /**< no error */
    ZBAR_ERR_NOMEM,             /**< out of memory */
    ZBAR_ERR_INTERNAL,          /**< internal library error */
    ZBAR_ERR_UNSUPPORTED,       /**< unsupported request */
    ZBAR_ERR_INVALID,           /**< invalid request */
    ZBAR_ERR_SYSTEM,            /**< system error */
    ZBAR_ERR_LOCKING,           /**< locking error */
    ZBAR_ERR_BUSY,              /**< all resources busy */
    ZBAR_ERR_XDISPLAY,          /**< X11 display error */
    ZBAR_ERR_XPROTO,            /**< X11 protocol error */
    ZBAR_ERR_CLOSED,            /**< output window is closed */
    ZBAR_ERR_WINAPI,            /**< windows system error */
    ZBAR_ERR_NUM                /**< number of error codes */
} zbar_error_t;


typedef struct errinfo_s {
    uint32_t magic;             /* just in case */
    errmodule_t module;         /* reporting module */
    char* buf;                  /* formatted and passed to application */
    int errnum;                 /* errno for system errors */

    errsev_t sev;
    zbar_error_t type;
    const char* func;           /* reporting function */
    const char* detail;         /* description */
    char* arg_str;              /* single string argument */
    int arg_int;                /* single integer argument */
} errinfo_t;






#endif