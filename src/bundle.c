/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <string.h>

#include "bundle.h"
#include "bundle_log.h"
#include "log.h"

typedef struct
{
    long    idx;
    double  *bundle_vec;
} Tbundle_sort;

static Tbundle_sort *l_bundle_sorted = NULL;

static long l_n_row = 0;
static long l_n_bundle_vec = 0;
static int  l_n_bundle_val = 0;

static long *l_bundle_idx = NULL;

static Tbundle_set *l_bundle_set;

void
free_bundle (void);

int
free_bundle_filter (void);

int
log_bundle_set (Tbundle_set *bundle_set);

static int
compare (const void *a, const void *b)
{
    int i;

    Tbundle_sort *bs_a = (Tbundle_sort *) a;
    Tbundle_sort *bs_b = (Tbundle_sort *) b;

    for (i = 0; i < l_n_bundle_val; i++)
        if (bs_a->bundle_vec[i] < bs_b->bundle_vec[i])
            return -1;
        else if (bs_a->bundle_vec[i] > bs_b->bundle_vec[i])
            return 1;
        else continue;

    return bs_a->idx - bs_b->idx;
}

static double *l_filter_val = NULL;
static int    l_n_filter_val = 0;

int
init_bundle_filter (char *s_filter_val)
{
    char *s, *p, *ep;
    double d;

    free_bundle_filter ();

    if (!s_filter_val)
        return 0;

    for (s = s_filter_val; (p = strsep (&s, ",")) != NULL; )
    {
        d = strtod (p, &ep);
        if (d == 0 && ep == p)
        {
            fprintf (stderr, "Error in filter value conversion of <%s>.\n", p);
            free_bundle_filter ();
            return -1;
        }
        l_filter_val = (double *) realloc (l_filter_val, ++l_n_filter_val * sizeof (double));
        l_filter_val[l_n_filter_val - 1] = d;
    }

    return 0;
}

int
free_bundle_filter ()
{
    if (l_filter_val) free (l_filter_val);
    l_filter_val = NULL;
    l_n_filter_val = 0;

    return 0;
}

static bool
skip_filter (double *p_bundle)
{
    /* Currently only a filter on the first scalar value of p_bundle is available.*/
    if (l_n_filter_val == 0)
        return false;  /*do not skip*/

    int i;
    for (i = 0; i < l_n_filter_val; i++)
    {
        if (*p_bundle == l_filter_val[i])
            return (false);
    }

    return true;
}

static int l_offset_sorted;

int
new_bundles (Tembed *emb, Tbundle_set **bundle_set)
{
    long    i;
    double *p_bundle;

    if (!emb)
        return -1;

    if (emb->n_row < 1)
        return -2;

    if (!emb->bundle_mat)
    {
        free_bundle ();
        *bundle_set  = NULL; /* means: Select all */
        return 0;
    }

    l_n_row        = emb->n_row;
    l_n_bundle_val = emb->n_bundle_val;

    l_bundle_sorted = (Tbundle_sort *) malloc (emb->n_row * sizeof (Tbundle_sort));
    l_bundle_set = (Tbundle_set *) malloc (sizeof(Tbundle_set));

    p_bundle = emb->bundle_mat;
    l_n_bundle_vec = 0;
    for (i = 0; i < emb->n_row; i++)
    {
        if (!skip_filter (p_bundle))
        {
            l_bundle_sorted[l_n_bundle_vec].idx        = i;
            l_bundle_sorted[l_n_bundle_vec].bundle_vec = p_bundle;
            l_n_bundle_vec++;
        }

        p_bundle += emb->n_bundle_val;
    }

    if (l_n_bundle_vec == 0)
    {
        free_bundle ();
        return 1;
    }

    qsort (l_bundle_sorted, l_n_bundle_vec, sizeof(Tbundle_sort), &compare);

    l_bundle_idx = (long *) malloc (l_n_bundle_vec * sizeof (long));

    for (i = 0; i < l_n_bundle_vec; i++)
        l_bundle_idx[i] = l_bundle_sorted[i].idx;


    l_bundle_set->idx        = l_bundle_idx;
    l_bundle_set->n_idx      = 0;
    l_bundle_set->bundle_num = 0;
    l_offset_sorted          = 0;

    l_bundle_set->n_bundle_val = l_n_bundle_val;
    l_bundle_set->bundle_vec   = (double *) malloc (l_n_bundle_val * sizeof (double));

    *bundle_set = l_bundle_set;

    return next_bundle();
}

static int
compare_vec (double *a, double *b)
{
    int i;
    for (i = 0; i < l_n_bundle_val; i++)
        if (a[i] != b[i])
            return -1;

    return 0;
}

int
next_bundle (void)
{
    double *bundle_vec;
    Tbundle_sort *curr;
    long n_curr_bundle;
    int  i;

    if (!l_bundle_set)
        return 1;
    else if ((l_bundle_set->idx - l_bundle_idx) + l_bundle_set->n_idx == l_n_bundle_vec) /*last set has been processed*/
    {
        free_bundle ();
        return 1;
    }

    l_bundle_set->idx += l_bundle_set->n_idx;
    l_offset_sorted   += l_bundle_set->n_idx;

    curr = l_bundle_sorted + l_offset_sorted;
    bundle_vec = curr->bundle_vec;

    n_curr_bundle = 0;

    do
    {
        if (compare_vec (bundle_vec, curr->bundle_vec) != 0)
            break;

        n_curr_bundle++;
    } while (++curr < l_bundle_sorted + l_n_bundle_vec);

    for (i = 0; i < l_n_bundle_val; i++)
        l_bundle_set->bundle_vec[i] = bundle_vec[i]; /*copy scalar values of vector*/

    l_bundle_set->n_idx = n_curr_bundle;
    l_bundle_set->bundle_num++;

    log_bundle_set (l_bundle_set);

    return 0;
}

void
free_bundle (void)
{
    if (l_bundle_sorted) free (l_bundle_sorted);
    l_bundle_sorted = NULL;

    if (l_bundle_idx) free (l_bundle_idx);
    l_bundle_idx = NULL;

    if (l_bundle_set) 
    {
        free (l_bundle_set->bundle_vec);
        free (l_bundle_set);
    }
    l_bundle_set = NULL;

    l_n_bundle_val         = 0;
    l_n_row                = 0;
    l_n_bundle_vec         = 0;
}

int
log_bundle_set (Tbundle_set *bundle_set)
{
    int i;
    static struct s_log_bp log_bp;
    static struct s_log_bv log_bv;

#ifdef LOG_HUMAN
    ATTACH_META_BP(meta_log_bp, log_bp);
    ATTACH_META_BV(meta_log_bv, log_bv);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_BUNDLE_PAR))
        return 2;

    log_bp.bundle_num = bundle_set->bundle_num;

    LOGREC(LOG_BP, &log_bp, sizeof (log_bp), &meta_log_bp);

    for (i = 0; i < l_n_bundle_val; i++)
    {
        log_bv.vec_idx = i;
        log_bv.vec_val = bundle_set->bundle_vec[i];
        LOGREC(LOG_BV, &log_bv, sizeof (log_bv), &meta_log_bv);
    }

    return 0;
}
