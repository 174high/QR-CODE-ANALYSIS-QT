#include <config.h>
#include <stdlib.h>     /* malloc, calloc, free */

#include <zbar.h>

#include "decoder.h"

zbar_decoder_t* zbar_decoder_create()
{
	zbar_decoder_t* dcode = calloc(1, sizeof(zbar_decoder_t));
	dcode->buf_alloc = BUFFER_MIN;
	dcode->buf = malloc(dcode->buf_alloc);

#ifdef ENABLE_QRCODE
	dcode->qrf.config = 1 << ZBAR_CFG_ENABLE;
#endif

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

