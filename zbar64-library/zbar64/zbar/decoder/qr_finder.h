

/* QR Code symbol finder state */
typedef struct qr_finder_s {
    unsigned s5;                /* finder pattern width */
//    qr_finder_line line;        /* position info needed by decoder */

    unsigned config;
} qr_finder_t;
