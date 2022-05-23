/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef POINT_H
#define POINT_H

#include <stdbool.h>

typedef struct
{
    bool        use_in_lib;
    bool        use_in_pre;
    char        *id;         /*pointer into id's array of embedding*/
    long        vec_num;     /*unique number for referencing*/
    long        *t;          /*pointer into corresponding row in t matrix of embedding*/
    long        *t_pre;      /*pointer into corresponding row in t_pre matrix of embedding*/
    double      *co_val;     /*pointer into corresponding row in co_val matrix (coordinate values) of embedding*/
    short       *co_var_num; /*numbers representing the variable used. For exclusion function.*/
    double      *pre_val;    /*pointer into corresponding row in pre_val matrix (prediction values) of embedding*/
    double      *bundle_val; /*pointer into corresponding row in bundle matrix of embedding*/
    double      *addit_val;  /*pointer into corresponding row in additional value matrix of embedding*/
} Tpoint;

typedef struct
{
    int    set_num;
    long   n_point;
    int    e;            /*dimension of co_val*/
    int    n_pre_val;    /*dimension of pre_val*/
    int    n_bundle_val; /*dimension of bundle_val*/
    int    n_addit_val;  /*dimension of addit_val*/
    Tpoint **point;
} Tpoint_set;

typedef enum { T_EXCL_NONE             = 0,    /*none*/
               T_EXCL_TIME_COORD       = 1,    /*exclude vecs with one or more shared coordinates */
               T_EXCL_TIME_WIN         = 2,    /*exclude vecs within a time range                 */
               T_EXCL_SELF             = 4     /*only itself                                      */
             } Texcl;

double *
get_co_vec (Tpoint *point);

long *
get_t_vec (Tpoint *point);

double *
get_pre_vec (Tpoint *point);

int 
exclude_init (Texcl excl, int var_win, int e);

bool 
exclude (Tpoint *tg, Tpoint *cd);  /*exclude candidate from prediction set for target?*/

#endif
