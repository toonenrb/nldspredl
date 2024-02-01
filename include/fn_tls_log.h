/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
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
    short   object_only_once;
};

#define ATTACH_META_KPTLS(_m, _s) \
static Trec_meta _m = { LOG_KPTLS, "KPTLS", 5, &_s, \
    { \
        { "kptls_nnn", FT_INT, -1, &_s.nnn }, \
        { "kptls_excl", FT_INT, -1, &_s.excl }, \
        { "kptls_var_win", FT_INT, -1, &_s.var_win }, \
        { "kptls_theta", FT_DOUBLE, -1, &_s.theta }, \
        { "kptls_center", FT_INT, -1, &_s.center }, \
        { "kptls_object_only_once", FT_SHORT, -1, &_s.object_only_once } \
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

struct s_log_dtlso {
    double  tol1;
    double  tol2;
    int     ldx;
    int     rank;
    int     ierr;
    int     iwarn;
    int     ldc;
    int     m;
    int     n;
    int     l;
};

#define ATTACH_META_DTLSO(_m, _s) \
static Trec_meta _m = { LOG_DTLSO, "DTLSO", 10, &_s, \
    { \
        { "dtlso_tol1", FT_DOUBLE, -1, &_s.tol1 }, \
        { "dtlso_tol2", FT_DOUBLE, -1, &_s.tol2 }, \
        { "dtlso_ldx", FT_INT, -1, &_s.ldx }, \
        { "dtlso_rank", FT_INT, -1, &_s.rank }, \
        { "dtlso_ierr", FT_INT, -1, &_s.ierr }, \
        { "dtlso_iwarn", FT_INT, -1, &_s.iwarn }, \
        { "dtlso_ldc", FT_INT, -1, &_s.ldc }, \
        { "dtlso_m", FT_INT, -1, &_s.m }, \
        { "dtlso_n", FT_INT, -1, &_s.n }, \
        { "dtlso_l", FT_INT, -1, &_s.l } \
    } \
}

struct s_log_dtlsan {    /*Name of array*/
    char    name;  /*C, S, X*/
    long    nrow;
    long    ncol;
};

#define ATTACH_META_DTLSAN(_m, _s) \
static Trec_meta _m = { LOG_DTLSAN, "DTLSAN", 3, &_s, \
    { \
        { "dtlsan_name", FT_CHAR, -1, &_s.name }, \
        { "dtlsan_nrow", FT_LONG, -1, &_s.nrow }, \
        { "dtlsan_ncol", FT_LONG, -1, &_s.ncol } \
    } \
}

struct s_log_dtlsav {    /*Array values*/
    long      row;
    long      col;
    double    val;
};

#define ATTACH_META_DTLSAV(_m, _s) \
static Trec_meta _m = { LOG_DTLSAV, "DTLSAV", 3, &_s, \
    { \
        { "dtlsav_row", FT_LONG, -1, &_s.row }, \
        { "dtlsav_col", FT_LONG, -1, &_s.col }, \
        { "dtlsav_val", FT_DOUBLE, -1, &_s.val } \
    } \
}

#endif
