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

/* Status values for pd.status field in logging.
 * bit 0 - 7:  general status codes
 * bit 8 - 9:  iwarn output
 * bit 10 - 12: ierr output
 * bit 13 - 15: info output from dsvdc.f
 */
typedef enum {
  STLS_OK                        = 0x0000,
  STLS_AUG_N_POINTS_ZERO         = 0x0001,
  STLS_WARNING                   = 0x0002,
  STLS_ERROR                     = 0x0004,
  STLS_WARN_IS_ERROR             = 0x0008,
  STLS_VAL_GT_RESTRICT           = 0x0010
} Tstatus_tls;

#endif
