#include <config.h>
#include <stdlib.h>     /* malloc, calloc, free */

#include <zbar.h>

#include "decoder.h"

zbar_decoder_t* zbar_decoder_create()
{
    zbar_decoder_t* dcode = calloc(1, sizeof(zbar_decoder_t));
    dcode->buf_alloc = BUFFER_MIN;
    dcode->buf = malloc(dcode->buf_alloc);

    /* initialize default configs */
#ifdef ENABLE_EAN
    dcode->ean.enable = 1;
    dcode->ean.ean13_config = ((1 << ZBAR_CFG_ENABLE) |
        (1 << ZBAR_CFG_EMIT_CHECK));
    dcode->ean.ean8_config = ((1 << ZBAR_CFG_ENABLE) |
        (1 << ZBAR_CFG_EMIT_CHECK));
    dcode->ean.upca_config = 1 << ZBAR_CFG_EMIT_CHECK;
    dcode->ean.upce_config = 1 << ZBAR_CFG_EMIT_CHECK;
    dcode->ean.isbn10_config = 1 << ZBAR_CFG_EMIT_CHECK;
    dcode->ean.isbn13_config = 1 << ZBAR_CFG_EMIT_CHECK;
# ifdef FIXME_ADDON_SYNC
    dcode->ean.ean2_config = 1 << ZBAR_CFG_ENABLE;
    dcode->ean.ean5_config = 1 << ZBAR_CFG_ENABLE;
# endif
#endif
#ifdef ENABLE_I25
    dcode->i25.config = 1 << ZBAR_CFG_ENABLE;
    CFG(dcode->i25, ZBAR_CFG_MIN_LEN) = 6;
#endif
#ifdef ENABLE_DATABAR
    dcode->databar.config = ((1 << ZBAR_CFG_ENABLE) |
        (1 << ZBAR_CFG_EMIT_CHECK));
    dcode->databar.config_exp = ((1 << ZBAR_CFG_ENABLE) |
        (1 << ZBAR_CFG_EMIT_CHECK));
    dcode->databar.csegs = 4;
    dcode->databar.segs = calloc(4, sizeof(*dcode->databar.segs));
#endif
#ifdef ENABLE_CODABAR
    dcode->codabar.config = 1 << ZBAR_CFG_ENABLE;
    CFG(dcode->codabar, ZBAR_CFG_MIN_LEN) = 4;
#endif
#ifdef ENABLE_CODE39
    dcode->code39.config = 1 << ZBAR_CFG_ENABLE;
    CFG(dcode->code39, ZBAR_CFG_MIN_LEN) = 1;
#endif
#ifdef ENABLE_CODE93
    dcode->code93.config = 1 << ZBAR_CFG_ENABLE;
#endif
#ifdef ENABLE_CODE128
    dcode->code128.config = 1 << ZBAR_CFG_ENABLE;
#endif
#ifdef ENABLE_PDF417
    dcode->pdf417.config = 1 << ZBAR_CFG_ENABLE;
#endif
#ifdef ENABLE_QRCODE
    dcode->qrf.config = 1 << ZBAR_CFG_ENABLE;
#endif

    zbar_decoder_reset(dcode);
    return(dcode);
}

void zbar_decoder_destroy(zbar_decoder_t* dcode)
{
#ifdef ENABLE_DATABAR
	if (dcode->databar.segs)
		free(dcode->databar.segs);
#endif
	if (dcode->buf)
		free(dcode->buf);
	free(dcode);
}


void zbar_decoder_reset(zbar_decoder_t* dcode)
{
	memset(dcode, 0, (long)&dcode->buf_alloc - (long)dcode);
#ifdef ENABLE_EAN
	ean_reset(&dcode->ean);
#endif
#ifdef ENABLE_I25
	i25_reset(&dcode->i25);
#endif
#ifdef ENABLE_DATABAR
	databar_reset(&dcode->databar);
#endif
#ifdef ENABLE_CODABAR
	codabar_reset(&dcode->codabar);
#endif
#ifdef ENABLE_CODE39
	code39_reset(&dcode->code39);
#endif
#ifdef ENABLE_CODE93
	code93_reset(&dcode->code93);
#endif
#ifdef ENABLE_CODE128
	code128_reset(&dcode->code128);
#endif
#ifdef ENABLE_PDF417
	pdf417_reset(&dcode->pdf417);
#endif
#ifdef ENABLE_QRCODE
	qr_finder_reset(&dcode->qrf);
#endif
}

void zbar_decoder_set_userdata(zbar_decoder_t* dcode,
	void* userdata)
{
	dcode->userdata = userdata;
}

zbar_decoder_handler_t*
zbar_decoder_set_handler(zbar_decoder_t* dcode,
	zbar_decoder_handler_t* handler)
{
	zbar_decoder_handler_t* result = dcode->handler;
	dcode->handler = handler;
	return(result);
}

void* zbar_decoder_get_userdata(const zbar_decoder_t* dcode)
{
	return(dcode->userdata);
}

zbar_symbol_type_t zbar_decoder_get_type(const zbar_decoder_t* dcode)
{
	return(dcode->type);
}

const char* zbar_decoder_get_data(const zbar_decoder_t* dcode)
{
	return((char*)dcode->buf);
}

unsigned int zbar_decoder_get_data_length(const zbar_decoder_t* dcode)
{
	return(dcode->buflen);
}

static __inline const unsigned int*
decoder_get_configp(const zbar_decoder_t* dcode,
    zbar_symbol_type_t sym)
{
    const unsigned int* config;
    switch (sym) {
#ifdef ENABLE_EAN
    case ZBAR_EAN13:
        config = &dcode->ean.ean13_config;
        break;

    case ZBAR_EAN2:
        config = &dcode->ean.ean2_config;
        break;

    case ZBAR_EAN5:
        config = &dcode->ean.ean5_config;
        break;

    case ZBAR_EAN8:
        config = &dcode->ean.ean8_config;
        break;

    case ZBAR_UPCA:
        config = &dcode->ean.upca_config;
        break;

    case ZBAR_UPCE:
        config = &dcode->ean.upce_config;
        break;

    case ZBAR_ISBN10:
        config = &dcode->ean.isbn10_config;
        break;

    case ZBAR_ISBN13:
        config = &dcode->ean.isbn13_config;
        break;
#endif

#ifdef ENABLE_I25
    case ZBAR_I25:
        config = &dcode->i25.config;
        break;
#endif

#ifdef ENABLE_DATABAR
    case ZBAR_DATABAR:
        config = &dcode->databar.config;
        break;
    case ZBAR_DATABAR_EXP:
        config = &dcode->databar.config_exp;
        break;
#endif

#ifdef ENABLE_CODABAR
    case ZBAR_CODABAR:
        config = &dcode->codabar.config;
        break;
#endif

#ifdef ENABLE_CODE39
    case ZBAR_CODE39:
        config = &dcode->code39.config;
        break;
#endif

#ifdef ENABLE_CODE93
    case ZBAR_CODE93:
        config = &dcode->code93.config;
        break;
#endif

#ifdef ENABLE_CODE128
    case ZBAR_CODE128:
        config = &dcode->code128.config;
        break;
#endif

#ifdef ENABLE_PDF417
    case ZBAR_PDF417:
        config = &dcode->pdf417.config;
        break;
#endif

#ifdef ENABLE_QRCODE
    case ZBAR_QRCODE:
        config = &dcode->qrf.config;
        break;
#endif

    default:
        config = NULL;
    }
    return(config);
}

void zbar_decoder_new_scan(zbar_decoder_t* dcode)
{
    /* soft reset decoder */
    memset(dcode->w, 0, sizeof(dcode->w));
    dcode->lock = 0;
    dcode->idx = 0;
    dcode->s6 = 0;
#ifdef ENABLE_EAN
    ean_new_scan(&dcode->ean);
#endif
#ifdef ENABLE_I25
    i25_reset(&dcode->i25);
#endif
#ifdef ENABLE_DATABAR
    databar_new_scan(&dcode->databar);
#endif
#ifdef ENABLE_CODABAR
    codabar_reset(&dcode->codabar);
#endif
#ifdef ENABLE_CODE39
    code39_reset(&dcode->code39);
#endif
#ifdef ENABLE_CODE93
    code93_reset(&dcode->code93);
#endif
#ifdef ENABLE_CODE128
    code128_reset(&dcode->code128);
#endif
#ifdef ENABLE_PDF417
    pdf417_reset(&dcode->pdf417);
#endif
#ifdef ENABLE_QRCODE
    qr_finder_reset(&dcode->qrf);
#endif
}

unsigned int zbar_decoder_get_configs(const zbar_decoder_t* dcode,
	zbar_symbol_type_t sym)
{
	const unsigned* config = decoder_get_configp(dcode, sym);
	if (!config)
		return(0);
	return(*config);
}

unsigned int zbar_decoder_get_modifiers(const zbar_decoder_t* dcode)
{
    return(dcode->modifiers);
}

int zbar_decoder_get_direction(const zbar_decoder_t* dcode)
{
    return(dcode->direction);
}

static __inline int decoder_set_config_bool(zbar_decoder_t* dcode,
    zbar_symbol_type_t sym,
    zbar_config_t cfg,
    int val)
{
    unsigned* config = (void*)decoder_get_configp(dcode, sym);
    if (!config || cfg >= ZBAR_CFG_NUM)
        return(1);

    if (!val)
        *config &= ~(1 << cfg);
    else if (val == 1)
        *config |= (1 << cfg);
    else
        return(1);

#ifdef ENABLE_EAN
    dcode->ean.enable = TEST_CFG(dcode->ean.ean13_config |
        dcode->ean.ean2_config |
        dcode->ean.ean5_config |
        dcode->ean.ean8_config |
        dcode->ean.upca_config |
        dcode->ean.upce_config |
        dcode->ean.isbn10_config |
        dcode->ean.isbn13_config,
        ZBAR_CFG_ENABLE);
#endif

    return(0);
}

static __inline int decoder_set_config_int(zbar_decoder_t* dcode,
    zbar_symbol_type_t sym,
    zbar_config_t cfg,
    int val)
{
    switch (sym) {

#ifdef ENABLE_I25
    case ZBAR_I25:
        CFG(dcode->i25, cfg) = val;
        break;
#endif
#ifdef ENABLE_CODABAR
    case ZBAR_CODABAR:
        CFG(dcode->codabar, cfg) = val;
        break;
#endif
#ifdef ENABLE_CODE39
    case ZBAR_CODE39:
        CFG(dcode->code39, cfg) = val;
        break;
#endif
#ifdef ENABLE_CODE93
    case ZBAR_CODE93:
        CFG(dcode->code93, cfg) = val;
        break;
#endif
#ifdef ENABLE_CODE128
    case ZBAR_CODE128:
        CFG(dcode->code128, cfg) = val;
        break;
#endif
#ifdef ENABLE_PDF417
    case ZBAR_PDF417:
        CFG(dcode->pdf417, cfg) = val;
        break;
#endif

    default:
        return(1);
    }
    return(0);
}


int zbar_decoder_set_config(zbar_decoder_t* dcode,
    zbar_symbol_type_t sym,
    zbar_config_t cfg,
    int val)
{
    if (sym == ZBAR_NONE) {
        static const zbar_symbol_type_t all[] = {
        ZBAR_EAN13, ZBAR_EAN2, ZBAR_EAN5, ZBAR_EAN8,
            ZBAR_UPCA, ZBAR_UPCE, ZBAR_ISBN10, ZBAR_ISBN13,
            ZBAR_I25, ZBAR_DATABAR, ZBAR_DATABAR_EXP, ZBAR_CODABAR,
        ZBAR_CODE39, ZBAR_CODE93, ZBAR_CODE128, ZBAR_QRCODE,
        ZBAR_PDF417, 0
        };
        const zbar_symbol_type_t* symp;
        for (symp = all; *symp; symp++)
            zbar_decoder_set_config(dcode, *symp, cfg, val);
        return(0);
    }
    
    if (cfg >= 0 && cfg < ZBAR_CFG_NUM)
        return(decoder_set_config_bool(dcode, sym, cfg, val));
    else if (cfg >= ZBAR_CFG_MIN_LEN && cfg <= ZBAR_CFG_MAX_LEN)
        return(decoder_set_config_int(dcode, sym, cfg, val));
    else
        return(1);
       
}