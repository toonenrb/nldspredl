/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef FN_EXP_H
#define FN_EXP_H

#include "fn.h"

typedef enum { FN_WEIGHT_DENOM_MINIMUM, FN_WEIGHT_DENOM_AVG_NN, FN_WEIGHT_DENOM_AVG_LIB } Tfn_denom;

int
init_fn_exponential (Tnew_fn_params *new_fn_params,
                     Tnext_fn_params *next_fn_params,
                     Tfn *fn,
                     int nnn_add, Texcl excl, int var_win,
                     Tfn_denom fn_denom, double exp_k,
                     bool object_only_once);

typedef struct
{
    int     nnn_add;
    Texcl   excl;
    int     var_win;
} Tfn_params_exponential;

#endif
