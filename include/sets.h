/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef SETS_H
#define SETS_H

#include <stdbool.h>
#include "embed.h"
#include "bundle.h"
#include "point.h"

typedef enum { LIB_SHIFT_SHIFT, LIB_SHIFT_RANDOM, LIB_SHIFT_BOOTSTRAP, LIB_SHIFT_BOOT_PERMUT } Tlib_shift_meth;

typedef int (*Tnew_sets) (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);
typedef int (*Tnext_set) (void);
typedef int (*Tfree_set) (void);

int
init_set_convergent_lib (int lib_size_min, int lib_size_max, int lib_inc, float lib_inc_inc_factor,
                         Tlib_shift_meth lib_shift_meth, int lib_shift, int n_bootstrap,
                         Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set);

int
init_set_k_fold (int k_fold, int n_repetition, Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set);

int
init_set_looc (Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set);

int
init_set_bootstrap (int lib_size, int n_bootstrap, bool pre_set_is_lib, bool lib_size_is_emb_size, bool per_addit_group,
                    Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set);

int
init_set_user_val (Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set);

#endif
