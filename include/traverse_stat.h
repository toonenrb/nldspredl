/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TRAVERSE_STAT_H
#define TRAVERSE_STAT_H

#include "tstoembdef.h"
#include "stat.h"

typedef struct
{
    void         *fn_params;
    int          emb_num;
    char         *emb_label;
    int          bundle_num;
    int          n_bundle_val;
    double       *bundle_vec;
    int          set_num;
    int          n_addit_val;
    double       *addit_val;
    long         lib_set_n_point;
    int          lib_set_e;
    int          lib_set_n_pre_val;
    long         pre_set_n_point;
    int          pre_set_e;
    int          pre_set_n_pre_val;
    Temb_lag_def *emb_lag_def;
    Tstat        *stat;
} Tnlpre_stat;

#endif
