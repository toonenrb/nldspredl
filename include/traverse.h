/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TRAVERSE_H
#define TRAVERSE_H

#include "tsfile.h"
#include "tstoembdef.h"
#include "sets.h"
#include "fn_exp.h"

#ifdef NLPRESTATOUT
#include "traverse_stat.h"
#endif

int
traverse_all (Tfdat *fdat, int n_emb_lag_def, Temb_lag_def emb_lag_def[],
              Tnew_sets new_sets, Tnext_set next_set, Tfree_set free_set,
              Tnew_fn_params new_fn_params, Tnext_fn_params next_fn_params, Tfn fn,
              bool validation, bool per_additional_val
#ifdef NLPRESTATOUT
                 , Tnlpre_stat **nlpre_stat, int *n_nlpre_stat
#endif
                 );

int
free_traverse ();
#endif
