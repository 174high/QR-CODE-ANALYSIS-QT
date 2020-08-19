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
#ifndef _ZBAR_H_
#define _ZBAR_H_

 /** decoder configuration options.
  * @since 0.4
  */
typedef enum zbar_config_e {
    ZBAR_CFG_ENABLE = 0,        /**< enable symbology/feature */
    ZBAR_CFG_ADD_CHECK,         /**< enable check digit when optional */
    ZBAR_CFG_EMIT_CHECK,        /**< return check digit when present */
    ZBAR_CFG_ASCII,             /**< enable full ASCII character set */
    ZBAR_CFG_NUM,               /**< number of boolean decoder configs */

    ZBAR_CFG_MIN_LEN = 0x20,    /**< minimum data length for valid decode */
    ZBAR_CFG_MAX_LEN,           /**< maximum data length for valid decode */

    ZBAR_CFG_UNCERTAINTY = 0x40,/**< required video consistency frames */

    ZBAR_CFG_POSITION = 0x80,   /**< enable scanner to collect position data */

    ZBAR_CFG_X_DENSITY = 0x100, /**< image scanner vertical scan density */
    ZBAR_CFG_Y_DENSITY,         /**< image scanner horizontal scan density */
} zbar_config_t;

/*------------------------------------------------------------*/
/** @name Image Scanner interface
 * @anchor c-imagescanner
 * mid-level image scanner interface.
 * reads barcodes from 2-D images
 */
 /*@{*/

struct zbar_image_scanner_s;
/** opaque image scanner object. */
typedef struct zbar_image_scanner_s zbar_image_scanner_t;

/** constructor. */
extern zbar_image_scanner_t* zbar_image_scanner_create(void);



/*------------------------------------------------------------*/
/** @name Decoder interface
 * @anchor c-decoder
 * low-level bar width stream decoder interface.
 * identifies symbols and extracts encoded data
 */
 /*@{*/

struct zbar_decoder_s;
/** opaque decoder object. */
typedef struct zbar_decoder_s zbar_decoder_t;

/** constructor. */
extern zbar_decoder_t* zbar_decoder_create(void);

#endif