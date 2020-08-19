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





