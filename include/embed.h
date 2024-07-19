/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef EMBED_H
#define EMBED_H

#include <stdbool.h>

typedef struct
{
    char    *var_name;
    int     var_no;    /*column number in fdat ts data, starting at 0*/
    int     lag;       /*positive lag = past value, negative lag = future*/
} Tco_val_meta;

typedef struct
{
    char    *var_name;
    int     var_no;    /*column number in fdat ts data, starting at 0*/
    int     lag;
} Tpre_val_meta;

typedef struct
{
    char    *var_name;
    int     var_no;    /*column number in fdat ts data, starting at 0*/
} Tbundle_val_meta;

/* Also store values of additional variables. These are not used for computation, but are reported together
 * with predicted and observed values to simplify statistical processing afterwards.
 * This only applies to numerical variables.
 */
typedef struct
{
    char    *var_name;
    int     var_no;    /*column number in fdat ts data, starting at 0*/
    int     lag;
} Taddit_val_meta;

typedef struct
{
    int             emb_num;
    char            *emb_label;
    long            n_row;
    int             e;                     /*embedding dimension*/
    int             n_pre_val;             /*number of columns in prediction values matrix*/
    int             n_bundle_val;          /*number of bundle variables (usually 1 or 0)*/
    int             n_addit_val;           /*number of additional variables to store.*/
    int             n_id;                  /*number of unique id's*/
    char            **id;                  /*array of pointers to all available id's, size = n_id*/
    char            **id_arr;              /*array of pointers to id's  size = n_row*/
    long            *id_start_row;         /*array of indexes that specify where an id block starts, size=n_id*/
    long            *vec_num;              /*vector number, unique. Equal to row number in the matrices*/
    long            *t_co_mat;             /*t matrix for co_val's  size = n_row*e */
    double          *co_val_mat;           /*co_value matrix   size = n_row*e */
    double          *pre_val_mat;          /*pre_val matrix    size = n_row*n_pre_val*/
    double          *bundle_mat;           /*bundle val matrix size = n_row*n_bundle_val*/
    long            *t_pre_mat;            /*t matrix for pre_val's, size = n_row*n_pre_val*/
    bool            *use_in_lib;
    bool            *use_in_pre;
    double          *addit_mat;            /*size = n_row*n_addit_val*/
    long            *t_addit_mat;          /*t values of additional variables*/
    int             span;                  /*difference between minimum lag and maximum lag*/
    Tco_val_meta    *co_meta;              /*size = e*sizeof(Tco_val_meta)*/
    Tpre_val_meta   *pre_meta;             /*size = n_pre_val*sizeof(Tpre_val_meta)*/
    Taddit_val_meta *addit_meta;           /*size = n_addit_val*sizeof(Taddit_val_meta)*/
    Tbundle_val_meta *bundle_meta;         /*size = n_bundle_val*sizeof(Tbundle_val_meta)*/
    int         n_ranges;    /*Only used when a range string was supplied. */
    int         *range_dim;  /*Only used when a range string was supplied. */
    int         *range_tau;  /*Only used when a range string was supplied. */
} Tembed;

/* pre_val_na_master contains a row for each NA value in the pre_val_mat.
 * It will contain the corresponding row number and column number of the location in the pre_val_mat.
 *
 * For each row in pre_val_master there will be rows, with the same row num (as of pre_val_na_master)
 * in co_val_na_loc and pre_val_na_loc. The co_val_na_loc matrix has the same number of columns as
 * the co_val_mat; the pre_val_na_loc matrix has the same number of columns as the pre_val_mat.
 * Each column in co_val_na_loc contains the row number (as a long value) of the row in co_val_mat
 * that has the same time index for this column as the time index of the column that was defined in
 * the pre_val_na_master row. But only if those rows have the same ID value (in the case of dewdrop
 * embeddings) as the ID in the pre_val matrix. If no such row is available, the co_val_na_loc will 
 * have the LONG_MAX value.
 * The same goes for pre_val_na_loc, but now for the pre_val_mat.
 * This is used for imputation.
 */

#endif
