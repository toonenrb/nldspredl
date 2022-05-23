/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "tsdat.h"
#include "tstoembdef.h"
#include "mkembed.h"
#include "log.h"
#include "mkembed_log.h"

static int
create_meta (Tembed *emb, Temb_lag_def *eld, Tfdat *fdat);

static int
validate_lag_def (Tfdat *fdat, Temb_lag_def *eld);

static int
log_emb_par (Tembed *emb);

static int
log_emb_vec (Tembed *emb);

Tembed *
create_embed (Tfdat *fdat, Temb_lag_def *eld, int emb_num)
{
    Tembed  *emb;
    int     i, j;
    long    vec_num = 0;
    int     max_lag, min_lag, range, n_max, zero_begin_offset, zero_end_offset, var_no;
    int     rows_malloc, rows_avail, ncol;
    double  **co_lag_mapper, **pre_lag_mapper, *curr_co_val, *curr_pre_val;
    double  **addit_lag_mapper = NULL, *curr_addit_val;
    double  **bundle_mapper = NULL, *curr_bundle_val;
    long    **t_mapper, *curr_t_val, **t_pre_mapper, *curr_t_pre_val, **t_addit_mapper = NULL, *curr_t_addit_val, *curr_vec_num;
    char    **id_mapper, *id_val, **curr_id, id_val_init[] = "1qDfx*()gV";
    bool    *use_in_lib_mapper, *use_in_pre_mapper, *curr_use_in_lib, *curr_use_in_pre;

    if (validate_lag_def(fdat, eld) < 0)
        return NULL;

    emb = (Tembed *) calloc (1, sizeof(Tembed));

    create_meta (emb, eld, fdat); /*e, n_pre_val, n_bundle_val, co_meta, pre_meta, bundle_meta*/
    
    /*find max lag and min lag. Note: positive lag is value in the past.*/
    max_lag = 0;
    for (i = 0; i < emb->e; i++)
        if (emb->co_meta[i].lag > max_lag) max_lag = emb->co_meta[i].lag;
    for (i = 0; i < emb->n_pre_val; i++)
        if (emb->pre_meta[i].lag > max_lag) max_lag = emb->pre_meta[i].lag;
    for (i = 0; i < emb->n_addit_val; i++)
        if (emb->addit_meta[i].lag > max_lag) max_lag = emb->addit_meta[i].lag;

    min_lag = 0;
    for (i = 0; i < emb->e; i++)
        if (emb->co_meta[i].lag < min_lag) min_lag = emb->co_meta[i].lag;
    for (i = 0; i < emb->n_pre_val; i++)
        if (emb->pre_meta[i].lag < min_lag) min_lag = emb->pre_meta[i].lag;
    for (i = 0; i < emb->n_addit_val; i++)
        if (emb->addit_meta[i].lag < min_lag) min_lag = emb->addit_meta[i].lag;

    range = max_lag - min_lag + 1;
    n_max = fdat->n_dat - range + 1;

    zero_begin_offset = max_lag;
    zero_end_offset   = zero_begin_offset + n_max - 1;

    /* Create arrays of pointers to travel through time series and create embedding.*/
    t_mapper = (long **) malloc(emb->e * sizeof(long *));
    for (i = 0; i < emb->e; i++)
        t_mapper[i] = fdat->t + zero_begin_offset - emb->co_meta[i].lag;

    t_pre_mapper = (long **) malloc(emb->n_pre_val * sizeof(long *));
    for (i = 0; i < emb->n_pre_val; i++)
        t_pre_mapper[i] = fdat->t + zero_begin_offset - emb->pre_meta[i].lag;

    co_lag_mapper = (double **) malloc (emb->e * sizeof(double *));
    for (i = 0; i < emb->e; i++)
    {
        for (var_no = 0; var_no < fdat->n_col; var_no++)
            if (strcmp (fdat->lab[var_no], emb->co_meta[i].var_name) == 0) break;

        co_lag_mapper[i] = fdat->dat + (zero_begin_offset - emb->co_meta[i].lag) * fdat->n_col + var_no;
    }

    pre_lag_mapper = (double **) malloc (emb->n_pre_val * sizeof(double *));
    for (i = 0; i < emb->n_pre_val; i++)
    {
        for (var_no = 0; var_no < fdat->n_col; var_no++)
            if (strcmp (fdat->lab[var_no], emb->pre_meta[i].var_name) == 0) break;

        pre_lag_mapper[i] = fdat->dat + (zero_begin_offset - emb->pre_meta[i].lag) * fdat->n_col + var_no;
    } 

    if (emb->n_addit_val)
    {
        addit_lag_mapper = (double **) malloc (emb->n_addit_val * sizeof(double *));
        t_addit_mapper = (long **) malloc (emb->n_addit_val * sizeof(long *));
    }

    for (i = 0; i < emb->n_addit_val; i++)
    {
        for (var_no = 0; var_no < fdat->n_col; var_no++)
            if (strcmp(fdat->lab[var_no], emb->addit_meta[i].var_name) == 0) break;

        addit_lag_mapper[i] = fdat->dat + (zero_begin_offset - emb->addit_meta[i].lag) * fdat->n_col + var_no;
    } 

    for (i = 0; i < emb->n_addit_val; i++)
        t_addit_mapper[i] = fdat->t + zero_begin_offset - emb->addit_meta[i].lag;

    id_mapper = (fdat->id ? fdat->id + zero_begin_offset : NULL);
    use_in_lib_mapper = (fdat->in_lib ? fdat->in_lib + zero_begin_offset : NULL);
    use_in_pre_mapper = (fdat->in_pre ? fdat->in_pre + zero_begin_offset : NULL);

    if (emb->n_bundle_val)
        bundle_mapper = (double **) malloc (emb->n_bundle_val * sizeof(double *));
    for (i = 0; i < emb->n_bundle_val; i++)
    {
        for (var_no = 0; var_no < fdat->n_bundle_col; var_no++)
            if (strcmp (fdat->bundle_lab[var_no], emb->bundle_meta[i].var_name) == 0) break;

        bundle_mapper[i] = fdat->bundle + zero_begin_offset * fdat->n_bundle_col + var_no;
    } 

    /* Create embedding */
    emb->co_val_mat  = NULL;
    emb->pre_val_mat = NULL;
    emb->bundle_mat  = NULL;
    emb->addit_mat   = NULL;
    emb->t_mat       = NULL;
    emb->t_pre_mat   = NULL;
    emb->t_addit_mat = NULL;
    emb->id          = NULL;
    emb->id_arr      = NULL;
    emb->vec_num     = NULL;
    emb->use_in_lib  = NULL;
    emb->use_in_pre  = NULL;
    emb->n_row       = 0;
    emb->emb_num     = emb_num;
    rows_malloc = 200;
    rows_avail  = 0;
    ncol        = fdat->n_col;
    id_val      = id_val_init;

    for (i = 0; i < n_max; i++)
    {
        /*check data*/
        for (j = 0; j < emb->e; j++)
            if (isnan (*co_lag_mapper[j])) goto next_shift;   /*next_shift: continues outer loop*/
        for (j = 0; j < emb->n_pre_val; j++)
            if (isnan (*pre_lag_mapper[j])) goto next_shift;
        /*note: values in addit_mat are allowed to be NaN*/

        /* All id's within range should be the same. */
        /* TS data should be ordered: id first, then time value. */
        if (id_mapper)
            if (strcmp (*(id_mapper - max_lag), *(id_mapper - min_lag)) != 0) 
                goto next_shift;

        if (emb->n_row == rows_avail) /*end of available space reached, increase space*/
        {
            emb->co_val_mat = (double *)
                realloc (emb->co_val_mat, (rows_avail + rows_malloc) * emb->e * sizeof(double));
            curr_co_val = emb->co_val_mat + emb->n_row * emb->e;

            emb->pre_val_mat = (double *)
                realloc (emb->pre_val_mat, (rows_avail + rows_malloc) * emb->n_pre_val * sizeof(double));
            curr_pre_val = emb->pre_val_mat + emb->n_row * emb->n_pre_val;

            if (addit_lag_mapper)
            {
                emb->addit_mat = (double *)
                    realloc (emb->addit_mat, (rows_avail + rows_malloc) * emb->n_addit_val * sizeof(double));
                curr_addit_val = emb->addit_mat + emb->n_row * emb->n_addit_val;

                emb->t_addit_mat = (long *)
                    realloc (emb->t_addit_mat, (rows_avail + rows_malloc) * emb->n_addit_val * sizeof(long));
                curr_t_addit_val = emb->t_addit_mat + emb->n_row * emb->n_addit_val;
            }

            emb->t_mat = (long *)
                realloc (emb->t_mat, (rows_avail + rows_malloc) * emb->e * sizeof(long));
            curr_t_val = emb->t_mat + emb->n_row * emb->e;

            emb->t_pre_mat = (long *)
                realloc (emb->t_pre_mat, (rows_avail + rows_malloc) * emb->n_pre_val * sizeof(long));
            curr_t_pre_val = emb->t_pre_mat + emb->n_row * emb->n_pre_val;

            emb->vec_num = (long *)
                realloc (emb->vec_num, (rows_avail + rows_malloc) * sizeof(long));
            curr_vec_num = emb->vec_num + emb->n_row;

            if (id_mapper)
            {
                emb->id_arr = (char **) realloc (emb->id_arr, (rows_avail + rows_malloc) * sizeof(char *));
                curr_id = emb->id_arr + emb->n_row;
            }

            if (use_in_lib_mapper)
            {
                emb->use_in_lib = (bool *)
                    realloc (emb->use_in_lib, (rows_avail + rows_malloc) * sizeof(bool));
                curr_use_in_lib = emb->use_in_lib + emb->n_row;
            }

            if (use_in_pre_mapper)
            {
                emb->use_in_pre = (bool *)
                    realloc (emb->use_in_pre, (rows_avail + rows_malloc) * sizeof(bool));
                curr_use_in_pre = emb->use_in_pre + emb->n_row;
            }

            if (bundle_mapper)
            {
                emb->bundle_mat = (double *)
                    realloc (emb->bundle_mat, (rows_avail + rows_malloc) * emb->n_bundle_val * sizeof(double));
                curr_bundle_val = emb->bundle_mat + emb->n_row * emb->n_bundle_val;
            }

            rows_avail += rows_malloc;
        }

        if (id_mapper)
        {
            if (strcmp(*id_mapper, id_val) != 0) /*new id*/
            {
                emb->id = (char **) realloc (emb->id, (emb->n_id + 1) * sizeof(char *));
                emb->id[emb->n_id] = (char *) malloc ((strlen(*id_mapper) + 1) * sizeof(char));
                strcpy (emb->id[emb->n_id], *id_mapper);
                id_val = emb->id[emb->n_id];
                emb->n_id++;
            }
        }


        for (j = 0; j < emb->e; j++)
            *curr_co_val++ = *co_lag_mapper[j];

        for (j = 0; j < emb->e; j++)
            *curr_t_val++ = *t_mapper[j];

        for (j = 0; j < emb->n_pre_val; j++)
            *curr_t_pre_val++ = *t_pre_mapper[j];

        for (j = 0; j < emb->n_pre_val; j++)
            *curr_pre_val++ = *pre_lag_mapper[j];

        for (j = 0; j < emb->n_addit_val; j++)
            *curr_t_addit_val++ = *t_addit_mapper[j];

        for (j = 0; j < emb->n_addit_val; j++)
            *curr_addit_val++ = *addit_lag_mapper[j];

        for (j = 0; j < emb->n_bundle_val; j++)
            *curr_bundle_val++ = *bundle_mapper[j];

        *curr_vec_num++ = vec_num++;

        if (id_mapper)
            *curr_id++ = id_val;

        if (use_in_lib_mapper)
            *curr_use_in_lib++ = *use_in_lib_mapper;

        if (use_in_pre_mapper)
            *curr_use_in_pre++ = *use_in_pre_mapper;

        emb->n_row++;

        next_shift:

        for (j = 0; j < emb->e; j++)
            co_lag_mapper[j] += ncol;

        for (j = 0; j < emb->e; j++)
            t_mapper[j]++;

        for (j = 0; j < emb->n_pre_val; j++)
            pre_lag_mapper[j] += ncol;

        for (j = 0; j < emb->n_pre_val; j++)
            t_pre_mapper[j]++;

        for (j = 0; j < emb->n_addit_val; j++)
            addit_lag_mapper[j] += ncol;

        for (j = 0; j < emb->n_addit_val; j++)
            t_addit_mapper[j]++;

        for (j = 0; j < emb->n_bundle_val; j++)
            bundle_mapper[j]++;

        if (id_mapper) id_mapper++;
        if (use_in_lib_mapper) use_in_lib_mapper++;
        if (use_in_pre_mapper) use_in_pre_mapper++;
    }

    if (co_lag_mapper) free (co_lag_mapper);
    if (pre_lag_mapper) free (pre_lag_mapper);
    if (addit_lag_mapper) free (addit_lag_mapper);
    if (bundle_mapper) free (bundle_mapper);
    if (t_mapper) free (t_mapper);
    if (t_pre_mapper) free (t_pre_mapper);
    if (t_addit_mapper) free (t_addit_mapper);

    log_emb_par (emb);
    log_emb_vec (emb);

    return emb;
}

char *
emb_label_add_lag (char *emb_label, char *var_name, int lag, bool first_of_group)
{
    static char *prev_var_name = NULL;
    char aint[15];

    if (!emb_label)
    {
        emb_label = (char *) malloc (sizeof(char));
        emb_label[0] = '\0';
    }

    if (first_of_group || !prev_var_name || strcmp (prev_var_name, var_name) != 0)
    {
        if (!first_of_group)
        {
            emb_label = (char *) realloc (emb_label, (strlen(emb_label) + 2) * sizeof(char));
            emb_label = strcat (emb_label, ";");
        }

        emb_label = (char *) realloc (emb_label, (strlen(emb_label) + strlen(var_name) + 1) * sizeof(char));
        emb_label = strcat (emb_label, var_name);
    }

    if (lag != INT_MAX)
    {
        snprintf (aint, 15, ",%d", lag);
        emb_label = (char *) realloc (emb_label, (strlen(emb_label) + strlen(aint) + 1) * sizeof(char));
        emb_label = strcat (emb_label, aint);
    }

    prev_var_name = var_name;

    return emb_label;
}

char *
emb_label_add_sep (char *emb_label, char *sep)
{
    if (!emb_label)
    {
        emb_label = (char *) malloc (sizeof(char));
        emb_label[0] = '\0';
    }

    emb_label = (char *) realloc (emb_label, (strlen(emb_label) + strlen(sep) + 1) * sizeof(char));
    emb_label = strcat (emb_label, sep);

    return emb_label;
}

int
create_meta (Tembed *emb, Temb_lag_def *eld, Tfdat *fdat)
{
    int  i;
    bool first_of_group;

    emb->e            = eld->n_co_lag;
    emb->emb_label    = NULL;
    emb->n_pre_val    = eld->n_pre_lag;
    emb->n_bundle_val = fdat->n_bundle_col;
    emb->n_addit_val  = eld->n_addit_lag;
    emb->co_meta     = (Tco_val_meta *) calloc (emb->e, sizeof(Tco_val_meta));
    emb->pre_meta    = (Tpre_val_meta *) calloc (emb->n_pre_val, sizeof(Tpre_val_meta));
    emb->addit_meta  = (Taddit_val_meta *) calloc (emb->n_addit_val, sizeof(Taddit_val_meta));
    emb->bundle_meta = emb->n_bundle_val > 0 ? (Tbundle_val_meta *) calloc (emb->n_bundle_val, sizeof(Tbundle_val_meta)) : NULL;
    emb->n_ranges    = eld->n_ranges;
    emb->range_dim   = NULL;
    emb->range_tau   = NULL;

    if (eld->n_ranges > 0)
    {
        emb->range_dim = (int *) malloc (eld->n_ranges * sizeof (int));
        emb->range_tau = (int *) malloc (eld->n_ranges * sizeof (int));
        memcpy (emb->range_dim, eld->range_dim, eld->n_ranges * sizeof (int));
        memcpy (emb->range_tau, eld->range_tau, eld->n_ranges * sizeof (int));
    }

    /*create meta data*/
    first_of_group = true;
    for (i = 0; i < eld->n_co_lag; i++)
    {
        emb->co_meta[i].var_name = malloc ((strlen(eld->co_lag[i].var_name) + 1) * sizeof(char));
        strcpy (emb->co_meta[i].var_name, eld->co_lag[i].var_name);
        emb->co_meta[i].lag = eld->co_lag[i].lag;
        emb->emb_label = emb_label_add_lag (emb->emb_label, eld->co_lag[i].var_name, eld->co_lag[i].lag, first_of_group);
        first_of_group = false;
    }
    emb->emb_label = emb_label_add_sep (emb->emb_label, ":");

    first_of_group = true;
    for (i = 0; i < eld->n_pre_lag; i++)
    {
        emb->pre_meta[i].var_name = malloc ((strlen(eld->pre_lag[i].var_name) + 1) * sizeof(char));
        strcpy (emb->pre_meta[i].var_name, eld->pre_lag[i].var_name);
        emb->pre_meta[i].lag = eld->pre_lag[i].lag;
        emb->emb_label = emb_label_add_lag (emb->emb_label, eld->pre_lag[i].var_name, eld->pre_lag[i].lag, first_of_group);
        first_of_group = false;
    }
    emb->emb_label = emb_label_add_sep (emb->emb_label, ":");

    first_of_group = true;
    for (i = 0; i < eld->n_addit_lag; i++)
    {
        emb->addit_meta[i].var_name = malloc ((strlen(eld->addit_lag[i].var_name) + 1) * sizeof(char));
        strcpy (emb->addit_meta[i].var_name, eld->addit_lag[i].var_name);
        emb->addit_meta[i].lag = eld->addit_lag[i].lag;
        emb->emb_label = emb_label_add_lag (emb->emb_label, eld->addit_lag[i].var_name, eld->addit_lag[i].lag, first_of_group);
        first_of_group = false;
    }
    emb->emb_label = emb_label_add_sep (emb->emb_label, ":");

    first_of_group = true;
    for (i = 0; i < emb->n_bundle_val; i++)
    {
        emb->bundle_meta[i].var_name = malloc ((strlen(fdat->bundle_lab[i]) + 1) * sizeof(char));
        strcpy (emb->bundle_meta[i].var_name, fdat->bundle_lab[i]);
        emb->emb_label = emb_label_add_lag (emb->emb_label, fdat->bundle_lab[i], INT_MAX, first_of_group);
        first_of_group = false;
    }

    return 0;
}

int
validate_lag_def (Tfdat *fdat, Temb_lag_def *eld)
{
    int    i, j;

    /*check existence of columns in time series data*/
    for (i = 0; i < eld->n_co_lag; i++)
    {
        for (j = 0; j < fdat->n_col; j++)
        {
            if (strcmp(eld->co_lag[i].var_name, fdat->lab[j]) == 0)
                goto next_co_var;
        }

        printf ("Unknown variable name in coordinate lag definitions <%s>\n", eld->co_lag[i].var_name);
        return -1;

        next_co_var: ;
    }

    for (i = 0; i < eld->n_pre_lag; i++)
    {
        for (j = 0; j < fdat->n_col; j++)
        {
            if (strcmp(eld->pre_lag[i].var_name, fdat->lab[j]) == 0)
                goto next_pre_var;
        }

        printf ("Unknown variable name in predictee lag definitions <%s>\n", eld->pre_lag[i].var_name);
        return -1;

        next_pre_var: ;
    }

    for (i = 0; i < eld->n_addit_lag; i++)
    {
        for (j = 0; j < fdat->n_col; j++)
        {
            if (strcmp(eld->addit_lag[i].var_name, fdat->lab[j]) == 0)
                goto next_addit_var;
        }

        printf ("Unknown variable name in additional lag definitions <%s>\n", eld->addit_lag[i].var_name);
        return -1;

        next_addit_var: ;
    }

    return(0);
}

int
free_embed (Tembed *emb)
{
    int i;

    if (!emb)
        return 0;

    if (emb->emb_label) free (emb->emb_label);

    if (emb->id)
    {
        for (i = 0; i < emb->n_id; i++)
            if (emb->id[i])
                free (emb->id[i]);
        free (emb->id);
    }

    if (emb->id_arr) free (emb->id_arr);

    if (emb->t_mat) free (emb->t_mat);
    if (emb->t_pre_mat) free (emb->t_pre_mat);
    if (emb->t_addit_mat) free (emb->t_addit_mat);

    if (emb->co_val_mat) free (emb->co_val_mat);
    if (emb->pre_val_mat) free (emb->pre_val_mat);
    if (emb->addit_mat) free (emb->addit_mat);
    if (emb->bundle_mat) free (emb->bundle_mat);

    if (emb->vec_num) free (emb->vec_num);

    if (emb->co_meta)
    {
        for (i = 0; i < emb->e; i++)
            if (emb->co_meta[i].var_name) free (emb->co_meta[i].var_name);
        free (emb->co_meta);
    }

    if (emb->pre_meta)
    {
        for(i = 0; i < emb->n_pre_val; i++)
            if (emb->pre_meta[i].var_name) free (emb->pre_meta[i].var_name);
        free (emb->pre_meta);
    }

    if (emb->addit_meta)
    {
        for (i = 0; i < emb->n_addit_val; i++)
            if (emb->addit_meta[i].var_name) free (emb->addit_meta[i].var_name);
        free (emb->addit_meta);
    }

    if (emb->bundle_meta)
    {
        for (i = 0; i < emb->n_bundle_val; i++)
            if (emb->bundle_meta[i].var_name) free(emb->bundle_meta[i].var_name);
        free (emb->bundle_meta);
    }

    if (emb->use_in_lib) free (emb->use_in_lib);
    if (emb->use_in_pre) free (emb->use_in_pre);

    if (emb->range_dim) free (emb->range_dim);
    if (emb->range_tau) free (emb->range_tau);

    free (emb);

    return 0;
}

static int
log_emb_par (Tembed *emb)
{
    int i;
    static struct s_log_embpar1 log_embpar1;
    static struct s_log_embpar2 log_embpar2;
    static struct s_log_embpar3 log_embpar3;
#ifdef LOG_HUMAN
    ATTACH_META_EMBPAR1(meta_log_embpar1, log_embpar1);
    ATTACH_META_EMBPAR2(meta_log_embpar2, log_embpar2);
    ATTACH_META_EMBPAR3(meta_log_embpar3, log_embpar3);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_EMB_PAR))
        return 2;

    log_embpar1.emb_num      = emb->emb_num;
    strncpy (log_embpar1.emb_label, emb->emb_label, 133);
    log_embpar1.n_row        = emb->n_row;
    log_embpar1.e            = emb->e;
    log_embpar1.n_pre_val    = emb->n_pre_val;
    log_embpar1.n_bundle_val = emb->n_bundle_val;
    log_embpar1.n_ranges     = emb->n_ranges;
    LOGREC(LOG_EMBPAR1, &log_embpar1, sizeof (log_embpar1), &meta_log_embpar1);

    for (i = 0; i < emb->e; i++)
    {
        log_embpar2.copr      = 'C';
        log_embpar2.coord     = i;
        strncpy (log_embpar2.var_name, emb->co_meta[i].var_name, 21);
        log_embpar2.lag       = emb->co_meta[i].lag;
        LOGREC(LOG_EMBPAR2, &log_embpar2, sizeof (log_embpar2), &meta_log_embpar2);
    }

    for (i = 0; i < emb->n_pre_val; i++)
    {
        log_embpar2.copr      = 'P';
        log_embpar2.coord     = i;
        strncpy (log_embpar2.var_name, emb->pre_meta[i].var_name, 21);
        log_embpar2.lag       = emb->pre_meta[i].lag;
        LOGREC(LOG_EMBPAR2, &log_embpar2, sizeof (log_embpar2), &meta_log_embpar2);
    }

    for (i = 0; i < emb->n_addit_val; i++)
    {
        log_embpar2.copr      = 'A';
        log_embpar2.coord     = i;
        strncpy (log_embpar2.var_name, emb->addit_meta[i].var_name, 21);
        log_embpar2.lag       = emb->addit_meta[i].lag;
        LOGREC(LOG_EMBPAR2, &log_embpar2, sizeof (log_embpar2), &meta_log_embpar2);
    }

    for (i = 0; i < emb->n_bundle_val; i++)
    {
        log_embpar2.copr      = 'B';
        log_embpar2.coord     = i;
        strncpy (log_embpar2.var_name, emb->bundle_meta[i].var_name, 21);
        log_embpar2.lag       = 0;
        LOGREC(LOG_EMBPAR2, &log_embpar2, sizeof (log_embpar2), &meta_log_embpar2);
    }

    for (i = 0; i < emb->n_ranges; i++)
    {
        log_embpar3.seq   = i;
        log_embpar3.dim   = emb->range_dim[i];
        log_embpar3.tau   = emb->range_tau[i];
        LOGREC(LOG_EMBPAR3, &log_embpar3, sizeof (log_embpar3), &meta_log_embpar3);
    }

    return 0;
}

static int
log_emb_vec (Tembed *emb)
{
    int i, j;

    char    **id;
    long    *t, *t_pre, *t_addit, *vec_num;
    double  *co_val, *pre_val, *addit_val, *bundle_val;
    bool    *use_in_lib, *use_in_pre;
    static struct s_log_vid log_vid;
    static struct s_log_val log_val;
#ifdef LOG_HUMAN
    ATTACH_META_VID(meta_log_vid, log_vid);
    ATTACH_META_VAL(meta_log_val, log_val);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_EMB_VEC))
        return 2;

    id         = emb->id_arr;
    t          = emb->t_mat;
    t_pre      = emb->t_pre_mat;
    t_addit    = emb->t_addit_mat;
    co_val     = emb->co_val_mat;
    pre_val    = emb->pre_val_mat;
    addit_val  = emb->addit_mat;
    bundle_val = emb->bundle_mat;
    use_in_lib = emb->use_in_lib;
    use_in_pre = emb->use_in_pre;
    vec_num    = emb->vec_num;

    for (i = 0; i < emb->n_row; i++)
    {
        log_vid.vec_num    = *vec_num;
        strncpy (log_vid.id, id ? *id: "", 10);
        log_vid.use_in_lib = use_in_lib ? (short) *use_in_lib: -1;
        log_vid.use_in_pre = use_in_pre ? (short) *use_in_pre: -1;
        LOGREC(LOG_VID, &log_vid, sizeof (log_vid), &meta_log_vid);

        vec_num++;
        if (id) id++;
        if (use_in_lib) use_in_lib++;
        if (use_in_pre) use_in_pre++;

        for (j = 0; j < emb->e; j++)
        {
            log_val.copr    = 'C';
            log_val.idx     = j;
            log_val.t       = *t;
            log_val.val     = *co_val;
            LOGREC(LOG_VAL, &log_val, sizeof (log_val), &meta_log_val);

            t++;
            co_val++;
        }

        for (j = 0; j < emb->n_pre_val; j++)
        {
            log_val.copr    = 'P';
            log_val.idx     = j;
            log_val.t       = *t_pre;
            log_val.val     = *pre_val;
            LOGREC(LOG_VAL, &log_val, sizeof (log_val), &meta_log_val);

            t_pre++;
            pre_val++;
        }

        for (j = 0; j < emb->n_addit_val; j++)
        {
            log_val.copr    = 'A';
            log_val.idx     = j;
            log_val.t       = *t_addit;
            log_val.val     = *addit_val;
            LOGREC(LOG_VAL, &log_val, sizeof (log_val), &meta_log_val);

            t_addit++;
            addit_val++;
        }

        for (j = 0; j < emb->n_bundle_val; j++)
        {
            log_val.copr    = 'B';
            log_val.idx     = j;
            log_val.t       = 0;
            log_val.val     = *bundle_val;
            LOGREC(LOG_VAL, &log_val, sizeof (log_val), &meta_log_val);

            bundle_val++;
        }
    }

    fflush (g_log_file);
    return 0;
}
