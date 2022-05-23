/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TSFILE_H
#define TSFILE_H

#include <stdio.h>
#include "tsdat.h"

Tfdat 
*csv_to_fdat (
    FILE *fi,             // Pointer to file
    bool head,            // File contains header (true/false)
    char *sep,            // Field separator
    char *tm_col,         // Name or number of column which contains time values (NULL->no such column)
    char *id_col,         // Name or number of column which contains id (for embeddings of multiple subjects), or NULL
    char *data_cols,      // Comma separated list of column names or numbers of data columns, which go in library or pred values.
    char *in_lib_col,     // If used, name of column which specifies whether value should be used in library set, or  NULL
    char *in_pre_col,     // If used, name of column which specifies whether value should be used in prediction set, or  NULL
    char *bundle_cols     // If used, comma separated list of column names or numbers of columns, for bundle values, or NULL.
    );
int   free_fdat (Tfdat *);

#endif
