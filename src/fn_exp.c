/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>

#include "kdt.h"     //k-d tree implementation.
#include "point.h"
#include "fn.h"
#include "fn_exp.h"
#include "log.h"
#include "fn_exp_log.h"

int
new_fn_params_exponential (int e, void **fn_params);

int
next_fn_params_exponential (void **fn_params);

int
fn_exponential (Tpoint_set *lib_set, Tpoint_set *pre_set, double **predicted);

int
log_fn_params_exponential (void);

int
est_pre_val (Tpoint **rs, double *sqdst, int n, int n_pre_val, double *pre_val, double rms_dist);

static int   l_nnn_add;    /* Number of nearest neighbours to add to embedding dimension.*/
static Texcl l_excl;       /* How to exclude vectors from library, based upon predictor vector.*/
static int   l_var_win;
static void  *l_fn_params = NULL;
static Tfn_denom l_fn_denom;
static double l_exp_k;


/* Exponential fn: neighbour values are exponentially weighted to make a prediction for target value*/
int
init_fn_exponential (Tnew_fn_params *new_fn_params, Tnext_fn_params *next_fn_params, Tfn *fn,
                         int nnn_add, Texcl excl, int var_win,
                         Tfn_denom fn_denom, double exp_k)
{
    l_nnn_add = nnn_add;
    l_excl    = excl;
    l_var_win = var_win;
    l_exp_k   = exp_k;
    l_fn_denom = fn_denom;

    *new_fn_params  = &new_fn_params_exponential;
    *next_fn_params = &next_fn_params_exponential;
    *fn             = &fn_exponential;

    log_fn (FN_EXP);

    return 0;
}

static bool new_fn_params_first;

int
new_fn_params_exponential (int e, void **fn_params)
{
    exclude_init (l_excl, l_var_win, e);
    new_fn_params_first = true;
    return next_fn_params_exponential (fn_params);
}

static double *l_pre_val = NULL;
static int    l_n_set = 0;
static double *l_u = NULL;

int
next_fn_params_exponential (void **fn_params)
{
    Tfn_params_exponential *_fn_params;

    log_fn_params_exponential ();

    /* currently, no traversable parameters for exponential fn function*/
    /* NOTE when there ARE: make sure that for each traversion, new memory is allocated */
    if (new_fn_params_first)
    {
        _fn_params = (Tfn_params_exponential *) malloc (sizeof(Tfn_params_exponential));
        _fn_params->nnn_add = l_nnn_add;
        _fn_params->excl    = l_excl;
        _fn_params->var_win = l_var_win;

        *fn_params = (void *) _fn_params;

        new_fn_params_first = false;
        return 0; /* one and only set */
    }
    else
    {
        *fn_params = NULL;

        if (l_pre_val)
            free (l_pre_val);
        l_pre_val = NULL;

        if (l_u)
            free (l_u);
        l_u = 0;
        l_n_set = 0;

        return 1; /* stop, no new fn parameters to traverse*/
    }
}

double
get_rms_dist (Tpoint **p, int n_point, int e)
{
    Tpoint **p1, **p2;
    long   n = 0;
    int    i;
    double dist, sqdist, mean_ssq_dist = 0.0;

    for (p1 = p; p1 < p + n_point - 1; p1++)
        for (p2 = p1 + 1; p2 < p + n_point; p2++)
        {
            sqdist = 0.0;
            for (i = 0; i < e; i++)
            {
                dist = (*p1)->co_val[i] - (*p2)->co_val[i];
                sqdist += dist * dist; 
            }
            mean_ssq_dist += (sqdist - mean_ssq_dist) / (n + 1);
            n++;
        }
    return sqrt (mean_ssq_dist);
}

bool
full_set (Tpoint **set, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (!*(set + i))
            return false;
    return true;
}

void
set_prediction_to_invalid (double *pre_val, int n_pre_val)
{
    int i;
    for (i = 0; i < n_pre_val; i++)
        pre_val[i] = NAN;
}

int
fn_exponential (Tpoint_set *lib_set, Tpoint_set *pre_set, double **predicted)
{
    TkdtNode *tx;
    double   *sqdst;
    Tpoint   **rs;       /*result set*/
    Tpoint   **target;
    double   *p_pre_val;
    double   rms_dist;
    int      nnn;        /*N nearest neighbours*/
    int      res;

    nnn = lib_set->e + l_nnn_add; 

    rs    = (Tpoint **) malloc (nnn * sizeof(Tpoint *));
    sqdst = (double *) malloc (nnn * sizeof(double));

    rms_dist = 0.0;
    if (l_fn_denom == FN_WEIGHT_DENOM_AVG_LIB)
        rms_dist = get_rms_dist (lib_set->point, lib_set->n_point, lib_set->e);

    if (l_pre_val)
        free (l_pre_val);
    l_pre_val = (double *) malloc (pre_set->n_point * pre_set->n_pre_val * sizeof(double));

    tx = kdtree ((void **) lib_set->point, lib_set->n_point, lib_set->e, (double * (*)(void *))get_co_vec);

    p_pre_val = l_pre_val;
    for (target = pre_set->point; target < pre_set->point + pre_set->n_point; target++)
    {
        /*find nearest neighbours*/
        kdt_nn ((void *)(*target), tx, lib_set->e, nnn, 
               (void **) rs, sqdst, (double * (*)(void *))get_co_vec, (bool (*)(void *, void *))exclude, true); 
        log_nn (*target, rs, sqdst, nnn);

        if (full_set (rs, nnn))
        {
            if (l_fn_denom == FN_WEIGHT_DENOM_AVG_NN)
                rms_dist = get_rms_dist (rs, nnn, lib_set->e);

            if ((res = est_pre_val (rs, sqdst, nnn, pre_set->n_pre_val, p_pre_val, rms_dist)) != 0)
                set_prediction_to_invalid (p_pre_val, pre_set->n_pre_val);
        }
        else
            set_prediction_to_invalid (p_pre_val, pre_set->n_pre_val);

        p_pre_val += pre_set->n_pre_val;
    }

    free_kdt (tx);

    free (rs); free (sqdst);

    log_predicted (pre_set->point, pre_set->n_point, pre_set->n_pre_val, l_pre_val);

    fflush (g_log_file);

    *predicted = l_pre_val;
    return 0;
}

int
est_pre_val (Tpoint **rs, double *sqdst, int n, int n_pre_val, double *pre_val, double rms_dist)
{
    double dst0, dst;
    double sum_u, sum_u_inv, val_est, mp;
    int    i_pre_val, i;

    if (l_n_set != n)
    {
        l_u     = (double *) realloc (l_u, n * sizeof(double));
        l_n_set = n;
    }

    if (l_fn_denom == FN_WEIGHT_DENOM_MINIMUM)
    {
        /*find first non-zero distance, results ordered large to small*/
        for (i = n - 1; i > -1; i--)
            if ((dst = sqrt(sqdst[i])) > 0)
                break;
        dst0 = dst;
    }
    else
        dst0 = rms_dist;

    /* If no non-zero distance found, set dst0 (the numerator in the exponent) to 1.
     * Since all distances are zero in that case, each neighbor will then get a weight of 1.
     */
    if (dst0 == 0.0)
        dst0 = 1.0;

    mp = -l_exp_k / dst0;

    sum_u = 0.0;

    for (i = 0; i < n; i++)
    {
        dst = sqrt (sqdst[i]);
        l_u[i] = exp (mp * dst);
        sum_u += l_u[i];
    }

    if (sum_u == 0.0)
        return -1; /*weights are too small, very unlikely, but still...*/

    sum_u_inv = 1.0 / sum_u;

    for (i_pre_val = 0; i_pre_val < n_pre_val; i_pre_val++)
    {
        val_est = 0.0;
        for (i = 0; i < n; i++)
            val_est += l_u[i] * sum_u_inv * rs[i]->pre_val[i_pre_val];

        pre_val[i_pre_val] = val_est;
    }

    return 0;
}

int
log_fn_params_exponential (void)
{
    static struct s_log_kpexp log_kpexp;
#ifdef LOG_HUMAN
    ATTACH_META_KPEXP(meta_log_kpexp, log_kpexp);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_FN_PAR))
        return 2;

    log_kpexp.nnn_add = l_nnn_add;
    log_kpexp.excl = (int) l_excl;
    log_kpexp.var_win = l_var_win;
    log_kpexp.denom_type = (int) l_fn_denom;
    log_kpexp.exp_k = l_exp_k;

    LOGREC(LOG_KPEXP, &log_kpexp, sizeof (log_kpexp), &meta_log_kpexp);

    return 0;
}

