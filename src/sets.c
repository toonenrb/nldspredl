/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Make sets of library and prediction vector (s).
 *
 * Note: The next_set functions return one of the following values:
 *       return_value = 0 ==> succesfull new set.
 *                    < 0 ==> error
 *                    > 0 ==> no more sets
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "embed.h"
#include "bundle.h"
#include "point.h"
#include "sets.h"
#include "log.h"
#include "sets_log.h"

static Tembed        *l_emb;
static Tpoint        *l_all_points = NULL;
static int           l_set_num;
static unsigned long l_n_points = 0;  /*Fix -Walloc_size.. warning of gcc*/

static Tpoint      **l_rnd_point_twice = NULL;
static Tpoint      **l_all_point_twice = NULL;
static Tpoint      **l_all_point_rnd   = NULL;

static Tpoint_set  *l_lib_set, *l_pre_set;

static short       *l_co_var_num = NULL;

int
new_sets_convergent_lib (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);

int
next_set_convergent_lib (void);

int
free_sets_convergent_lib (void);

int
new_sets_k_fold (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);

int
next_set_k_fold (void);

int
new_sets_looc (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);

int
next_set_looc (void);

int
free_sets_looc (void);

int
new_sets_bootstrap (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);

int
next_set_bootstrap (void);

int
free_sets_bootstrap (void);

int
new_sets_user_val (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set);

int
next_set_user_val (void);

int
free_sets_user_val (void);

int
log_set (Tlog_levels log_level, Tpoint_set *set);

int
log_set_par_convergent_lib (void);

int
log_set_par_k_fold (void);

int
log_set_par_looc (void);

int
log_set_par_bootstrap (void);

int
log_set_par_user_val (void);

static int
free_sets (void)
{
    if (l_all_points)
    {
        free (l_all_points);
        l_all_points = NULL;
        l_n_points   = 0;
    }

    if (l_lib_set)
    {
        free (l_lib_set);
        l_lib_set = NULL;
    }

    if (l_pre_set)
    {
        free (l_pre_set);
        l_pre_set = NULL;
    }

    if (l_rnd_point_twice)
    {
        free (l_rnd_point_twice);
        l_rnd_point_twice = NULL;
    }

    if (l_all_point_twice)
    {
        free (l_all_point_twice);
        l_all_point_twice = NULL;
    }

    if (l_all_point_rnd)
    {
        free (l_all_point_rnd);
        l_all_point_rnd = NULL;
    }

    if (l_co_var_num)
    {
        free (l_co_var_num);
        l_co_var_num = NULL;
    }

}

static short *
make_var_num (Tembed *emb)
{
    char **var_name;
    int  i, j, num_var;
    short *co_var_num;

    var_name = (char **) malloc (emb->e * sizeof (char *));
    co_var_num = (short *) malloc (emb->e * sizeof (short));

    num_var = 0;
    for (i = 0; i < emb->e; i++)
    {
        for (j = 0; j < num_var; j++)
            if (strcmp(emb->co_meta[i].var_name, var_name[j]) == 0)
                break;

        if (j == num_var) /*var name not yet in list*/
            var_name[j] = emb->co_meta[i].var_name;

        co_var_num[i] = j;
    }

    free (var_name);
    return co_var_num;
}

static int
make_sets (Tembed *emb, Tbundle_set *bundle_set)
{
    char   **id;
    long   *t, *t_pre, *vec_num;
    double *co_val, *pre_val, *addit_val;
    bool   *use_in_lib, *use_in_pre;
    Tpoint *point;
    int    e, n_pre_val, n_addit_val;

    if (!emb)
        return -1;

    if (emb->n_row <= 0)
        return -2;

    l_emb = emb;

    /* create new array with all points */
    l_n_points = bundle_set? bundle_set->n_idx: emb->n_row;

    l_all_points = (Tpoint *) malloc (l_n_points * sizeof (Tpoint));

    id           = emb->id_arr;
    vec_num      = emb->vec_num;
    t            = emb->t_mat;
    t_pre        = emb->t_pre_mat;
    co_val       = emb->co_val_mat;
    pre_val      = emb->pre_val_mat;
    use_in_lib   = emb->use_in_lib;
    use_in_pre   = emb->use_in_pre;
    addit_val    = emb->addit_mat;

    n_pre_val    = emb->n_pre_val;
    e            = emb->e;
    n_addit_val  = emb->n_addit_val;

    l_co_var_num = make_var_num (emb);

    if (bundle_set)
    {
        long   *bundle_idx, idx;

        bundle_idx = bundle_set->idx;

        for (point = l_all_points; point < l_all_points + l_n_points; point++)
        {
            idx = *bundle_idx;

            point->use_in_lib = use_in_lib? *(use_in_lib + idx): true;
            point->use_in_pre = use_in_pre? *(use_in_pre + idx): true;
            point->vec_num    = *(vec_num + idx);
            point->id         = id? *(id + idx): NULL;
            point->t          = t + idx * e;
            point->t_pre      = t_pre + idx * n_pre_val;
            point->co_val     = co_val + idx * e;
            point->pre_val    = pre_val + idx * n_pre_val;
            point->co_var_num = l_co_var_num;
            point->addit_val  = addit_val ? addit_val + idx * n_addit_val: NULL;

            bundle_idx++;
        }
    }
    else
    {
        for (point = l_all_points; point < l_all_points + l_n_points; point++)
        {
            point->use_in_lib = use_in_lib? *use_in_lib: true;
            point->use_in_pre = use_in_pre? *use_in_pre: true;
            point->vec_num    = *vec_num;
            point->id         = id? *id: NULL;
            point->t          = t;
            point->t_pre      = t_pre;
            point->co_val     = co_val;
            point->pre_val    = pre_val;
            point->co_var_num = l_co_var_num;
            point->addit_val  = addit_val? addit_val: NULL;

            if (id) id++;
            vec_num++;
            if (use_in_lib) use_in_lib++;
            if (use_in_pre) use_in_pre++;
            t       += e;
            t_pre   += n_pre_val;
            co_val  += e;
            pre_val += n_pre_val;
            if (addit_val) addit_val += n_addit_val;
        }
    }

    return 0;
}

static Tpoint_set *
init_set (Tembed *emb)
{
    Tpoint_set *set;
    set = (Tpoint_set *) malloc (sizeof (Tpoint_set));

    set->point       = NULL;
    set->e           = emb->e;
    set->n_pre_val   = emb->n_pre_val;
    set->n_point     = emb->n_row;
    set->set_num     = -1;
    set->n_addit_val = emb->n_addit_val;

    return set;
}

static int
shrink_points (Tpoint_set *set, int n)
{
    set->point = (Tpoint **) realloc (set->point, n * sizeof (Tpoint *));
    set->n_point = n;
}


static int
shuffle (int n, int *values)
{
    int i, j, swap;

    for (i = n - 1; i > 0; i--)
    {
        j = rand() % (i + 1); /*skewed, but will do*/
        swap = values[i];
        values[i] = values[j];
        values[j] = swap;
    }
}

int
fill_rnd_point_twice (Tpoint ** rnd_point_twice, Tpoint *all_points, int n_points)
{
    int *rnd_index;
    int i;

    rnd_index = (int *) malloc (n_points * sizeof (int));

    for (i = 0; i < n_points; i++)
        rnd_index[i] = i;

    shuffle (n_points, rnd_index);

    for (i = 0; i < 2 * n_points; i++)
        rnd_point_twice[i] = all_points + rnd_index[i % n_points];

    free (rnd_index);
    return 0;
}

/* convergence graph methods *******************************************/
static int              l_lib_size_min, l_lib_size_max, l_lib_inc, l_lib_inc_start, l_n_bootstrap;
static float            l_lib_inc_inc_factor, l_f_lib_inc;
static Tlib_shift_meth  l_lib_shift_meth;
static int              l_lib_size = 0, l_lib_shift = 0, l_shift, l_i_boot = 0;
static int              l_permut_swaps;

int
init_set_convergent_lib (int lib_size_min, int lib_size_max, int lib_inc, float lib_inc_inc_factor,
                         Tlib_shift_meth lib_shift_meth, int lib_shift, int n_bootstrap,
                         Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set)
{
    l_lib_size_min       = lib_size_min;
    l_lib_size_max       = lib_size_max;
    l_lib_inc_start      = lib_inc < 1? 1: lib_inc;
    l_lib_inc_inc_factor = lib_inc_inc_factor;
    l_lib_shift_meth     = lib_shift_meth;
    l_lib_shift          = lib_shift < 1? 1: lib_shift;
    l_n_bootstrap        = n_bootstrap > 0? n_bootstrap: 1;

    *new_sets = &new_sets_convergent_lib;
    *next_set = &next_set_convergent_lib;
    *free_set = &free_sets;

    return 0;
}

static int l_old_lib_size;
int
new_sets_convergent_lib (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set)
{
    int i;

    if (make_sets (emb, bundle_set) < 0)
    {
        free_sets_convergent_lib ();
        return -1;
    }

    if (l_lib_size_min > l_n_points && l_n_bootstrap != 1)
    {
        free_sets_convergent_lib ();
        return -2;
    }

    if (l_lib_size_min > l_lib_size_max)
    {
        free_sets_convergent_lib ();
        return -3;
    }

    srand(1);

    l_lib_size = l_lib_size_min;
    l_shift    = 0;
    l_i_boot   = 0;
    l_set_num  = 0;
    l_old_lib_size = -1;

    l_f_lib_inc = l_lib_inc_start;
    l_lib_inc   = l_lib_inc_start;

    l_pre_set = init_set (emb);
    l_pre_set->point = (Tpoint **) malloc (l_n_points * sizeof (Tpoint *));
    for (i = 0; i < l_n_points; i++)
        l_pre_set->point[i] = l_all_points + i;
    l_pre_set->n_point = l_n_points;

    l_lib_set = init_set (emb);

    switch (l_lib_shift_meth)
    {
        case LIB_SHIFT_RANDOM:
            l_rnd_point_twice = (Tpoint **) calloc (2 * l_n_points, sizeof (Tpoint *));
            fill_rnd_point_twice (l_rnd_point_twice, l_all_points, l_n_points);
            l_lib_set->n_point = l_lib_size;
            break;
        case LIB_SHIFT_SHIFT:
            l_all_point_twice = (Tpoint **) calloc (2 * l_n_points, sizeof (Tpoint *));

            for (i = 0; i < 2 * l_n_points; i++)
                l_all_point_twice[i] = l_all_points + i % l_n_points;
            l_lib_set->n_point = l_lib_size;
            break;
        case LIB_SHIFT_BOOTSTRAP:
            l_all_point_rnd = (Tpoint **) calloc (l_lib_size_max, sizeof (Tpoint *));
            l_lib_set->point = l_all_point_rnd;
            l_lib_set->n_point = l_lib_size;
            break;
        case LIB_SHIFT_BOOT_PERMUT:
            if (l_lib_size_max > l_n_points)
                l_lib_size_max = l_n_points;
            l_all_point_rnd = (Tpoint **) calloc (l_n_points, sizeof (Tpoint *));
            l_lib_set->point = l_all_point_rnd;
            l_lib_set->n_point = l_lib_size;
            for (i = 0; i < l_n_points; i++)
                l_all_point_rnd[i] = l_all_points + i;
            /*l_permut_swaps = l_lib_size / log ((double) l_n_bootstrap);*/
            l_permut_swaps = l_lib_size / 2 + 1;
            break;
    }

    *pre_set = l_pre_set;
    *lib_set = l_lib_set;

    return next_set_convergent_lib ();
}

int
next_set_convergent_lib ()
{
    int    i, idx1, idx2;
    Tpoint *sav_point;

    if ( ((l_lib_shift_meth == LIB_SHIFT_RANDOM || l_lib_shift_meth == LIB_SHIFT_SHIFT) &&
          l_shift >= l_n_points) ||
         ((l_lib_shift_meth == LIB_SHIFT_BOOTSTRAP || l_lib_shift_meth == LIB_SHIFT_BOOT_PERMUT) &&
          l_i_boot >= l_n_bootstrap)
       )
    {
        /*increase library size*/
        l_old_lib_size = l_lib_size;
        l_lib_size += l_lib_inc;
        /*if lib_inc_inc_factor > 0.0 => also increase factor*/
        l_f_lib_inc  += l_lib_inc_inc_factor * l_f_lib_inc;
        l_lib_inc = l_f_lib_inc;

        /*Make sure max lib_size is used*/
        if (l_lib_size > l_lib_size_max && l_old_lib_size < l_lib_size_max)
            l_lib_size = l_lib_size_max;

        l_lib_set->n_point = l_lib_size;
        l_shift = 0;
        l_i_boot = 0;

        /*l_permut_swaps = l_lib_size / log ((double) l_n_bootstrap) + 1;*/
        l_permut_swaps = l_lib_size / 2 + 1;
    }


    if (l_lib_size > l_lib_size_max ||
        (l_lib_size >= l_n_points && l_lib_shift_meth != LIB_SHIFT_BOOTSTRAP) )
    {
        free_sets_convergent_lib ();
        return 1;   /*end of iterations*/
    }

    l_pre_set->set_num = l_set_num;
    l_lib_set->set_num = l_set_num;

    log_set_par_convergent_lib ();

    switch (l_lib_shift_meth)
    {
        case LIB_SHIFT_RANDOM:
            l_lib_set->point = l_rnd_point_twice + l_shift;
            l_shift += l_lib_shift; /*prepare next shift through lib*/
            break;
        case LIB_SHIFT_SHIFT:
            l_lib_set->point = l_all_point_twice + l_shift;
            l_shift += l_lib_shift; /*prepare next shift through lib*/
            break;
        case LIB_SHIFT_BOOTSTRAP:
            for (i = 0; i < l_lib_size; i++)
                l_all_point_rnd[i] = l_all_points + (rand() % l_n_points);
            l_i_boot++;
            break;
        case LIB_SHIFT_BOOT_PERMUT:
            if (l_lib_size >= l_n_points)
                break;
            for (i = 0; i < l_permut_swaps; i++)
            {
                idx1 = rand () % l_lib_size;
                idx2 = l_lib_size + rand () % (l_n_points - l_lib_size);
                sav_point = l_all_point_rnd[idx1];
                l_all_point_rnd[idx1] =  l_all_point_rnd[idx2];
                l_all_point_rnd[idx2] = sav_point;
            }
            l_i_boot++;
            break;
    }

    log_set (LOG_LIB_SET, l_lib_set);
    log_set (LOG_PRE_SET, l_pre_set);
    l_set_num++;

    return 0; /*successful iteration*/
}

int
free_sets_convergent_lib ()
{
    if (l_pre_set && l_pre_set->point)
        free (l_pre_set->point);
    free_sets ();
    return 0;
}

/* k_fold validation ***************************************************/
static int         l_k_fold;
static int         l_k;
static int         l_repetition;
static int         l_n_repetition;

int
init_set_k_fold (int k_fold, int n_repetition, /*set n_repetition to 1 for regular k-fold*/
                 Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set)
{
    *new_sets = &new_sets_k_fold;
    *next_set = &next_set_k_fold;
    *free_set = &free_sets;

    l_k_fold       = k_fold;
    l_n_repetition = n_repetition;

    return 0;
}

int
new_sets_k_fold (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set)
{
    if (make_sets (emb, bundle_set) < 0)
    {
        free_sets ();
        return -1;
    }

    if (l_k_fold > l_n_points)
    {
        free_sets ();
        *lib_set = NULL;
        *pre_set = NULL;
        return -2;
    }

    l_k          = 0;
    l_set_num    = 0;
    l_repetition = 0;

    l_rnd_point_twice = (Tpoint **) calloc (2 * l_n_points, sizeof (Tpoint *));

    *lib_set = l_lib_set = init_set (emb);
    *pre_set = l_pre_set = init_set (emb);

    srand(1);

    return next_set_k_fold ();
}

int
next_set_k_fold ()
{
    int    n_pre;
    Tpoint **pre_begin, **pre_end;

    if (l_k == l_k_fold)
    {
        l_repetition++;
        l_k = 0;
    }

    if (l_repetition == l_n_repetition)
        return 1; /*end of iterations*/

    if (l_k == 0)
        fill_rnd_point_twice (l_rnd_point_twice, l_all_points, l_n_points);

    pre_begin = l_rnd_point_twice + (int) floor (((double) l_k / l_k_fold) * l_n_points);
    pre_end   = l_rnd_point_twice + (int) floor (((double) (l_k + 1) / l_k_fold) * l_n_points);

    n_pre = pre_end - pre_begin;

    l_pre_set->point = pre_begin;
    l_pre_set->n_point = n_pre;
    l_pre_set->set_num = l_set_num;
    l_lib_set->point = pre_end + 1;
    l_lib_set->n_point = l_n_points - n_pre;
    l_lib_set->set_num = l_set_num;

    log_set_par_k_fold ();

    log_set (LOG_LIB_SET, l_lib_set);
    log_set (LOG_PRE_SET, l_pre_set);

    /*Prepare for next round*/
    l_k++;
    l_set_num++;

    return 0;
}

/* looc validation *****************************************************/
int
init_set_looc (Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set)
{
    *new_sets = &new_sets_looc;
    *next_set = &next_set_looc;
    *free_set = &free_sets;

    return 0;
}

int
new_sets_looc (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set)
{
    int i;

    /* Note: exclusion of predictee from library set is done by exclusion function
     *       during predictee set traversal.
     */

    if (make_sets (emb, bundle_set) < 0)
    {
        free_sets_looc ();
        return -1;
    }

    l_lib_set = init_set (emb);
    l_pre_set = init_set (emb);

    l_set_num = 0;

    l_lib_set->point = (Tpoint **) malloc (l_n_points * sizeof (Tpoint *));
    for (i = 0; i < l_n_points; i++)
        l_lib_set->point[i] = l_all_points + i;
    l_lib_set->n_point = l_n_points;
    l_lib_set->set_num = l_set_num;

    l_pre_set->point = (Tpoint **) malloc (l_n_points * sizeof (Tpoint *));
    for (i = 0; i < l_n_points; i++)
        l_pre_set->point[i] = l_all_points + i;
    l_pre_set->n_point = l_n_points;
    l_pre_set->set_num = l_set_num;

    *lib_set = l_lib_set;
    *pre_set = l_pre_set;

    log_set_par_looc ();

    log_set (LOG_LIB_SET, l_lib_set);
    log_set (LOG_PRE_SET, l_pre_set);

    return 0;
}

int
next_set_looc ()
{
    free_sets_looc ();
    return 1; /*since there is only one set => end of sets*/
}

int
free_sets_looc (void)
{
    if (l_pre_set && l_pre_set->point)
        free (l_pre_set->point);
    if (l_lib_set && l_lib_set->point)
        free (l_lib_set->point);
    free_sets ();
}

/* bootstrap validation *****************************************************/
static int l_n_addit_val;

typedef struct
{
    Tpoint *point;
    double *addit_val;
} Tsort_struct;

typedef struct
{
    Tsort_struct *begin;
    int          n;
} Tsort_addit_group;

static int
compare_addit_val (const Tsort_struct *s1, const Tsort_struct *s2)
{
    double *d1, *d2;
    int    i;
    d1 = s1->addit_val;
    d2 = s2->addit_val;
    for (i = 0; i < l_n_addit_val; i++)
    {
        if (*d1 < *d2)
            return -1;
        else if (*d1 > *d2)
            return 1;
        d1++; d2++;
    }

    return 0;
}

static bool l_pre_set_is_lib       = false;
static bool l_lib_size_is_emb_size = true;
static bool l_per_addit_group      = false;
static Tsort_struct      *l_sort_structs     = NULL;
static Tsort_addit_group *l_sort_addit_group = NULL;
static int  l_n_addit_group  = 0;

int
init_set_bootstrap (int lib_size, int n_bootstrap, bool pre_set_is_lib, bool lib_size_is_emb_size, bool per_addit_group,
                    Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set)
{
    *new_sets = &new_sets_bootstrap;
    *next_set = &next_set_bootstrap;
    *free_set = &free_sets;

    l_pre_set_is_lib  = pre_set_is_lib;
    l_lib_size        = lib_size;
    l_n_bootstrap     = n_bootstrap;
    l_per_addit_group = per_addit_group;

    l_lib_size_is_emb_size = lib_size_is_emb_size;

    if (l_n_bootstrap < 1)
        return -1;

    if (l_per_addit_group && !l_lib_size_is_emb_size)
        return -2;

    return 0;
}

int
new_sets_bootstrap (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set)
{
    int i;
    /* Note: exclusion of predictee from library set is done by exclusion function
     *       during predictee set traversal.
     * Use complete set as predictee set.
     */

    if (make_sets (emb, bundle_set) < 0)
    {
        free_sets_bootstrap ();
        return -1;
    }

    srand (1);

    l_lib_set = init_set (emb);
    l_pre_set = init_set (emb);

    l_set_num = 0;
    l_i_boot  = 0;

    if (l_lib_size_is_emb_size)
        l_lib_size = l_n_points;

    l_lib_set->point = (Tpoint **) malloc (l_lib_size * sizeof (Tpoint *));
    l_lib_set->n_point = l_lib_size;

    if (l_pre_set_is_lib)
    {
        l_pre_set->point   = l_lib_set->point;
        l_pre_set->n_point = l_lib_size;
    }
    else /*use complete embedding as prediction set*/
    {
        l_pre_set->point = (Tpoint **) malloc (l_n_points * sizeof (Tpoint *));
        for (i = 0; i < l_n_points; i++)
            l_pre_set->point[i] = l_all_points + i;
        l_pre_set->n_point = l_n_points;
    }

    if (l_per_addit_group) /*bootstrapping is done per group of additional values, keeping the same number of points per group.*/
    {
        int n_points_in_group, j;
        bool first_group;

        l_sort_structs = (Tsort_struct *) realloc (l_sort_structs, l_lib_size * sizeof (Tsort_struct));

        for (i = 0; i < l_lib_size; i++)
        {
            l_sort_structs[i].point     = l_all_points + i;
            l_sort_structs[i].addit_val = l_all_points[i].addit_val;
        }

        /* Sort according to additional values.*/
        l_n_addit_val = emb->n_addit_val;
        qsort (l_sort_structs, l_lib_size, sizeof (Tsort_struct),
                (int (*) (const void *, const void *)) compare_addit_val);

        /* Determine number of points per group, and create group structures.*/
        first_group = true;
        l_n_addit_group = 0;
        for (i = 0; i < l_lib_size; i++)
        {
            if (first_group || compare_addit_val (l_sort_structs + i, l_sort_structs + i - 1) != 0)
            {
                /*start new group*/
                l_sort_addit_group = (Tsort_addit_group *) realloc (l_sort_addit_group,
                                                                    (l_n_addit_group + 1) * sizeof (Tsort_addit_group));
                first_group = false;
                l_n_addit_group++;
                l_sort_addit_group[l_n_addit_group - 1].begin = l_sort_structs + i;
                l_sort_addit_group[l_n_addit_group - 1].n     = 0;
            }

            l_sort_addit_group[l_n_addit_group - 1].n++;
        }
    }

    *lib_set = l_lib_set;
    *pre_set = l_pre_set;

    return next_set_bootstrap ();
}

int
next_set_bootstrap ()
{
    int i, j, k;

    if (l_i_boot == l_n_bootstrap)
    {
        free_sets_bootstrap ();
        return 1; /*end of itterations*/
    }

    if (l_per_addit_group)
    {
        k = 0;
        for (j = 0; j < l_n_addit_group; j++)
        {
            for (i = 0; i < l_sort_addit_group[j].n; i++)
            {
                l_lib_set->point[k] = (l_sort_addit_group[j].begin + rand () % l_sort_addit_group[j].n)->point;
                k++;
            }
        }
    }
    else
        for (i = 0; i < l_lib_size; i++)
            l_lib_set->point[i] = l_all_points + rand () % l_n_points;

    l_pre_set->set_num = l_set_num;
    l_lib_set->set_num = l_set_num;

    log_set_par_bootstrap ();

    log_set (LOG_LIB_SET, l_lib_set);
    log_set (LOG_PRE_SET, l_pre_set);

    l_set_num++; /*prepare for next round*/
    l_i_boot++;

    return 0;
}

int
free_sets_bootstrap (void)
{
    if (l_pre_set && l_pre_set->point && !l_pre_set_is_lib)
        free (l_pre_set->point);
    if (l_lib_set && l_lib_set->point)
        free (l_lib_set->point);
    if (l_sort_structs)
        free (l_sort_structs);
    l_sort_structs = NULL;
    if (l_sort_addit_group)
        free (l_sort_addit_group);
    l_sort_addit_group = NULL;
    l_n_addit_group = 0;
    free_sets ();
}

/* user defined validation *****************************************************/
int
init_set_user_val (Tnew_sets *new_sets, Tnext_set *next_set, Tfree_set *free_set)
{
    *new_sets = &new_sets_user_val;
    *next_set = &next_set_user_val;
    *free_set = &free_sets;

    return 0;
}

int
new_sets_user_val (Tembed *emb, Tbundle_set *bundle_set, Tpoint_set **lib_set, Tpoint_set **pre_set)
{
    /* The user has defined (in the file) which time indices should be included in lib,
     * and which ones in the prediction set.
     */
    Tpoint **lib_point, **pre_point, *point;
    int    i, n_lib, n_pre;

    if (make_sets (emb, bundle_set) < 0)
    {
        free_sets_user_val ();
        return -1;
    }

    l_set_num = 0;

    l_lib_set = init_set (emb);
    l_pre_set = init_set (emb);

    /* Initially assume all points in set.*/
    l_lib_set->point = (Tpoint **) malloc (emb->n_row * sizeof (Tpoint *));
    l_pre_set->point = (Tpoint **) malloc (emb->n_row * sizeof (Tpoint *));

    lib_point = l_lib_set->point;
    pre_point = l_pre_set->point;

    l_lib_set->set_num = l_set_num;
    l_pre_set->set_num = l_set_num;

    n_lib = n_pre = 0;
    for (i = 0; i < l_n_points; i++)
    {
        point = l_all_points + i;

        if (point->use_in_lib)
        {
            *lib_point = point;
            n_lib++;
            lib_point++;
        }

        if (point->use_in_pre)
        {
            *pre_point = point;
            n_pre++;
            pre_point++;
        }
    }

    if (n_lib == 0 || n_pre == 0)
    {
        free_sets_user_val ();
        return (-1);
    }

    /* Shrink point arrays*/
    shrink_points (l_lib_set, n_lib);
    shrink_points (l_pre_set, n_pre);

    *lib_set = l_lib_set;
    *pre_set = l_pre_set;

    log_set_par_user_val ();

    log_set (LOG_LIB_SET, l_lib_set);
    log_set (LOG_PRE_SET, l_pre_set);

    return 0;
}

int
next_set_user_val (void)
{
    free_sets_user_val ();
    return 1; /*since there is only one set => end of sets*/
}

int
free_sets_user_val (void)
{
    if (l_pre_set && l_pre_set->point)
        free (l_pre_set->point);
    if (l_lib_set && l_lib_set->point)
        free (l_lib_set->point);
    free_sets ();
}

int
log_set_par_convergent_lib ()
{
    static struct s_log_spcl log_spcl;
#ifdef LOG_HUMAN
    ATTACH_META_SPCL(meta_log_spcl, log_spcl);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_SET_PAR))
        return 2;

    log_spcl.lib_size = l_lib_size;
    log_spcl.shift    = l_shift;
    log_spcl.boot_i   = l_i_boot;
    LOGREC(LOG_SPCL, &log_spcl, sizeof (log_spcl), &meta_log_spcl);

    return 0;
}

int
log_set_par_k_fold ()
{
    static struct s_log_spkf log_spkf;
#ifdef LOG_HUMAN
    ATTACH_META_SPKF(meta_log_spkf, log_spkf);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_SET_PAR))
        return 2;

    log_spkf.k = l_k;
    log_spkf.repetition = l_repetition;
    LOGREC(LOG_SPKF, &log_spkf, sizeof (log_spkf), &meta_log_spkf);

    return 0;
}

int
log_set_par_looc ()
{
    static struct s_log_splo log_splo;
#ifdef LOG_HUMAN
    ATTACH_META_SPLO(meta_log_splo, log_splo);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_SET_PAR))
        return 2;

    log_splo.dummy = 0;
    LOGREC(LOG_SPLO, &log_splo, sizeof (log_splo), &meta_log_splo);

    return 0;
}

int
log_set_par_bootstrap ()
{
    static struct s_log_spbt log_spbt;
#ifdef LOG_HUMAN
    ATTACH_META_SPBT(meta_log_spbt, log_spbt);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_SET_PAR))
        return 2;

    log_spbt.boot_i = l_i_boot;
    LOGREC(LOG_SPBT, &log_spbt, sizeof (log_spbt), &meta_log_spbt);

    return 0;
}

int
log_set_par_user_val ()
{
    static struct s_log_spuv log_spuv;
#ifdef LOG_HUMAN
    ATTACH_META_SPUV(meta_log_spuv, log_spuv);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_SET_PAR))
        return 2;

    log_spuv.dummy = 0;
    LOGREC(LOG_SPUV, &log_spuv, sizeof (log_spuv), &meta_log_spuv);

    return 0;
}

int
log_set (Tlog_levels log_level, Tpoint_set *set)
{
    char set_code[2];
    int  i;
    Tpoint **pt;
    static struct s_log_sh log_sh;
    static struct s_log_sd log_sd;
#ifdef LOG_HUMAN
    ATTACH_META_SH(meta_log_sh, log_sh);
    ATTACH_META_SD(meta_log_sd, log_sd);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & log_level))
        return 2;

    log_sh.set_code = log_level == LOG_LIB_SET? 'L': 'P';
    log_sh.set_num  = set->set_num;
    log_sh.n_point  = set->n_point;
    LOGREC(LOG_SH, &log_sh, sizeof (log_sh), &meta_log_sh);

    pt = set->point;
    for (i = 0; i < set->n_point; i++)
    {
        log_sd.set_code = log_sh.set_code;
        log_sd.vec_idx  = i;
        log_sd.vec_num  = (*pt)->vec_num;
        LOGREC(LOG_SD, &log_sd, sizeof (log_sd), &meta_log_sd);

        pt++;
    }

    fflush (g_log_file);
    return 0;
}
