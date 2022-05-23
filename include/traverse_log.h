/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TRAVERSE_LOG_H
#define TRAVERSE_LOG_H

#include "log_meta.h"

struct s_log_adhd {
    int     n_addit_val;
};

#define ATTACH_META_ADHD(_m, _s) \
static Trec_meta _m = { LOG_ADHD, "ADHD", 1, &_s, \
    { \
        { "adhd_n_addit_val", FT_INT, -1, &_s.n_addit_val } \
    } \
};

struct s_log_addt {
    int     idx;
    double  addit_val;
};

#define ATTACH_META_ADDT(_m, _s) \
static Trec_meta _m = { LOG_ADDT, "ADDT", 2, &_s, \
    { \
        { "addt_idx", FT_INT, -1, &_s.idx }, \
        { "addt_addit_val", FT_DOUBLE, -1, &_s.addit_val } \
    } \
};

#endif
