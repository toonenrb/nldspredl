/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef BUNDLE_LOG_H
#define BUNDLE_LOG_H

#include "log_meta.h"

struct s_log_bp {
    int  bundle_num;
};

#define ATTACH_META_BP(_m, _s) \
    static Trec_meta _m = { LOG_BP, "BP", 1, &_s, \
        { \
            { "bp_bundle_num", FT_INT, -1, &_s.bundle_num } \
        } \
    }

struct s_log_bv {
    int    vec_idx;
    double vec_val;
};

#define ATTACH_META_BV(_m, _s) \
    static Trec_meta _m = { LOG_BV, "BV", 2, &_s, \
        { \
            { "bv_vec_idx", FT_INT, -1, &_s.vec_idx }, \
            { "bv_vec_val", FT_DOUBLE, -1, &_s.vec_val } \
        } \
    }

#endif
