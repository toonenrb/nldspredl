/*
 * $Log: fn_tls_log.h,v $
 * Revision 1.1  2022/01/21 09:23:27  R.B.Toonen
 * Changed kern names in fn.
 *
 */

#ifndef FN_TLS_LOG_H
#define FN_TLS_LOG_H

#include "log_meta.h"

struct s_log_kptls {
    int     nnn;
    int     excl;
    int     var_win;
    double  theta;
    int     center;
};

#define ATTACH_META_KPTLS(_m, _s) \
static Trec_meta _m = { LOG_KPTLS, "KPTLS", 5, &_s, \
    { \
        { "kptls_nnn", FT_INT, -1, &_s.nnn }, \
        { "kptls_excl", FT_INT, -1, &_s.excl }, \
        { "kptls_var_win", FT_INT, -1, &_s.var_win }, \
        { "kptls_theta", FT_DOUBLE, -1, &_s.theta }, \
        { "kptls_center", FT_INT, -1, &_s.center } \
    } \
}

struct s_log_vpm {
    char    copr;
    int     var_num;
    double  var_mean;
};

#define ATTACH_META_VPM(_m, _s) \
static Trec_meta _m = { LOG_VPM, "VPM", 3, &_s, \
    { \
        { "vpm_copr", FT_CHAR, -1, &_s.copr }, \
        { "vpm_var_num", FT_INT, -1, &_s.var_num }, \
        { "vpm_var_mean", FT_DOUBLE, -1, &_s.var_mean } \
    } \
}

struct s_log_vpt {
    long    target_num;
    int     pre_val_num;
    int     var_coord;
    double  var_val;
};

#define ATTACH_META_VPT(_m, _s) \
static Trec_meta _m = { LOG_VPT, "VPT", 4, &_s, \
    { \
        { "vpt_target_num", FT_LONG, -1, &_s.target_num }, \
        { "vpt_pre_val_num", FT_INT, -1, &_s.pre_val_num }, \
        { "vpt_var_coord", FT_INT, -1, &_s.var_coord }, \
        { "vpt_var_val", FT_DOUBLE, -1, &_s.var_val } \
    } \
}

#endif
