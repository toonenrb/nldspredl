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
#include "fn_tls.h"
#include "log.h"
#include "fn_tls_log.h"

static int    l_nnn = 0;    /* When greater than 0, TLS / SVD only done on l_nnn nearest neighbours (for large datasets) */
static Texcl  l_excl;       /* How to exclude vectors from library, based upon predictor vector.*/
static int    l_var_win;
static double l_theta_min, l_theta_max, l_delta_theta;
static void   *l_fn_params = NULL;
static bool   l_center = false;
static double l_restrict_prediction;
static Ttls_ref_meth l_ref_meth;
static int    l_ref_xnn;
static double *l_shortest_dist = NULL;
static bool   l_warn_is_error;
static bool   l_object_only_once;

int
new_fn_params_tls (int e, void **fn_params);

int
next_fn_params_tls (void **fn_params);

int
fn_tls (Tpoint_set *lib_set, Tpoint_set *pre_set, double **predicted);

static void
free_tls (void);

int
log_fn_params_tls (void);

int
log_var_params (Tpoint *target, int e, int n_pre_val, int ldx, double *var_par, bool center, double *means);

int
log_dtls (double tol1, double tol2, int ldx, int rank, int ierr,
          int iwarn, int ldc, int m, int n, int l,
          double *c, double *x, double *s);

static void
init_xnn (void);

extern void
dtls_ (double * aug_mat, int * ldc, int * n_points, int * n_a, int * n_b, 
       double * l_s, double * l_x, int * _ldx, double * l_wrk, int * rank, 
       double * tol1, double * tol2, char * comprt, int * ierr, int * iwarn);

int
init_fn_tls (Tnew_fn_params *new_fn_params,
                 Tnext_fn_params *next_fn_params,
                 Tfn *fn,
                 double theta_min, double theta_max, double delta_theta,
                 int nnn, Texcl excl, int var_win, bool center, double restrict_prediction,
                 bool warn_is_error, Ttls_ref_meth ref_meth, int ref_xnn, bool object_only_once)
{
    l_nnn     = nnn;
    l_excl    = excl;
    l_var_win = var_win;

    l_theta_min   = theta_min;
    l_theta_max   = theta_max;
    l_delta_theta = delta_theta;

    l_center      = center;

    l_ref_meth    = ref_meth;
    l_ref_xnn     = ref_xnn;
    
    l_warn_is_error = warn_is_error;
    l_object_only_once = object_only_once;

    if (l_ref_meth == KTLS_REFMETH_XNN_GT_ZERO && l_ref_xnn < 1)
    {
        fprintf (stderr, "Number of values for reference distance too small <%d>\n", ref_xnn);
        return -1;
    }

    l_restrict_prediction = restrict_prediction;

    *new_fn_params  = &new_fn_params_tls;
    *next_fn_params = &next_fn_params_tls;
    *fn             = &fn_tls;

    log_fn (FN_TLS);

    return 0;
}

static double l_theta;

int
new_fn_params_tls (int e, void **fn_params)
{
    exclude_init (l_excl, l_var_win, e);
    l_theta = l_theta_min - 1.0;
    if (l_ref_meth == KTLS_REFMETH_XNN_GT_ZERO)
        l_shortest_dist = (double *) realloc (l_shortest_dist, l_ref_xnn * sizeof (double));
    return next_fn_params_tls (fn_params);
}

static double *l_pre_val = NULL;
static int    *l_status = NULL;

int
next_fn_params_tls (void **fn_params)
{
    Tfn_params_tls *_fn_params;


    if (l_theta < l_theta_min)
        l_theta = l_theta_min;
    else
        l_theta += l_delta_theta;

    if (l_theta > l_theta_max)
    {
        *fn_params = NULL;

        if (l_pre_val)
            free (l_pre_val);
        l_pre_val = NULL;

        if (l_status)
            free (l_status);

        free_tls ();

        return 1; /* stop, no new fn parameters to traverse*/
    }

    _fn_params = (Tfn_params_tls *) malloc (sizeof(Tfn_params_tls));
    _fn_params->nnn     = l_nnn;
    _fn_params->excl    = l_excl;
    _fn_params->var_win = l_var_win;
    _fn_params->theta   = l_theta;
    _fn_params->center  = l_center;

    *fn_params = (void *) _fn_params;

    log_fn_params_tls ();

    return 0;
}

static double
getdist (double *vec1, double *vec2, int e)
{
    double dist, sqdist;
    double *pv1, *pv2;

    for (pv1 = vec1, pv2 = vec2, sqdist = 0.0; pv1 < vec1 + e; pv1++, pv2++)
    {
        dist    = *pv1 - *pv2;
        sqdist += dist * dist;
    }

    return (sqrt (sqdist));
}

static void
init_xnn (void)
{
    int i;
    for (i = 0; i < l_ref_xnn; i++) l_shortest_dist[i] = DBL_MAX;
}

static void
add_xnn (double d)  /*d >= 0.0*/
{
    int i, j;
    /*Note: extremely simple non optimised procedure.*/
    if (d == 0.0)
        return;
    for (i = 0; i < l_ref_xnn; i++)
        if (d > l_shortest_dist[i])
            break;
    if (i > 0)
    {
        for (j = 0; j < i - 1; j++)
            l_shortest_dist[j] = l_shortest_dist[j+1];

        l_shortest_dist[i - 1] = d;
    }

    return;
}

static double
get_xnn_ref_dst ()
{
    int i, n = 0;
    double val = 0.0;
    for (i = l_ref_xnn - 1; i >= 0; i--)
    {
        if (l_shortest_dist[i] == DBL_MAX)
            break;

        val = (l_shortest_dist[i] - val) / ++n;
    }

    return val;
}

static int
fill_aug_mat (double *aug_mat, Tpoint *target, Tpoint **rs, int n_rs, double *sqdst, double *weight,
              int ldc, int e, int n_pre_val, double *means, bool center,
              Ttls_ref_meth ref_meth)
{
    double   *p1_aug_mat, *p2_aug_mat;
    double   *p_sqdst, *p_weight;
    double   avg_dst, dval, *p_vec, *vec, fact;
    int      aug_n_col, i, n;
    Tpoint   **p_pt;
    double   *p_mean;
    double   ref_dst;

    aug_n_col = e + 1 + n_pre_val;

    /* NOTE: because of Fortran routines, aug_mat will be column first order */

    n          = 0;
    avg_dst    = 0.0;
    p1_aug_mat = aug_mat;
    p_weight   = weight;
    p_sqdst    = sqdst;

    if (center)
        for (p_mean = means; p_mean < means + aug_n_col - 1; p_mean++)
            *p_mean = 0.0;

    if (ref_meth == KTLS_REFMETH_XNN_GT_ZERO)
        init_xnn ();

    n = 0;
    for (p_pt = rs; p_pt < rs + n_rs; p_pt++)
    {
        if (sqdst) /*squared distances already available*/
        {
            if (! *p_pt)
                break;
            *p_weight++ = dval = sqrt (*p_sqdst++);
        }
        else
        {
            if (target == *p_pt || exclude (target, *p_pt))
                continue;
            *p_weight++ = dval =  getdist (target->co_val, (*p_pt)->co_val, e);
        }

        avg_dst += (dval - avg_dst) / (n + 1);

        if (ref_meth == KTLS_REFMETH_XNN_GT_ZERO)
            add_xnn (dval);

        /* copy vector and prediction values into augmented matrix */

        p2_aug_mat = p1_aug_mat;
        /* first column is 1, to allow for constant */
        *p2_aug_mat = 1.0;
        p2_aug_mat += ldc;
        p_mean = means; /*first column (of 1's) is not centered.*/
        for (p_vec = vec = (*p_pt)->co_val; p_vec < vec + e; p_vec++)
        {
            *p2_aug_mat = *p_vec;
            p2_aug_mat += ldc;
            if (center)
            {
                *p_mean    += (*p_vec - *p_mean) / (n + 1);
                p_mean++;
            }
        }

        for (p_vec = vec = (*p_pt)->pre_val; p_vec < vec + n_pre_val; p_vec++)
        {
            *p2_aug_mat = *p_vec;
            p2_aug_mat += ldc;
            if (center)
            {
                *p_mean    += (*p_vec - *p_mean) / (n + 1);
                p_mean++;
            }
        }

        n++;
        p1_aug_mat++;
    }

    /* TODO check if n == 0 */

    if (center) /*subtract means from columns*/
    {
        p1_aug_mat = aug_mat + ldc; /*the constants column is not centered.*/
        for (p_mean = means; p_mean < means + aug_n_col - 1; p_mean++)
        {
            for (p2_aug_mat = p1_aug_mat; p2_aug_mat < p1_aug_mat + n; p2_aug_mat++)
                *p2_aug_mat -= *p_mean;
            p1_aug_mat += ldc;
        }
    }


    /* Compute weights and apply to augmented matrix */
    ref_dst = avg_dst;
    if (ref_meth == KTLS_REFMETH_XNN_GT_ZERO)
    {
        if ((ref_dst = get_xnn_ref_dst ()) == 0.0)
        {
            fprintf (stdout, "Warning: reference distance is 0.0, switching to overall mean distance.\n");
            ref_dst = avg_dst;
        }
    }

    fact = -1.0 * l_theta / ref_dst;
    for (p_weight = weight; p_weight < weight + n; p_weight++)
        *p_weight = exp (fact * (*p_weight));

    /* This looks a bit messy, due to the fact that the augmented matrix is in column first order */
    p1_aug_mat = aug_mat;
    for (i = 0; i < aug_n_col; i++)
    {
        p2_aug_mat = p1_aug_mat;
        for (p_weight = weight; p_weight < weight + n; p_weight++)
            *p2_aug_mat++ *= *p_weight;

        p1_aug_mat += ldc;
    }

    return n;
}


static double *l_s   = NULL;
static double *l_x   = NULL;
static double *l_wrk = NULL;

int
tls (double *aug_mat, int ldc, int n_points, int n_a, int n_b, double **_x, int *_ldx, int *err, int *warn)
{
    int rank =  -1, ierr, iwarn;
    double tol1 = 0.0000000000000001, tol2 = 0.00001;
    char comprt = 'X'; /*compute both rank and tol1*/

    static int _n_a = 0, _n_b = 0, _n_points = 0;

    /* Lazy resizing of memory.*/
    if (_n_a + _n_b < n_a + n_b || !l_s)
    {
        if (l_s)
            free (l_s);
        l_s = (double *) malloc ((n_a + n_b) * sizeof (double));
    }

    if (_n_a * _n_b < n_a * n_b || !l_x)
    {
        if (l_x)
            free (l_x);
        l_x = (double *) malloc ((n_a * n_b) * sizeof (double));
    }

    if (_n_a + _n_b + _n_points < n_a + n_b + n_points || !l_wrk)
    {
        if (l_wrk)
            free (l_wrk);
        l_wrk = (double *) malloc ((n_a + n_b + n_points) * sizeof(double));
    }

    *_ldx  = n_a;
    _n_a = n_a;
    _n_b = n_b;
    _n_points = n_points;

    dtls_ (aug_mat, &ldc, &n_points, &n_a, &n_b, l_s, l_x, _ldx, l_wrk, &rank, &tol1, &tol2, &comprt, &ierr, &iwarn);

/*    if (iwarn > 0)
 *      fprintf (stderr, "Warning: rank lowered to %d\n", rank);
 */
    log_dtls (tol1, tol2, *_ldx, rank, ierr, iwarn, ldc, n_points, n_a, n_b, aug_mat, l_x, l_s);

    *_x   = l_x;
    *err  = ierr;
    *warn = iwarn;
    return ierr;
}

static void
free_tls (void)
{
    if (l_s)
        free (l_s);
    if (l_x)
        free (l_x);
    if (l_wrk)
        free (l_wrk);
    if (l_status)
        free (l_status);

    l_s      = NULL;
    l_x      = NULL;
    l_wrk    = NULL;
    l_status = NULL;

    if (l_shortest_dist)
        free (l_shortest_dist);

    l_shortest_dist = NULL;
}

int
fn_tls (Tpoint_set *lib_set, Tpoint_set *pre_set, double **predicted)
{
    TkdtNode *tx;
    double   *sqdst, *weight;
    double   *p1_x, *aug_mat, *p_pre_val;
//    double   *c, *p1_x, *aug_mat, *p_pre_val;
    int      aug_n_col, aug_n_points, ldc, e, n_pre_val, i, j, ierr, iwarn, ldx, n_rs, n_warn;
    Tpoint   **ptarget;
    Tpoint   **rs;       /*result set*/
    double   *means = NULL;
    int      *p_status;

    e         = pre_set->e;
    n_pre_val = pre_set->n_pre_val;
    n_warn = 0;

    /* means contains the mean value per axis, over all vectors from the result set.
     * It is filled in fill_aug_matrix.
     */
    if (l_center)
        means = (double *) malloc ((e + n_pre_val) * sizeof(double));

    /* Matrix to hold predicted values */
    if (l_pre_val)
        free (l_pre_val);
    l_pre_val = (double *) malloc (pre_set->n_point * n_pre_val * sizeof(double));

    /* Array to hold status values */
    if (l_status)
        free (l_status);
    l_status = (int *) calloc (pre_set->n_point * n_pre_val,  sizeof(int));

    aug_n_col = e + 1 + n_pre_val;

    if (l_nnn > 0)
    {
        rs     = (Tpoint **) malloc (l_nnn * sizeof (Tpoint *));
        n_rs   = l_nnn;
        ldc    = aug_n_col > n_rs? aug_n_col: n_rs;  /*leading dimension of aug_mat (column-first order)*/

        sqdst  = (double *) malloc (ldc * sizeof (double));
        tx     = kdtree ((void **) lib_set->point, lib_set->n_point, e, (double * (*)(void *))get_co_vec);
    }
    else
    {
        sqdst    = NULL;
        rs       = lib_set->point;
        n_rs     = lib_set->n_point;
        ldc    = aug_n_col > n_rs? aug_n_col: n_rs;  /*leading dimension of aug_mat (column-first order)*/
    }

    aug_mat = (double *) malloc (ldc * aug_n_col * sizeof(double));
    weight  = (double *) malloc (n_rs * sizeof (double));


    p_pre_val = l_pre_val;
    p_status = l_status;
    for (ptarget = pre_set->point; ptarget < pre_set->point + pre_set->n_point; ptarget++)
    {
        if (l_nnn > 0)
        {
            /* Find nearest neighbours */
            kdt_nn ((void *)(*ptarget), tx, lib_set->e, l_nnn, 
                   (void **) rs, sqdst, (double * (*)(void *))get_co_vec, (bool (*)(void *, void *))exclude, l_object_only_once); 
            log_nn (*ptarget, rs, sqdst, l_nnn);
        }

        aug_n_points = fill_aug_mat (aug_mat, *ptarget, rs, n_rs, sqdst, weight,
                                     ldc, e, n_pre_val, means, l_center, l_ref_meth);

        if (aug_n_points == 0)
        {
            for (i = 0; i < n_pre_val; i++)
            {
                *p_pre_val++ = NAN;
                *p_status++ = STLS_AUG_N_POINTS_ZERO;
            }

            continue;
        }


        /* TLS */
        if ((ierr = tls (aug_mat, ldc, aug_n_points, e + 1, n_pre_val, &p1_x, &ldx, &ierr, &iwarn)) != 0)
        {
            /* fprintf (stderr, "Error in TLS estimation <%d>.\n", ierr); */
            for (i = 0; i < n_pre_val; i++)
            {
                *p_pre_val++ = NAN;

                if (ierr >= 1000)
                    *p_status++ = (ierr - 1000) * 0x2000 | iwarn * 0x0100 | STLS_ERROR;
                else
                    *p_status++ = ierr * 0x2000 | iwarn * 0x0100 | STLS_ERROR;
            }

            continue;
        }

        if (l_warn_is_error && iwarn > 0)
        {
            for (i = 0; i < n_pre_val; i++)
            {
                *p_pre_val++ = NAN;
                *p_status++ = iwarn * 0x0100 | STLS_WARN_IS_ERROR;
            }

            continue;
        }

        if (iwarn > 0)
        {
            n_warn++;
            for (j = 0; j < n_pre_val; j++)
                *(p_status + j) = iwarn * 0x0100 | STLS_WARNING;
        }

        log_var_params (*ptarget, e, n_pre_val, ldx, p1_x, l_center, means);

        if (l_center)
        {
            for (i = 0; i < n_pre_val; i++)
            {
                *p_pre_val = means[e + i] + p1_x[0];
                for (j = 0; j < e; j++)
                    *p_pre_val += ((*ptarget)->co_val[j] - means[j]) * p1_x[j+1];
                if (l_restrict_prediction > 0.0 &&
                        (*p_pre_val > l_restrict_prediction || *p_pre_val < -l_restrict_prediction))
                {
                    *p_pre_val = NAN;
                    *p_status |= STLS_VAL_GT_RESTRICT;
                }
                p_status++;
                p_pre_val++;
                p1_x += ldx;
            }
        }
        else
        {
            for (i = 0; i < n_pre_val; i++)
            {
                *p_pre_val = p1_x[0];
                for (j = 0; j < e; j++)
                    *p_pre_val += (*ptarget)->co_val[j] * p1_x[j+1];
                if (l_restrict_prediction > 0.0 &&
                        (*p_pre_val > l_restrict_prediction || *p_pre_val < l_restrict_prediction))
                {
                    *p_pre_val = NAN;
                    *p_status |= STLS_VAL_GT_RESTRICT;
                }
                p_status++;
                p_pre_val++;
                p1_x += ldx;
            }
        }
    }

    if (n_warn > 0)
        fprintf (stderr, "Warning: %d warnings in tls procedure.\n", n_warn);

    log_predicted (pre_set->point, pre_set->n_point, pre_set->n_pre_val, l_pre_val, l_status);

    if (l_nnn > 0)
    {
        if (rs)
            free (rs);
        if (tx)
            free_kdt (tx);
        if (sqdst)
            free (sqdst);
    }

    if (aug_mat)
        free (aug_mat);
    if (weight)
        free (weight);
    if (means)
        free (means);

    fflush (g_log_file);

    *predicted = l_pre_val;

    return 0;
}

int
log_fn_params_tls (void)
{
    static struct s_log_kptls log_kptls;
#ifdef LOG_HUMAN
    ATTACH_META_KPTLS(meta_log_kptls, log_kptls);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_FN_PAR))
        return 2;

    log_kptls.nnn = l_nnn;
    log_kptls.excl = (int) l_excl;
    log_kptls.var_win = l_var_win;
    log_kptls.theta = l_theta;
    log_kptls.center = l_center? 1: 0;
    log_kptls.object_only_once = l_object_only_once;

    LOGREC(LOG_KPTLS, &log_kptls, sizeof (log_kptls), &meta_log_kptls);

    return 0;
}

int
log_var_params (Tpoint *target, int e, int n_pre_val, int ldx, double *var_par, bool center, double *means)
{
    int    i, j;
    double *p1_var_par, *p2_var_par;
    static struct s_log_vpm log_vpm;
    static struct s_log_vpt log_vpt;
#ifdef LOG_HUMAN
    ATTACH_META_VPM(meta_log_vpm, log_vpm);
    ATTACH_META_VPT(meta_log_vpt, log_vpt);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_VAR_PAR))
        return 2;

    for (i = 0; i < e; i++)
    {
        log_vpm.copr     = 'C';
        log_vpm.var_num  = i;
        log_vpm.var_mean = means? means[i]: NAN;
        LOGREC(LOG_VPM, &log_vpm, sizeof (log_vpm), &meta_log_vpm);
    }

    for (i = 0; i < n_pre_val; i++)
    {
        log_vpm.copr     = 'P';
        log_vpm.var_num  = i;
        log_vpm.var_mean = means? means[e + i]: NAN;
        LOGREC(LOG_VPM, &log_vpm, sizeof (log_vpm), &meta_log_vpm);
    }

    p1_var_par = var_par;
    for (i = 0; i < n_pre_val; i++)
    {
        p2_var_par = p1_var_par;
        for (j = 0; j < e + 1; j++)
        {
            log_vpt.target_num  = target->vec_num;
            log_vpt.pre_val_num = i;
            log_vpt.var_coord   = j;
            log_vpt.var_val     = *p2_var_par++;
            LOGREC(LOG_VPT, &log_vpt, sizeof (log_vpt), &meta_log_vpt);
        }

        p1_var_par += ldx;
    }
    return 0;
}

int
log_dtls (double tol1, double tol2, int ldx, int rank, int ierr,
          int iwarn, int ldc, int m, int n, int l,
          double *c, double *x, double *s)
{
    static struct s_log_dtlso log_dtlso;
    static struct s_log_dtlsan log_dtlsan;
    static struct s_log_dtlsav log_dtlsav;
    double *prow, *pval;

    long row,col;
#ifdef LOG_HUMAN
    ATTACH_META_DTLSO(meta_log_dtlso, log_dtlso);
    ATTACH_META_DTLSAN(meta_log_dtlsan, log_dtlsan);
    ATTACH_META_DTLSAV(meta_log_dtlsav, log_dtlsav);
#endif

    if (!g_log_file)
        return 1;

    if ( !((g_log_level & LOG_DTLS_STATUS) || (g_log_level & LOG_DTLS_ARRAYS)) )
        return 2;

    log_dtlso.tol1  = tol1;
    log_dtlso.tol2  = tol2;
    log_dtlso.ldx   = ldx;
    log_dtlso.rank  = rank;
    log_dtlso.ierr  = ierr;
    log_dtlso.iwarn = iwarn;
    log_dtlso.ldc   = ldc;
    log_dtlso.m     = m;
    log_dtlso.n     = n;
    log_dtlso.l     = l;

    LOGREC(LOG_DTLSO, &log_dtlso, sizeof (log_dtlso), &meta_log_dtlso);

    if ( !(g_log_level & LOG_DTLS_ARRAYS) )
        return 0;

    log_dtlsan.name = 'C';
    log_dtlsan.nrow = ldc;
    log_dtlsan.ncol = n + l;

    LOGREC(LOG_DTLSAN, &log_dtlsan, sizeof (log_dtlsan), &meta_log_dtlsan);

    prow = c;
    for (log_dtlsav.row = 0; log_dtlsav.row < log_dtlsan.nrow; log_dtlsav.row++)
    {
      pval = prow;
      for (log_dtlsav.col = 0; log_dtlsav.col < log_dtlsan.ncol; log_dtlsav.col++)
      {
        log_dtlsav.val = *pval;
        pval += ldc;
        LOGREC(LOG_DTLSAV, &log_dtlsav, sizeof (log_dtlsav), &meta_log_dtlsav);
      }
      prow++;
    }

    log_dtlsan.name = 'S';
    log_dtlsan.nrow = n + l;
    log_dtlsan.ncol = 1;

    LOGREC(LOG_DTLSAN, &log_dtlsan, sizeof (log_dtlsan), &meta_log_dtlsan);

    prow = s;
    log_dtlsav.col = 0;
    for (log_dtlsav.row = 0; log_dtlsav.row < log_dtlsan.nrow; log_dtlsav.row++)
    {
      log_dtlsav.val = *prow;
      LOGREC(LOG_DTLSAV, &log_dtlsav, sizeof (log_dtlsav), &meta_log_dtlsav);

      prow++;
    }

    log_dtlsan.name = 'X';
    log_dtlsan.nrow = ldx;
    log_dtlsan.ncol = l;

    LOGREC(LOG_DTLSAN, &log_dtlsan, sizeof (log_dtlsan), &meta_log_dtlsan);

    prow = x;
    for (log_dtlsav.row = 0; log_dtlsav.row < log_dtlsan.nrow; log_dtlsav.row++)
    {
      pval = prow;
      for (log_dtlsav.col = 0; log_dtlsav.col < log_dtlsan.ncol; log_dtlsav.col++)
      {
        log_dtlsav.val = *pval;
        pval += ldx;
        LOGREC(LOG_DTLSAV, &log_dtlsav, sizeof (log_dtlsav), &meta_log_dtlsav);
      }
      prow++;
    }

    return 0;
}

