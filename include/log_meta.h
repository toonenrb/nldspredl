/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef LOG_META_H
#define LOG_META_H

typedef enum { FT_CHAR, FT_STRING, FT_INT, FT_SHORT, FT_LONG, FT_DOUBLE } Tfield_type;

#define N_LOG_REC 27

typedef enum {
    LOG_BP           = 0,
    LOG_BV           = 1,
    LOG_KP           = 2,
    LOG_TG1          = 3,
    LOG_TG2          = 4,
    LOG_NN           = 5,
    LOG_PD           = 6,
    LOG_KPEXP        = 7,
    LOG_KPTLS        = 8,
    LOG_VPM          = 9,
    LOG_VPT          = 10,
    LOG_EMBPAR1      = 11,
    LOG_EMBPAR2      = 12,
    LOG_EMBPAR3      = 13,
    LOG_VID          = 14,
    LOG_VAL          = 15,
    LOG_SPCL         = 16,
    LOG_SPKF         = 17,
    LOG_SPLO         = 18,
    LOG_SPUV         = 19,
    LOG_SPBT         = 20,
    LOG_SH           = 21,
    LOG_SD           = 22,
    LOG_STNP         = 23,
    LOG_STAT         = 24,
    LOG_ADHD         = 25,
    LOG_ADDT         = 26
} Tlog_rec_type;

#define MAX_COLNAME_LOG 30
#define MAX_RECNAME_LOG 10

typedef struct {
    char        name[MAX_COLNAME_LOG];
    Tfield_type field_type;
    int         str_sz;  /*when field type is string*/
    void        *val;
} Tfield_meta;    /*Used when reading records from a binary log file and converting to for example csv file.*/

typedef struct {
    Tlog_rec_type rec_type;
    char          name[MAX_RECNAME_LOG];
    int           nfield;
    void          *rec;
    Tfield_meta field_meta[];
} Trec_meta;

#endif
