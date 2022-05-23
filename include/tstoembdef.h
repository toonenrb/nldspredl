/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TSTOEMBDEF_H
#define TSTOEMBDEF_H

typedef struct
{
    char    *var_name;
    int     lag;
} Tlag_var;

typedef struct
{
    int         n_co_lag;    /*vector coordinate values*/
    Tlag_var    *co_lag;
    int         n_pre_lag;   /*variables to predict.*/
    Tlag_var    *pre_lag;
    int         n_addit_lag; /*additional values, not used for computation.*/
    Tlag_var    *addit_lag;
    int         n_ranges;    /*Only used when a range string was supplied. */
    int         *range_dim;  /*Only used when a range string was supplied. */
    int         *range_tau;  /*Only used when a range string was supplied. */
} Temb_lag_def;

void
free_lag_def (Temb_lag_def *eld, int n_lag_def);

Temb_lag_def *
str_to_lag_def (char *lag_def_str, int *_n_lag_def);

Temb_lag_def *
str_range_to_lag_def (char *lag_range_str, int *_n_lag_def);
#endif
