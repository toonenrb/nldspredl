/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef TSDAT_H
#define TSDAT_H

/* Tfdat:
 * dat: Points to memory, mapped as matrix of size (n_dat * n_col).
 *      Data is stored in row major order. One row contains values of different variables at
 *      the same time.
 * t  : The time corresponding to the row.
 * lab: Array of pointers to strings. Each string specifies the name of the column in dat
 *      with the same column offset as the array index.
 * id : When combining data of multiple systems (e.g. persons, experiments, etc.) the id
 *      column is used to differentiate between the systems. This is used to prevent the contruction
 *      of embedding vectors containing values from different systems. One vector should only contain
 *      data from one system.
 * in_lib: If used: specifies if data from the row can be used in the set of library vectors.
 * in_pre: If used: specifies if data from the row can be used in the set of prediction values.
 *
 * bundle    : Values of external forcing variables which can be used to
               create bundle (or fibre) embeddings. (J. Stark, J. Nonlin. Sci., 1999)
 * bundle_lab: Array of pointers to strings. Each string specifies the name of the column in
 *             bundle with the same column offset as the array index.
 *
 * NOTE: data in time series file should be ordered according to id first and then to time value.
 *       When using multiple id's then always use the column for t as well and supply integer time value.
 *      example:     Id        Time     Var A      Var B  etc
 *                   A1          0      20         -10    ..
 *                   A1          1      24          -7    ..
 *                    .          .       .           .    ..
 *                   A1        130      17           2    ..
 *                   B1          0      31          -1    ..
 *                   B1          1      23          -5    ..
 *                    etc
 *
 */

typedef struct
{
    double  *  dat;
    long    *  t;
    char    ** lab;
    char    ** bundle_lab;
    char    ** id;
    bool    *  in_lib;
    bool    *  in_pre;
    double  *  bundle;
    int     n_col;
    long    n_dat;
    int     n_bundle_col;
} Tfdat;

#endif
