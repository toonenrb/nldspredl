/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef STAT_H
#define STAT_H

#include "point.h"

typedef struct
{
    int     n_pre_val;
    long    *n_pre_obs;           /*number of non NaN combinations*/
    double  *avg_pre, *avg_obs;
    double  *var_pre, *var_obs;
    double  *cov_pre_obs, *mae_pre_obs, *rmse_pre_obs;
    double  *md_obs, *md_pre;     /*median*/
    double  *mdad_obs, *mdad_pre; /*median absolute deviation from median*/
    double  *mdae_pre_obs;        /*median absolute error*/

} Tstat;

Tstat *
prediction_stat (Tpoint_set *points_observed, double *predicted);

int
free_stat (Tstat *stat);

#endif
