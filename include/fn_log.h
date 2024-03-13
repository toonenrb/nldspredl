/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef FN_LOG_H
#define FN_LOG_H

#include "log_meta.h"

struct s_log_kp {
    char fn_type[4];
};

#define ATTACH_META_KP(_m, _s) \
static Trec_meta _m = { LOG_KP, "KP", 1, &_s, \
    { \
        { "kp_fn_type", FT_STRING, 4, _s.fn_type } \
    } \
}

struct s_log_tg1 {
    long target_num;
};

#define ATTACH_META_TG1(_m, _s) \
static Trec_meta _m = { LOG_TG1, "TG1", 1, &_s, \
    { \
        { "tg1_target_num", FT_LONG, -1, &_s.target_num } \
    } \
}


struct s_log_nn {
    int seq;
    long num;
    double sqdst;
};

#define ATTACH_META_NN(_m, _s) \
static Trec_meta _m = { LOG_NN, "NN", 3, &_s, \
    { \
        { "nn_seq", FT_INT, -1, &_s.seq }, \
        { "nn_num", FT_LONG, -1, &_s.num }, \
        { "nn_sqdst", FT_DOUBLE, -1, &_s.sqdst } \
    } \
}

struct s_log_tg2 {
    long target_num;
};

#define ATTACH_META_TG2(_m, _s) \
static Trec_meta _m = { LOG_TG2, "TG2", 1, &_s, \
    { \
        { "tg2_target_num", FT_LONG, -1, &_s.target_num } \
    } \
}


struct s_log_pd {
    int pre_val_num;
    double pre_val;
    double obs_val;
    int  status;
};

#define ATTACH_META_PD(_m, _s) \
static Trec_meta _m = { LOG_PD, "PD", 4, &_s, \
    { \
        { "pd_pre_val_num", FT_INT, -1, &_s.pre_val_num }, \
        { "pd_pre_val", FT_DOUBLE, -1, &_s.pre_val }, \
        { "pd_obs_val", FT_DOUBLE, -1, &_s.obs_val }, \
        { "pd_status", FT_INT, -1, &_s.status } \
    } \
}

#endif
