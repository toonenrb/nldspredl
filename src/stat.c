/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <float.h>
#include <math.h>

#include "point.h"
#include "log.h"
#include "stat.h"
#include "stat_log.h"

static double *observed = NULL;
static long   size_observed = 0;

static double *l_data1 = NULL;
static double *l_data2 = NULL;
static double *l_data3 = NULL;
static long   l_n_data = 0;

static int
log_stat (Tstat *stat);

static int
cmpdoublep (const double *p1, const double *p2)
{
    if (*p1 < *p2)
        return -1;
    else if (*p1 > *p2)
        return 1;

    return 0;
}

static double
get_median (double *data, int n)
{
    /* Rearranges data in *data */
    if (n == 0)
        return NAN;
    qsort ((void *) data, n, sizeof (double), (int (*) (const void *, const void *)) cmpdoublep);
    if (n % 2)
        return data[n / 2];
    else
        return (data[n / 2] + data[n / 2 - 1]) / 2.0;
}

static double
get_mdae (double *data1, double *data2, double *data3, int n)
{
    int i;
    for (i = 0; i <n ; i++)
        data3[i] = fabs (data1[i] - data2[i]);

    return get_median (data3, n);
}

static double
get_mdad (double *data, double md, int n)
{
    /*data should already be sorted low to high, and md is median*/
    double curr_e, curr_b, high, last_high;
    int i, b, e, half;

    if (n == 0)
        return NAN;
    b = 0;
    e = n - 1;
    half = n / 2;

    curr_e = data[e] - md;
    curr_b = md - data[b];
    for (i = 0; i < half; i++)
    {
        if (curr_e > curr_b)
        {
            last_high = curr_e;
            curr_e = data[--e] - md;
        }
        else
        {
            last_high = curr_b;
            curr_b = md - data[++b];
        }
    }
    high = curr_e > curr_b? curr_e: curr_b;

    if (n % 2)
        return high;
    else
        return (high + last_high) / 2;
}

/* compute_stats: only computes statistics for pairs without any missing values.*/
void
compute_stats (double *data1, double *data2, int stride, long size,
               long *_n,
               double *_mean1, double *_mean2,
               double *_var1, double *_var2,
               double *_cov, double *_mae, double *_rmse,
               double *_md1, double *_md2,
               double *_mdad1, double *_mdad2,
               double *_mdae)
{
    long i, n = 0;
    double mean1 = 0.0, mean2 = 0.0, var1 = 0.0, var2 = 0.0, cov = 0.0, mae = 0.0, mse = 0.0;
    double md1 = 0.0, md2 = 0.0;

    if (size > l_n_data)
    {
        l_data1 = realloc (l_data1, size * sizeof (double));
        l_data2 = realloc (l_data2, size * sizeof (double));
        l_data3 = realloc (l_data3, size * sizeof (double));
        l_n_data = size;
    }

    /*Copy into arrays for manipulation and sorting*/
    for (i = 0; i < size; i++)
    {
        if (isnan (data1[i * stride]) || isnan (data2[i * stride]))
            continue;
        l_data1[n] = data1[i * stride];
        l_data2[n] = data2[i * stride];
        n++;
    }

    for (i = 0; i < n; i++)
    {
        mean1 += (l_data1[i] - mean1) / (i + 1);
        mean2 += (l_data2[i] - mean2) / (i + 1);
    }

    *_n = n;
    *_mean1 = mean1;
    *_mean2 = mean2;

    for (i = 0; i < n; i++)
    {
        const long double delta_var1 = (l_data1[i] - mean1);
        const long double delta_var2 = (l_data2[i] - mean2);
        var1 += (delta_var1 * delta_var1 - var1) / (i + 1);
        var2 += (delta_var2 * delta_var2 - var2) / (i + 1);
        cov  += (delta_var1 * delta_var2 - cov) / (i + 1);

        const double delta = l_data1[i] - l_data2[i];
        mse += (delta * delta - mse) / (i + 1);
        mae += (fabs (delta) - mae) / (i + 1);
    }

    *_var1 = var1;
    *_var2 = var2;
    *_cov  = cov;
    *_mae  = mae;
    *_rmse = sqrt (mse);

    /* Robust statistics */
    /* Median absolute error */
    *_mdae = get_mdae (l_data1, l_data2, l_data3,  n);

    /* Median. Note: does not preserve order of values in l_data1 or l_data2 */
    md1 = get_median (l_data1, n);
    md2 = get_median (l_data2, n);
    *_md1 = md1;
    *_md2 = md2;
    /*l_data1 and l_data2 are now sorted*/
    /* Median absolute deviation */
    *_mdad1 = get_mdad (l_data1, md1, n);
    *_mdad2 = get_mdad (l_data2, md2, n);
}

Tstat *
prediction_stat (Tpoint_set *points_observed, double *predicted)
{
    double *obs_point_val, *observed_val;
    int    pre_count;
    long   point_count;
    Tstat  *stat;

    if (observed && size_observed != points_observed->n_point * points_observed->n_pre_val)
    {
        free (observed);
        observed = NULL;
        size_observed = 0;
    }

    if (!observed)
    {
        size_observed = points_observed->n_point * points_observed->n_pre_val;
        observed = (double *) malloc (size_observed * sizeof (double));
    }

    stat = (Tstat *) malloc (sizeof(Tstat));
    stat->n_pre_val = points_observed->n_pre_val;

    stat->n_pre_obs = (long *) malloc (stat->n_pre_val * sizeof (long));

    stat->avg_pre = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->avg_obs = (double *) malloc (stat->n_pre_val * sizeof (double));

    stat->var_pre = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->var_obs = (double *) malloc (stat->n_pre_val * sizeof (double));

    stat->cov_pre_obs  = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->mae_pre_obs  = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->rmse_pre_obs = (double *) malloc (stat->n_pre_val * sizeof (double));

    stat->md_obs = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->md_pre = (double *) malloc (stat->n_pre_val * sizeof (double));

    stat->mdad_obs = (double *) malloc (stat->n_pre_val * sizeof (double));
    stat->mdad_pre = (double *) malloc (stat->n_pre_val * sizeof (double));

    stat->mdae_pre_obs = (double *) malloc (stat->n_pre_val * sizeof (double));

    observed_val = observed;
    /* Put values of points into a continuous block of memory.*/
    for (point_count = 0; point_count < points_observed->n_point; point_count++)
    {
        obs_point_val = points_observed->point[point_count]->pre_val;
        for (pre_count = 0; pre_count < points_observed->n_pre_val; pre_count++)
            *observed_val++ = *obs_point_val++;
    }

    for (pre_count = 0; pre_count < points_observed->n_pre_val; pre_count++)
        compute_stats (observed + pre_count, predicted + pre_count,
                       points_observed->n_pre_val, points_observed->n_point,
                       &stat->n_pre_obs[pre_count],
                       &stat->avg_obs[pre_count], &stat->avg_pre[pre_count],
                       &stat->var_obs[pre_count], &stat->var_pre[pre_count],
                       &stat->cov_pre_obs[pre_count], &stat->mae_pre_obs[pre_count], &stat->rmse_pre_obs[pre_count],
                       &stat->md_obs[pre_count], &stat->md_pre[pre_count],
                       &stat->mdad_obs[pre_count], &stat->mdad_pre[pre_count],
                       &stat->mdae_pre_obs[pre_count]);

    log_stat (stat);

    return stat;
}

int
free_stat (Tstat *stat)
{
    if (observed)
    {
        free (observed);
        observed = NULL;
        size_observed = 0;
    }

    if (l_n_data > 0)
    {
        free (l_data1);
        l_data1 = NULL;
        free (l_data2);
        l_data2 = NULL;
        free (l_data3);
        l_data3 = NULL;
        l_n_data = 0;
    }

    if (!stat)
        return 0;

    if (stat->n_pre_obs) free (stat->n_pre_obs);

    if (stat->avg_pre) free (stat->avg_pre);
    if (stat->avg_obs) free (stat->avg_obs);

    if (stat->var_pre) free (stat->var_pre);
    if (stat->var_obs) free (stat->var_obs);

    if (stat->cov_pre_obs ) free (stat->cov_pre_obs );
    if (stat->mae_pre_obs ) free (stat->mae_pre_obs );
    if (stat->rmse_pre_obs) free (stat->rmse_pre_obs);

    if (stat->md_obs) free (stat->md_obs);
    if (stat->md_pre) free (stat->md_pre);

    if (stat->mdad_obs) free (stat->mdad_obs);
    if (stat->mdad_pre) free (stat->mdad_pre);

    if (stat->mdae_pre_obs) free (stat->mdae_pre_obs);

    free (stat);

    return 0;
}

static int
log_stat (Tstat *stat)
{
    int i;
    static struct s_log_stnp log_stnp;
    static struct s_log_stat log_stat;
#ifdef LOG_HUMAN
    ATTACH_META_STNP(meta_log_stnp, log_stnp);
    ATTACH_META_STAT(meta_log_stat, log_stat);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_STATISTICS))
        return 2;

    log_stnp.n_pre_val = stat->n_pre_val;
    LOGREC(LOG_STNP, &log_stnp, sizeof (log_stnp), &meta_log_stnp);

    for (i = 0; i < stat->n_pre_val; i++)
    {
        log_stat.pre_val_num  = i;
        log_stat.n_pre_obs    = stat->n_pre_obs[i];
        log_stat.avg_pre      = stat->avg_pre[i];
        log_stat.avg_obs      = stat->avg_obs[i];
        log_stat.var_pre      = stat->var_pre[i];
        log_stat.var_obs      = stat->var_obs[i];
        log_stat.cov_pre_obs  = stat->cov_pre_obs[i];
        log_stat.mae_pre_obs  = stat->mae_pre_obs[i];
        log_stat.rmse_pre_obs = stat->rmse_pre_obs[i];
        log_stat.md_pre       = stat->md_pre[i];
        log_stat.md_obs       = stat->md_obs[i];
        log_stat.mdad_pre     = stat->mdad_pre[i];
        log_stat.mdad_obs     = stat->mdad_obs[i];
        log_stat.mdae_pre_obs = stat->mdae_pre_obs[i];
        LOGREC(LOG_STAT, &log_stat, sizeof (log_stat), &meta_log_stat);
    }

    return 0;
}
