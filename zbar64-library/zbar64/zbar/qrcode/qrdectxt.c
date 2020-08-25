/*Copyright (C) 2008-2010  Timothy B. Terriberry (tterribe@xiph.org)
  You can redistribute this library and/or modify it under the terms of the
   GNU Lesser General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option) any later
   version.*/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include "qrcode.h"
#include "qrdec.h"
#include "util.h"
#include "image.h"
#include "decoder.h"
#include "error.h"
#include "img_scanner.h"

static int text_is_ascii(const unsigned char* _text, int _len) {
    int i;
    for (i = 0; i < _len; i++)if (_text[i] >= 0x80)return 0;
    return 1;
}

static int text_is_latin1(const unsigned char* _text, int _len) {
    int i;
    for (i = 0; i < _len; i++) {
        /*The following line fails to compile correctly with gcc 3.4.4 on ARM with
           any optimizations enabled.*/
        if (_text[i] >= 0x80 && _text[i] < 0xA0)return 0;
    }
    return 1;
}

static void enc_list_mtf(iconv_t _enc_list[3], iconv_t _enc) {
    int i;
    for (i = 0; i < 3; i++)if (_enc_list[i] == _enc) {
        int j;
        for (j = i; j-- > 0;)_enc_list[j + 1] = _enc_list[j];
        _enc_list[0] = _enc;
        break;
    }
}

int qr_code_data_list_extract_text(const qr_code_data_list* _qrlist,
    zbar_image_scanner_t* iscn,
    zbar_image_t* img)
{
    iconv_t              sjis_cd;
    iconv_t              utf8_cd;
    iconv_t              latin1_cd;
    const qr_code_data* qrdata;
    int                  nqrdata;
    unsigned char* mark;
    int                  ntext;
    int                  i;
    qrdata = _qrlist->qrdata;
    nqrdata = _qrlist->nqrdata;
    mark = (unsigned char*)calloc(nqrdata, sizeof(*mark));
    ntext = 0;
    /*This is the encoding the standard says is the default.*/
    latin1_cd = iconv_open("UTF-8", "ISO8859-1");
    /*But this one is often used, as well.*/
    sjis_cd = iconv_open("UTF-8", "SJIS");
    /*This is a trivial conversion just to check validity without extra code.*/
    utf8_cd = iconv_open("UTF-8", "UTF-8");
    for (i = 0; i < nqrdata; i++)if (!mark[i]) {
        const qr_code_data* qrdataj;
        const qr_code_data_entry* entry;
        iconv_t                   enc_list[3];
        iconv_t                   eci_cd;
        int                       sa[16];
        int                       sa_size;
        char* sa_text;
        size_t                    sa_ntext;
        size_t                    sa_ctext;
        int                       fnc1;
        int                       fnc1_2ai;
        int                       has_kanji;
        int                       eci;
        int                       err;
        int                       j;
        int                       k;
        zbar_symbol_t* syms = NULL, ** sym = &syms;
        qr_point dir;
        int horiz;
    }

    return ntext;
}
