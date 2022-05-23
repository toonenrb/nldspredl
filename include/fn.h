/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef FN_H
#define FN_H

#include "point.h"

typedef enum { FN_EXP, FN_TLS } Tfn_type;

typedef int (*Tfn) (Tpoint_set *lib_set, Tpoint_set *pre_set, double **predicted);
typedef int (*Tnew_fn_params) (int e, void **fn_params);
typedef int (*Tnext_fn_params) (void **fn_params);

int
log_fn (Tfn_type fn_type);

int
log_nn (Tpoint *target, Tpoint **rs, double *sqdst, int n);

int
log_predicted (Tpoint **pt, int n, int n_val, double *pre_val);

#endif
