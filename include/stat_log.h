/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef STAT_LOG_H
#define STAT_LOG_H

#include "log_meta.h"

struct s_log_stnp {
    int     n_pre_val;
};

#define ATTACH_META_STNP(_m, _s) \
static Trec_meta _m = { LOG_STNP, "STNP", 1, &_s, \
    { \
        { "stnp_n_pre_val", FT_INT, -1, &_s.n_pre_val } \
    } \
}

struct s_log_stat {
    int     pre_val_num;
    long    n_pre_obs;
    double  avg_pre;
    double  avg_obs;
    double  var_pre;
    double  var_obs;
    double  cov_pre_obs;
    double  mae_pre_obs;
    double  rmse_pre_obs;
    double  md_pre;
    double  md_obs;
    double  mdad_pre;
    double  mdad_obs;
    double  mdae_pre_obs;
};

#define ATTACH_META_STAT(_m, _s) \
static Trec_meta _m = { LOG_STAT, "STAT", 14, &_s, \
    { \
        { "stat_pre_val_num", FT_INT, -1, &_s.pre_val_num }, \
        { "stat_n_pre_obs", FT_LONG, -1, &_s.n_pre_obs }, \
        { "stat_avg_pre", FT_DOUBLE, -1, &_s.avg_pre }, \
        { "stat_avg_obs", FT_DOUBLE, -1, &_s.avg_obs }, \
        { "stat_var_pre", FT_DOUBLE, -1, &_s.var_pre }, \
        { "stat_var_obs", FT_DOUBLE, -1, &_s.var_obs }, \
        { "stat_cov_pre_obs", FT_DOUBLE, -1, &_s.cov_pre_obs }, \
        { "stat_mae_pre_obs", FT_DOUBLE, -1, &_s.mae_pre_obs }, \
        { "stat_rmse_pre_obs", FT_DOUBLE, -1, &_s.rmse_pre_obs }, \
        { "stat_md_pre", FT_DOUBLE, -1, &_s.md_pre }, \
        { "stat_md_obs", FT_DOUBLE, -1, &_s.md_obs }, \
        { "stat_mdad_pre", FT_DOUBLE, -1, &_s.mdad_pre }, \
        { "stat_mdad_obs", FT_DOUBLE, -1, &_s.mdad_obs }, \
        { "stat_mdae_pre_obs", FT_DOUBLE, -1, &_s.mdae_pre_obs }, \
    } \
}

#endif
