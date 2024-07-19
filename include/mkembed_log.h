/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef MKEMBED_LOG_H
#define MKEMBED_LOG_H

#include "log_meta.h"

struct s_log_embpar1 {
    int     emb_num;
    char    emb_label[160];
    long    n_row;
    int     e;
    int     n_pre_val;
    int     n_bundle_val;
    int     n_ranges;
};

#define ATTACH_META_EMBPAR1(_m, _s) \
static Trec_meta _m = { LOG_EMBPAR1, "EMBPAR1", 7, &_s, \
    { \
        { "embpar1_emb_num", FT_INT, -1, &_s.emb_num }, \
        { "embpar1_emb_label", FT_STRING, 133, _s.emb_label }, \
        { "embpar1_n_row", FT_LONG, -1, &_s.n_row }, \
        { "embpar1_e", FT_INT, -1, &_s.e }, \
        { "embpar1_n_pre_val", FT_INT, -1, &_s.n_pre_val }, \
        { "embpar1_n_bundle_val", FT_INT, -1, &_s.n_bundle_val }, \
        { "embpar1_n_ranges", FT_INT, -1, &_s.n_ranges } \
    } \
}

struct s_log_embpar2 {
    char    copr;
    int     coord;
    char    var_name[21];
    int     lag;
    int     var_no;
};

#define ATTACH_META_EMBPAR2(_m, _s) \
static Trec_meta _m = { LOG_EMBPAR2, "EMBPAR2", 5, &_s, \
    { \
        { "embpar2_copr", FT_CHAR, -1, &_s.copr }, \
        { "embpar2_coord", FT_INT, -1, &_s.coord }, \
        { "embpar2_var_name", FT_STRING, 21, &_s.var_name }, \
        { "embpar2_lag", FT_INT, -1, &_s.lag }, \
        { "embpar2_var_no", FT_INT, -1, &_s.var_no } \
    } \
}

struct s_log_embpar3 {
    int    seq;
    int    dim;
    int    tau;
};

#define ATTACH_META_EMBPAR3(_m, _s) \
static Trec_meta _m = { LOG_EMBPAR3, "EMBPAR3", 3, &_s, \
    { \
        { "embpar3_seq", FT_INT, -1, &_s.seq }, \
        { "embpar3_dim", FT_INT, -1, &_s.dim }, \
        { "embpar3_tau", FT_INT, -1, &_s.tau } \
    } \
}

struct s_log_vid {
    long    vec_num;
    char    id[10];
    short   use_in_lib;
    short   use_in_pre;
};

#define ATTACH_META_VID(_m, _s) \
static Trec_meta _m = { LOG_VID, "VID", 4, &_s, \
    { \
        { "vid_vec_num", FT_LONG, -1, &_s.vec_num }, \
        { "vid_id", FT_STRING, 10, _s.id }, \
        { "vid_use_in_lib", FT_SHORT, -1, &_s.use_in_lib }, \
        { "vid_use_in_pre", FT_SHORT, -1, &_s.use_in_pre } \
    } \
}


struct s_log_val {
    char   copr;
    int    idx;
    long   t;
    double val;
};

#define ATTACH_META_VAL(_m, _s) \
static Trec_meta _m = { LOG_VAL, "VAL", 4,  &_s, \
    { \
        { "val_copr", FT_CHAR, -1, &_s.copr }, \
        { "val_idx", FT_INT, -1, &_s.idx }, \
        { "val_t", FT_LONG, -1, &_s.t }, \
        { "val_val", FT_DOUBLE, -1, &_s.val } \
    } \
}

#endif
