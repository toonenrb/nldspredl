/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef SETS_LOG_H
#define SETS_LOG_H

#include "log_meta.h"

struct s_log_spcl {
    int     lib_size;
    int     shift;
    int     boot_i;
};

#define ATTACH_META_SPCL(_m, _s) \
static Trec_meta _m = { LOG_SPCL, "SPCL", 3, &_s, \
    { \
        { "spcl_lib_size", FT_INT, -1, &_s.lib_size }, \
        { "spcl_shift", FT_INT, -1, &_s.shift }, \
        { "spcl_boot_i", FT_INT, -1, &_s.boot_i } \
    } \
}


struct s_log_spkf {
    int     k;
    int     repetition;
};

#define ATTACH_META_SPKF(_m, _s) \
static Trec_meta _m = { LOG_SPKF, "SPKF", 2, &_s, \
    { \
        { "spkf_k", FT_INT, -1, &_s.k }, \
        { "spkf_repetition", FT_INT, -1, &_s.repetition } \
    } \
}

struct s_log_splo {
    short    dummy;
};

#define ATTACH_META_SPLO(_m, _s) \
static Trec_meta _m = { LOG_SPLO, "SPLO", 1, &_s, \
    { \
        { "splo_dummy", FT_SHORT, -1, &_s.dummy } \
    } \
}

struct s_log_spbt {
    int      boot_i;
};

#define ATTACH_META_SPBT(_m, _s) \
static Trec_meta _m = { LOG_SPBT, "SPBT", 1, &_s, \
    { \
        { "spbt_boot_i", FT_INT, -1, &_s.boot_i } \
    } \
}

struct s_log_spuv {
    short    dummy;
};

#define ATTACH_META_SPUV(_m, _s) \
static Trec_meta _m = { LOG_SPUV, "SPUV", 1, &_s, \
    { \
        { "spuv_dummy", FT_SHORT, -1, &_s.dummy } \
    } \
}

struct s_log_sh {
    char     set_code;
    int      set_num;
    long     n_point;
};

#define ATTACH_META_SH(_m, _s) \
static Trec_meta _m = { LOG_SH, "SH", 3, &_s, \
    { \
        { "sh_set_code", FT_CHAR, -1, &_s.set_code }, \
        { "sh_set_num", FT_INT, -1, &_s.set_num }, \
        { "sh_n_point", FT_LONG, -1, &_s.n_point } \
    } \
}


struct s_log_sd {
    char     set_code;
    int      vec_idx;
    long     vec_num;
};

#define ATTACH_META_SD(_m, _s) \
static Trec_meta _m = { LOG_SD, "SD", 3, &_s, \
    { \
        { "sd_set_code", FT_CHAR, -1, &_s.set_code }, \
        { "sd_vec_idx", FT_INT, -1, &_s.vec_idx }, \
        { "sd_vec_num", FT_LONG, -1, &_s.vec_num } \
    } \
}


#endif
