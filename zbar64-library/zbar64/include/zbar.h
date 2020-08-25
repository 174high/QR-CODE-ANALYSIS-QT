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

 /** decoded symbol type. */
typedef enum zbar_symbol_type_e {
    ZBAR_NONE = 0,  /**< no symbol decoded */
    ZBAR_PARTIAL = 1,  /**< intermediate status */
    ZBAR_EAN2 = 2,  /**< GS1 2-digit add-on */
    ZBAR_EAN5 = 5,  /**< GS1 5-digit add-on */
    ZBAR_EAN8 = 8,  /**< EAN-8 */
    ZBAR_UPCE = 9,  /**< UPC-E */
    ZBAR_ISBN10 = 10,  /**< ISBN-10 (from EAN-13). @since 0.4 */
    ZBAR_UPCA = 12,  /**< UPC-A */
    ZBAR_EAN13 = 13,  /**< EAN-13 */
    ZBAR_ISBN13 = 14,  /**< ISBN-13 (from EAN-13). @since 0.4 */
    ZBAR_COMPOSITE = 15,  /**< EAN/UPC composite */
    ZBAR_I25 = 25,  /**< Interleaved 2 of 5. @since 0.4 */
    ZBAR_DATABAR = 34,  /**< GS1 DataBar (RSS). @since 0.11 */
    ZBAR_DATABAR_EXP = 35,  /**< GS1 DataBar Expanded. @since 0.11 */
    ZBAR_CODABAR = 38,  /**< Codabar. @since 0.11 */
    ZBAR_CODE39 = 39,  /**< Code 39. @since 0.4 */
    ZBAR_PDF417 = 57,  /**< PDF417. @since 0.6 */
    ZBAR_QRCODE = 64,  /**< QR Code. @since 0.10 */
    ZBAR_CODE93 = 93,  /**< Code 93. @since 0.11 */
    ZBAR_CODE128 = 128,  /**< Code 128 */

    /** mask for base symbol type.
     * @deprecated in 0.11, remove this from existing code
     */
     ZBAR_SYMBOL = 0x00ff,
     /** 2-digit add-on flag.
      * @deprecated in 0.11, a ::ZBAR_EAN2 component is used for
      * 2-digit GS1 add-ons
      */
      ZBAR_ADDON2 = 0x0200,
      /** 5-digit add-on flag.
       * @deprecated in 0.11, a ::ZBAR_EAN5 component is used for
       * 5-digit GS1 add-ons
       */
       ZBAR_ADDON5 = 0x0500,
       /** add-on flag mask.
        * @deprecated in 0.11, GS1 add-ons are represented using composite
        * symbols of type ::ZBAR_COMPOSITE; add-on components use ::ZBAR_EAN2
        * or ::ZBAR_EAN5
        */
        ZBAR_ADDON = 0x0700,
} zbar_symbol_type_t;

/** "color" of element: bar or space. */
typedef enum zbar_color_e {
    ZBAR_SPACE = 0,    /**< light area or space between bars */
    ZBAR_BAR = 1,      /**< dark area or colored bar segment */
} zbar_color_t;

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

/** decoded symbol coarse orientation.
 * @since 0.11
 */
typedef enum zbar_orientation_e {
    ZBAR_ORIENT_UNKNOWN = -1,   /**< unable to determine orientation */
    ZBAR_ORIENT_UP,             /**< upright, read left to right */
    ZBAR_ORIENT_RIGHT,          /**< sideways, read top to bottom */
    ZBAR_ORIENT_DOWN,           /**< upside-down, read right to left */
    ZBAR_ORIENT_LEFT,           /**< sideways, read bottom to top */
} zbar_orientation_t;

/*@}*/

struct zbar_symbol_s;
typedef struct zbar_symbol_s zbar_symbol_t;

struct zbar_symbol_set_s;
typedef struct zbar_symbol_set_s zbar_symbol_set_t;

/** set iterator.
 * @returns the first decoded symbol result in a set
 * @returns NULL if the set is empty
 * @since 0.10
 */
extern const zbar_symbol_t*
zbar_symbol_set_first_symbol(const zbar_symbol_set_t* symbols);


/** consistently compute fourcc values across architectures
 * (adapted from v4l2 specification)
 * @since 0.11
 */
#define zbar_fourcc(a, b, c, d)                 \
        ((unsigned long)(a) |                   \
         ((unsigned long)(b) << 8) |            \
         ((unsigned long)(c) << 16) |           \
         ((unsigned long)(d) << 24))


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

/** destructor. */
extern void zbar_image_scanner_destroy(zbar_image_scanner_t* scanner);

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

/** decoder data handler callback function.
 * called by decoder when new data has just been decoded
 */
typedef void (zbar_decoder_handler_t)(zbar_decoder_t* decoder);

/** constructor. */
extern zbar_decoder_t* zbar_decoder_create(void);

/** destructor. */
extern void zbar_decoder_destroy(zbar_decoder_t* decoder);

/*------------------------------------------------------------*/
/** @name Scanner interface
 * @anchor c-scanner
 * low-level linear intensity sample stream scanner interface.
 * identifies "bar" edges and measures width between them.
 * optionally passes to bar width decoder
 */
 /*@{*/

struct zbar_scanner_s;
/** opaque scanner object. */
typedef struct zbar_scanner_s zbar_scanner_t;

/*------------------------------------------------------------*/
/** @name Image interface
 * stores image data samples along with associated format and size
 * metadata
 */
 /*@{*/

struct zbar_image_s;
/** opaque image object. */
typedef struct zbar_image_s zbar_image_t;

/** decoder symbology modifier flags.
 * @since 0.11
 */
typedef enum zbar_modifier_e {
    /** barcode tagged as GS1 (EAN.UCC) reserved
     * (eg, FNC1 before first data character).
     * data may be parsed as a sequence of GS1 AIs
     */
    ZBAR_MOD_GS1 = 0,

    /** barcode tagged as AIM reserved
     * (eg, FNC1 after first character or digit pair)
     */
     ZBAR_MOD_AIM,

     /** number of modifiers */
     ZBAR_MOD_NUM,
} zbar_modifier_t;

/** constructor.
 * if decoder is non-NULL it will be attached to scanner
 * and called automatically at each new edge
 * current color is initialized to ::ZBAR_SPACE
 * (so an initial BAR->SPACE transition may be discarded)
 */
extern zbar_scanner_t* zbar_scanner_create(zbar_decoder_t* decoder);

/** destructor. */
extern void zbar_scanner_destroy(zbar_scanner_t* scanner);

/** clear all scanner state.
 * also resets an associated decoder
 */
extern zbar_symbol_type_t zbar_scanner_reset(zbar_scanner_t* scanner);

/** reference count manipulation.
 * increment the reference count when you store a new reference.
 * decrement when the reference is no longer used.  do not refer to
 * the object any longer once references have been released.
 * @since 0.10
 */
extern void zbar_symbol_set_ref(const zbar_symbol_set_t* symbols,
    int refs);

/** clear all decoder state.
 * any partial symbols are flushed
 */
extern void zbar_decoder_reset(zbar_decoder_t* decoder);

/** setup data handler callback.
 * the registered function will be called by the decoder
 * just before zbar_decode_width() returns a non-zero value.
 * pass a NULL value to disable callbacks.
 * @returns the previously registered handler
 */
extern zbar_decoder_handler_t*
zbar_decoder_set_handler(zbar_decoder_t* decoder,
    zbar_decoder_handler_t* handler);

/** associate user specified data value with the decoder. */
extern void zbar_decoder_set_userdata(zbar_decoder_t* decoder,
    void* userdata);

/** return user specified data value associated with the decoder. */
extern void *zbar_decoder_get_userdata(const zbar_decoder_t *decoder);


/** cleanup handler callback function.
 * called to free sample data when an image is destroyed.
 */
typedef void (zbar_image_cleanup_handler_t)(zbar_image_t* image);

/** data handler callback function.
 * called when decoded symbol results are available for an image
 */
typedef void (zbar_image_data_handler_t)(zbar_image_t* image,
    const void* userdata);


/** retrieve last decoded symbol type.
 * @returns the type or ::ZBAR_NONE if no new data available
 */
extern zbar_symbol_type_t
zbar_decoder_get_type(const zbar_decoder_t* decoder);

/** retrieve last scanned width. */
extern unsigned zbar_scanner_get_width(const zbar_scanner_t* scanner);


/** specify image sample data.  when image data is no longer needed by
 * the library the specific data cleanup handler will be called
 * (unless NULL)
 * @note application image data will not be modified by the library
 */
extern void zbar_image_set_data(zbar_image_t* image,
    const void* data,
    unsigned long data_byte_length,
    zbar_image_cleanup_handler_t* cleanup_hndlr);

/** built-in cleanup handler.
 * passes the image data buffer to free()
 */
extern void zbar_image_free_data(zbar_image_t* image);

/** retrieve sample position of last edge.
 * @since 0.10
 */
extern unsigned zbar_scanner_get_edge(const zbar_scanner_t* scn,
    unsigned offset,
    int prec);
/** retrieve last decoded data.
 * @returns the data string or NULL if no new data available.
 * the returned data buffer is owned by library, contents are only
 * valid between non-0 return from zbar_decode_width and next library
 * call
 */
extern const char* zbar_decoder_get_data(const zbar_decoder_t* decoder);

/** retrieve length of binary data.
 * @returns the length of the decoded data or 0 if no new data
 * available.
 */
extern unsigned int
zbar_decoder_get_data_length(const zbar_decoder_t* decoder);

/** retrieve string name for symbol encoding.
 * @param sym symbol type encoding
 * @returns the static string name for the specified symbol type,
 * or "UNKNOWN" if the encoding is not recognized
 */
extern const char* zbar_get_symbol_name(zbar_symbol_type_t sym);

/** retrieve symbology boolean config settings.
 * @returns a bitmask indicating which configs are currently set for the
 * specified symbology.
 * @since 0.11
 */
extern unsigned int zbar_decoder_get_configs(const zbar_decoder_t* decoder,
    zbar_symbol_type_t symbology);

/** retrieve modifier flags for the last decoded symbol.
 * @returns a bitmask indicating which characteristics were detected
 * during decoding.
 * @since 0.11
 */
extern unsigned int zbar_decoder_get_modifiers(const zbar_decoder_t* decoder);

/** retrieve last decode direction.
 * @returns 1 for forward and -1 for reverse
 * @returns 0 if the decode direction is unknown or does not apply
 * @since 0.11
 */
extern int zbar_decoder_get_direction(const zbar_decoder_t* decoder);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @see zbar_decoder_set_config()
 * @since 0.4
 */
extern int zbar_image_scanner_set_config(zbar_image_scanner_t* scanner,
    zbar_symbol_type_t symbology,
    zbar_config_t config,
    int value);

/** set config for indicated symbology (0 for all) to specified value.
 * @returns 0 for success, non-0 for failure (config does not apply to
 * specified symbology, or value out of range)
 * @since 0.4
 */
extern int zbar_decoder_set_config(zbar_decoder_t* decoder,
    zbar_symbol_type_t symbology,
    zbar_config_t config,
    int value);

/*@}*/


/*------------------------------------------------------------*/
/** @name Video interface
 * @anchor c-video
 * mid-level video source abstraction.
 * captures images from a video device
 */
 /*@{*/

struct zbar_video_s;
/** opaque video object. */
typedef struct zbar_video_s zbar_video_t;

/** constructor. */
extern zbar_video_t* zbar_video_create(void);

/** destructor. */
extern void zbar_video_destroy(zbar_video_t* video);

/** mark start of a new scan pass. resets color to ::ZBAR_SPACE.
 * also updates an associated decoder.
 * @returns any decode results flushed from the pipeline
 * @note when not using callback handlers, the return value should
 * be checked the same as zbar_scan_y()
 * @note call zbar_scanner_flush() at least twice before calling this
 * method to ensure no decode results are lost
 */
extern zbar_symbol_type_t zbar_scanner_new_scan(zbar_scanner_t* scanner);

/** flush scanner processing pipeline.
 * forces current scanner position to be a scan boundary.
 * call multiple times (max 3) to completely flush decoder.
 * @returns any decode/scan results flushed from the pipeline
 * @note when not using callback handlers, the return value should
 * be checked the same as zbar_scan_y()
 * @since 0.9
 */
extern zbar_symbol_type_t zbar_scanner_flush(zbar_scanner_t* scanner);

/** mark start of a new scan pass.
 * clears any intra-symbol state and resets color to ::ZBAR_SPACE.
 * any partially decoded symbol state is retained
 */
extern void zbar_decoder_new_scan(zbar_decoder_t* decoder);

/** process next sample intensity value.
 * intensity (y) is in arbitrary relative units.
 * @returns result of zbar_decode_width() if a decoder is attached,
 * otherwise @returns (::ZBAR_PARTIAL) when new edge is detected
 * or 0 (::ZBAR_NONE) if no new edge is detected
 */
extern zbar_symbol_type_t zbar_scan_y(zbar_scanner_t* scanner,
    int y);

#endif