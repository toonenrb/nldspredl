/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef BUNDLE_H
#define BUNDLE_H
#include "embed.h"

typedef struct
{
    long   *idx;            /* Index into embedding.*/
    long   n_idx;           /* Number of point indexes in current bundle.*/
    int    bundle_num;
    double *bundle_vec;     /* Holds the values of the bundle vars.*/
    int    n_bundle_val;    /* Number of scalars in bundle vec.*/ 
} Tbundle_set;

int
new_bundles (Tembed *emb, Tbundle_set **bundle_set);

int
next_bundle (void);

void
free_bundle (void);

int
init_bundle_filter (char *s_filter_val);

#endif
