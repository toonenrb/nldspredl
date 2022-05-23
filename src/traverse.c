/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Take time series data as input and traverse ranges of embeddings, lib_sizes and predictor parameters to create predictions
 * and or validate parameters.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "tsdat.h"
#include "tstoembdef.h"
#include "embed.h"
#include "mkembed.h"
#include "sets.h"
#include "point.h"
#include "fn_exp.h"
#include "stat.h"
#include "bundle.h"
#include "traverse.h"
#include "log.h"
#include "traverse_log.h"
#include "traverse_stat.h"

#define STAT_ALLOC_SIZE 500

/* Define NLPRESTATOUT in gcc call when no direct statistics output is required. */

static int
get_stats (Tpoint_set *points_observed, double *predicted, bool per_additional_val,
           void *fn_params, int emb_num, int set_num, Tbundle_set *bundle_set,
           Tembed *emb,
           Temb_lag_def *emb_lag_def, Tpoint_set *lib_set, Tpoint_set *pre_set,
           Tnlpre_stat **_nlpre_stat, long *_n_nlpre_stat
           );

static int
log_addit_hd (int n_addit_val);

static int
log_addit_dt (int n_addit_val, double *addit_val);

int
traverse_all (Tfdat *fdat, int n_emb_lag_def, Temb_lag_def emb_lag_def[],
              Tnew_sets new_sets, Tnext_set next_set, Tfree_set free_set,
              Tnew_fn_params new_fn_params, Tnext_fn_params next_fn_params, Tfn fn,
              bool validation, bool per_additional_val
#ifdef NLPRESTATOUT
              , Tnlpre_stat **_nlpre_stat, int *_n_nlpre_stat
#endif
                 )
{
    Tpoint_set  *lib_set, *pre_set;
    Tbundle_set *bundle_set;
    double      *predicted;
    Tembed      *emb;
    int         i_emb, set_num, nb;
    void        *fn_params;
    Tnlpre_stat *nlpre_stat = NULL;
    long        n_nlpre_stat = 0;

    for (i_emb = 0; i_emb < n_emb_lag_def; i_emb++)
    {
        emb = create_embed (fdat, emb_lag_def + i_emb, i_emb);
        if (!emb)
        {
            fprintf(stdout, "Skipping embedding <%d>.\n", i_emb);
            continue;
        }

        if (emb->n_row == 0)
        {
            free_embed (emb);
            fprintf(stdout, "Skipping embedding <%d>, zero rows.\n", i_emb);
            continue;
        }

        if ((nb = new_bundles (emb, &bundle_set)) < 0)
        {
            free_traverse ();
            free_embed (emb);
            return -5;
        }
        else if (nb > 0)
        {
            free_embed (emb);
            fprintf(stdout, "Skipping embedding <%d>, no bundle to process.\n", i_emb);
            continue;
        }

        do
        {

            if ( (*new_fn_params) (emb->e, &fn_params) != 0)
            {
                fprintf (stdout, "Warning: unable to get next set of fn parameters\n");
                continue;
            }

            do
            {
                if ( (*new_sets) (emb, bundle_set, &lib_set, &pre_set) != 0)
                {
                    fprintf (stdout, "Warning: unable to get new sets.\n");
                    continue;
                }

                set_num = 1;

                do
                {
                    if ( (*fn) (lib_set, pre_set, &predicted) < 0)
                    {
                        fprintf (stdout, "Warning: fn returned error.\n");
                        continue;
                    }

                    if (validation)
                    {
                        if (get_stats (pre_set, predicted, per_additional_val, fn_params, i_emb, set_num,
                                       bundle_set, emb, emb_lag_def + i_emb, lib_set, pre_set,
                                       &nlpre_stat, &n_nlpre_stat) < 0)
                        {
                            fprintf (stdout, "Warning: error when computing statistics.\n");
                            continue;
                        }
                    }

                    if (g_log_file)
                        fflush (g_log_file);

                    set_num++;
                } while ( (*next_set) () == 0); /*changes lib_set and pre_set contents*/

                (*free_set) ();

            } while ( (*next_fn_params) (&fn_params) == 0);

        } while (next_bundle () == 0);

        free_bundle ();

        free_embed (emb);
    }

#ifdef NLPRESTATOUT
    if (validation)
    {
        *_n_nlpre_stat = n_nlpre_stat;
        *_nlpre_stat   = nlpre_stat;
    }
    else
    {
        *_n_nlpre_stat = 0;
        *_nlpre_stat   = NULL;
    }
#endif

    return 0;
}

Tnlpre_stat *l_nlpre_stat = NULL;
long        l_n_nlpre_stat = 0;
long        l_n_nlpre_stat_alloc = 0;

static Tnlpre_stat *
create_nlpre_stat (Tstat *stat,
                   void *fn_params, int emb_num, int set_num, Tbundle_set *bundle_set,
                   Tembed *emb, double *addit_val,
                   Temb_lag_def *emb_lag_def, Tpoint_set *lib_set, Tpoint_set *pre_set)
{
    Tnlpre_stat *p_nlpre_stat;

    if (stat)
    {
        if (l_n_nlpre_stat_alloc == l_n_nlpre_stat)
        {
            l_nlpre_stat = 
               (Tnlpre_stat *) realloc (l_nlpre_stat, (l_n_nlpre_stat_alloc + STAT_ALLOC_SIZE) * sizeof (Tnlpre_stat));
            l_n_nlpre_stat_alloc += STAT_ALLOC_SIZE;
        }

        p_nlpre_stat = l_nlpre_stat + l_n_nlpre_stat;

        p_nlpre_stat->stat          = stat;
        p_nlpre_stat->emb_lag_def   = emb_lag_def;
        p_nlpre_stat->fn_params = fn_params;
        p_nlpre_stat->emb_num       = emb_num;
        p_nlpre_stat->bundle_num    = bundle_set? bundle_set->bundle_num: 0;
        p_nlpre_stat->set_num       = set_num;
        p_nlpre_stat->n_addit_val   = 0;
        p_nlpre_stat->addit_val     = NULL;
        p_nlpre_stat->n_bundle_val   = 0;
        p_nlpre_stat->bundle_vec    = NULL;

        p_nlpre_stat->lib_set_n_point   = lib_set->n_point;
        p_nlpre_stat->lib_set_e         = lib_set->e;
        p_nlpre_stat->lib_set_n_pre_val = lib_set->n_pre_val;

        p_nlpre_stat->pre_set_n_point   = pre_set->n_point;
        p_nlpre_stat->pre_set_e         = pre_set->e;
        p_nlpre_stat->pre_set_n_pre_val = pre_set->n_pre_val;

        p_nlpre_stat->emb_label = (char *) malloc ((strlen(emb->emb_label) + 1) * sizeof (char));
        strcpy (p_nlpre_stat->emb_label, emb->emb_label);

        if (addit_val)
        {
            p_nlpre_stat->n_addit_val = emb->n_addit_val;
            p_nlpre_stat->addit_val   = (double *) malloc (emb->n_addit_val * sizeof (double));
            memcpy (p_nlpre_stat->addit_val, addit_val, emb->n_addit_val * sizeof (double));
        }

        if (bundle_set)
        {
            p_nlpre_stat->n_bundle_val = bundle_set->n_bundle_val;
            p_nlpre_stat->bundle_vec   = (double *) malloc (bundle_set->n_bundle_val * sizeof (double));
            memcpy (p_nlpre_stat->bundle_vec, bundle_set->bundle_vec, bundle_set->n_bundle_val * sizeof (double));
        }

        l_n_nlpre_stat++;

        return p_nlpre_stat;
    }

    return NULL;
}

typedef struct
{
    Tpoint *point;
    double *predicted;
    double *addit_val;
} Tsort_struct;

static int l_n_addit_val;

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

static long l_n_points_sorted = 0;
static Tpoint **l_points_sorted = NULL;
static double *l_predicted_sorted = NULL;
static Tsort_struct *l_sort_structs = NULL;

static int
get_stats (Tpoint_set *points_observed, double *predicted, bool per_additional_val,
           void *fn_params, int emb_num, int set_num, Tbundle_set *bundle_set,
           Tembed *emb,
           Temb_lag_def *emb_lag_def, Tpoint_set *lib_set, Tpoint_set *pre_set,
           Tnlpre_stat **_nlpre_stat, long *_n_nlpre_stat)
{
    Tstat       *stat;

    stat = prediction_stat (points_observed, predicted);
    create_nlpre_stat (stat, fn_params, emb_num, set_num, bundle_set, emb, NULL, emb_lag_def, lib_set, pre_set);

    if (per_additional_val && emb->n_addit_val > 0)
    {
        /* Create statistics for each combination of values in additional variables */
        /* Note: grouped in order of first additional variable to last              */
        Tpoint      **save_points;
        long        i, j, save_n_point, start_point, next_start;
        double      *p_predicted;

        log_addit_hd (emb->n_addit_val);

        l_n_addit_val = emb->n_addit_val;
        if (points_observed->n_point > l_n_points_sorted)
        {
            l_points_sorted = (Tpoint **) realloc (l_points_sorted, points_observed->n_point * sizeof (Tpoint *));
            l_predicted_sorted = (double *) realloc (l_predicted_sorted, 
                                                points_observed->n_point * points_observed->n_pre_val * sizeof (double));
            l_sort_structs = (Tsort_struct *) realloc (l_sort_structs, points_observed->n_point * sizeof (Tsort_struct));

            l_n_points_sorted = points_observed->n_point;
        }

        for (i = 0; i < points_observed->n_point; i++)
        {
            l_sort_structs[i].point = points_observed->point[i];
            l_sort_structs[i].predicted = predicted + i * points_observed->n_pre_val;
            l_sort_structs[i].addit_val = points_observed->point[i]->addit_val;
        }

        /* Sort according to additional values */
        qsort (l_sort_structs, points_observed->n_point, sizeof (Tsort_struct),
                (int (*) (const void *, const void *)) compare_addit_val);

        /* Create temporary structures for use in prediction_stat function.*/
        p_predicted = l_predicted_sorted;
        for (i = 0; i < points_observed->n_point; i++)
        {
            l_points_sorted[i] = l_sort_structs[i].point;
            for (j = 0; j < points_observed->n_pre_val; j++)
                *p_predicted++ = l_sort_structs[i].predicted[j];
        }

        save_points  = points_observed->point;
        save_n_point = points_observed->n_point;

        /* Compute statistics per group with same additional values */
        start_point = 0;
        while (start_point < save_n_point)
        {
            next_start = start_point + 1;
            while (next_start < save_n_point)
            {
                if (compare_addit_val (l_sort_structs + start_point, l_sort_structs + next_start) != 0)
                    break;
                next_start++;
            }

            points_observed->point = l_points_sorted + start_point;
            points_observed->n_point = next_start - start_point;

            log_addit_dt (emb->n_addit_val, l_sort_structs[start_point].addit_val);
            stat = prediction_stat (points_observed, l_predicted_sorted + start_point * points_observed->n_pre_val);
            create_nlpre_stat (stat, fn_params, emb_num, set_num, bundle_set,
                               emb, l_sort_structs[start_point].point->addit_val, emb_lag_def, lib_set, pre_set);
            start_point = next_start;
        }

        points_observed->point   = save_points;
        points_observed->n_point = save_n_point;
    }

    *_nlpre_stat   = l_nlpre_stat;
    *_n_nlpre_stat = l_n_nlpre_stat;

    return 0;
}

int
free_traverse ()
{
    int i;
    void *p_fn_params = NULL;
    Tnlpre_stat *p_nlpre_stat;

    if (!l_nlpre_stat)
        return 1;

    for (i = 0; i < l_n_nlpre_stat; i++)
    {
        /* No deeper memory allocations in fn_params, otherwise we need a dedicated */
        /* free function for each type of fn params                                 */
        p_nlpre_stat = l_nlpre_stat + i;
        if (p_nlpre_stat->fn_params && p_nlpre_stat->fn_params != p_fn_params)
        {
            p_fn_params = p_nlpre_stat->fn_params;
            free (p_nlpre_stat->fn_params); 
        }

        if (p_nlpre_stat->stat)
            free_stat (p_nlpre_stat->stat); 

        if (p_nlpre_stat->bundle_vec)
            free (p_nlpre_stat->bundle_vec); 

        if (p_nlpre_stat->emb_label)
            free (p_nlpre_stat->emb_label); 

        if (p_nlpre_stat->addit_val)
            free (p_nlpre_stat->addit_val); 
    }

    free (l_nlpre_stat); 

    if (l_points_sorted)
        free (l_points_sorted);
    if (l_predicted_sorted)
        free (l_predicted_sorted);
    if (l_sort_structs)
        free (l_sort_structs);
    l_n_points_sorted = 0;

    return 0;
}

static int
log_addit_hd (int n_addit_val)
{
    static struct s_log_adhd log_adhd;
#ifdef LOG_HUMAN
    ATTACH_META_ADHD(meta_log_adhd, log_adhd);
#endif
    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_STATISTICS))
        return 2;

    log_adhd.n_addit_val = n_addit_val;
    LOGREC(LOG_ADHD, &log_adhd, sizeof (log_adhd), &meta_log_adhd);

    return 0;
}

static int
log_addit_dt (int n_addit_val, double *addit_val)
{
    int i;
    static struct s_log_addt log_addt;
#ifdef LOG_HUMAN
    ATTACH_META_ADDT(meta_log_addt, log_addt);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_STATISTICS))
        return 2;

    for (i = 0; i < n_addit_val; i++)
    {
        log_addt.idx = i;
        log_addt.addit_val = addit_val[i];
        LOGREC(LOG_ADDT, &log_addt, sizeof (log_addt), &meta_log_addt);
    }

    return 0;
}
