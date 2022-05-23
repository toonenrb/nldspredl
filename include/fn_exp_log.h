/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef FN_EXP_LOG_H
#define FN_EXP_LOG_H

#include "log_meta.h"

struct s_log_kpexp {
    int    nnn_add;
    int    excl;
    int    var_win;
    int    denom_type;
    double exp_k;
};

#define ATTACH_META_KPEXP(_m, _s) \
static Trec_meta _m = { LOG_KPEXP, "KPEXP", 5, &_s, \
    { \
        { "kpexp_nnn_add", FT_INT, -1, &_s.nnn_add }, \
        { "kpexp_excl", FT_INT, -1, &_s.excl }, \
        { "kpexp_var_win", FT_INT, -1, &_s.var_win }, \
        { "kpexp_denom_type", FT_INT, -1, &_s.denom_type }, \
        { "kpexp_exp_k", FT_DOUBLE, -1, &_s.exp_k } \
    } \
}

#endif
