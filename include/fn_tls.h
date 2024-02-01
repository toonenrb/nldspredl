/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef FN_TLS_H
#define FN_TLS_H

#include <stdbool.h>
#include "fn.h"

typedef enum { KTLS_REFMETH_MEAN, KTLS_REFMETH_XNN_GT_ZERO } Ttls_ref_meth;

int
init_fn_tls (Tnew_fn_params *new_fn_params,
             Tnext_fn_params *next_fn_params,
             Tfn *fn,
             double theta_min, double theta_max, double delta_theta,
             int nnn, Texcl excl, int var_win, bool center, double restrict_prediction,
             bool warn_is_error, Ttls_ref_meth ref_meth, int ref_xnn, bool object_only_once);

typedef struct
{
    int     nnn;
    Texcl   excl;
    int     var_win;
    double  theta;
    bool    center;
} Tfn_params_tls;

#endif
