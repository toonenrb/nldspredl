/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Convert a binary logfile to a table format.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"
#include "log_meta.h"
#include "logtotbl.h"

/* The following includes are needed to include the log struct definitions */
#include "bundle_log.h"
#include "fn_log.h"
#include "fn_exp_log.h"
#include "fn_tls_log.h"
#include "mkembed_log.h"
#include "sets_log.h"
#include "stat_log.h"
#include "traverse_log.h"

static struct s_log_bp l_log_bp;
static struct s_log_bv l_log_bv;
static struct s_log_kp l_log_kp;
static struct s_log_tg1 l_log_tg1;
static struct s_log_tg2 l_log_tg2;
static struct s_log_nn l_log_nn;
static struct s_log_pd l_log_pd;
static struct s_log_kpexp l_log_kpexp;
static struct s_log_kptls l_log_kptls;
static struct s_log_vpm l_log_vpm;
static struct s_log_vpt l_log_vpt;
static struct s_log_embpar1 l_log_embpar1;
static struct s_log_embpar2 l_log_embpar2;
static struct s_log_embpar3 l_log_embpar3;
static struct s_log_vid l_log_vid;
static struct s_log_val l_log_val;
static struct s_log_spcl l_log_spcl;
static struct s_log_spkf l_log_spkf;
static struct s_log_splo l_log_splo;
static struct s_log_spbt l_log_spbt;
static struct s_log_spuv l_log_spuv;
static struct s_log_sh l_log_sh;
static struct s_log_sd l_log_sd;
static struct s_log_stnp l_log_stnp;
static struct s_log_stat l_log_stat;
static struct s_log_adhd l_log_adhd;
static struct s_log_addt l_log_addt;
static struct s_log_dtlso l_log_dtlso;
static struct s_log_dtlsan l_log_dtlsan;
static struct s_log_dtlsav l_log_dtlsav;

/* Attach meta definitions */
ATTACH_META_BP(meta_bp, l_log_bp);
ATTACH_META_BV(meta_bv, l_log_bv);
ATTACH_META_KP(meta_kp, l_log_kp);
ATTACH_META_TG1(meta_tg1, l_log_tg1);
ATTACH_META_TG2(meta_tg2, l_log_tg2);
ATTACH_META_NN(meta_nn, l_log_nn);
ATTACH_META_PD(meta_pd, l_log_pd);
ATTACH_META_KPEXP(meta_kpexp, l_log_kpexp);
ATTACH_META_KPTLS(meta_kptls, l_log_kptls);
ATTACH_META_VPM(meta_vpm, l_log_vpm);
ATTACH_META_VPT(meta_vpt, l_log_vpt);
ATTACH_META_EMBPAR1(meta_embpar1, l_log_embpar1);
ATTACH_META_EMBPAR2(meta_embpar2, l_log_embpar2);
ATTACH_META_EMBPAR3(meta_embpar3, l_log_embpar3);
ATTACH_META_VID(meta_vid, l_log_vid);
ATTACH_META_VAL(meta_val, l_log_val);
ATTACH_META_SPCL(meta_spcl, l_log_spcl);
ATTACH_META_SPKF(meta_spkf, l_log_spkf);
ATTACH_META_SPLO(meta_splo, l_log_splo);
ATTACH_META_SPBT(meta_spbt, l_log_spbt);
ATTACH_META_SPUV(meta_spuv, l_log_spuv);
ATTACH_META_SH(meta_sh, l_log_sh);
ATTACH_META_SD(meta_sd, l_log_sd);
ATTACH_META_STNP(meta_stnp, l_log_stnp);
ATTACH_META_STAT(meta_stat, l_log_stat);
ATTACH_META_ADHD(meta_adhd, l_log_adhd);
ATTACH_META_ADDT(meta_addt, l_log_addt);
ATTACH_META_DTLSO(meta_dtlso, l_log_dtlso);
ATTACH_META_DTLSAN(meta_dtlsan, l_log_dtlsan);
ATTACH_META_DTLSAV(meta_dtlsav, l_log_dtlsav);

static Ttbl_rec *l_tbl_rec = NULL;

static int
init_tbl (Tfield_meta *log_field_meta[], int n_fields, Ttbl_rec **pl_tbl_rec);

static int
tbl_realloc (Ttbl_rec *tbl_rec, void *tbl_col_ptr[], long *n_row_alloc);

static int
init_struct_lists ();

static int
init_field_lists (char *field_names[], Trec_meta *rec_meta[],
                  void *log_field_val[], Tfield_meta *log_field_meta[]);


#define N_ROW_ALLOC 200
#define TBL_MAX_FIELD 100

Ttbl_rec *
log_to_tbl (FILE *logfile,
            char *field_names[], /*zero length name (= "") = terminator*/
            char *rec_new_row)
{
    Trec_meta   *rec_meta[N_LOG_REC];           /*Array of pointers to the meta definitions of the log structs*/
    void        *rec_struct[N_LOG_REC];         /*Array of pointers to the log structs, use for read function*/

    void        *log_field_val[TBL_MAX_FIELD];  /*Array of pointers to field values in log structs*/
    Tfield_meta *log_field_meta[TBL_MAX_FIELD]; /*Array of pointers to the corresponding meta definitions of the fields*/
    void        *tbl_col_ptr[TBL_MAX_FIELD];    /*Array of pointers into current tbl columns, current row*/

    Ttbl_col    *col;

    int         n_fields = 0;
    long        n_row, n_row_alloc;
    int         i, rec_idx, field_idx, len;
    
    int         rec_new_row_idx;

    if (logfile == NULL)
    {
        fprintf (stderr, "Invalid log file pointer\n");
        return NULL;
    }

    init_struct_lists (rec_meta, rec_struct);

    if ((n_fields = init_field_lists (field_names, rec_meta, log_field_val, log_field_meta)) < 1)
    {
        fprintf (stderr, "Error when processing field names <%d>\n", n_fields);
        return NULL;
    }

    if (!rec_new_row)
    {
        fprintf (stderr, "No record defined for new table row.\n");
        return NULL;
    }

    rec_new_row_idx = -1;
    for (i = 0; i < N_LOG_REC; i++)
    {
        if (strncmp (rec_new_row, rec_meta[i]->name, 10) == 0)
        {
            rec_new_row_idx = rec_meta[i]->rec_type;
            break;
        }
    }

    if (rec_new_row_idx < 0)
    {
        fprintf (stderr, "Record for new row does not exist <%s>\n", rec_new_row);
        return NULL;
    }

    init_tbl (log_field_meta, n_fields, &l_tbl_rec);
    col = l_tbl_rec->col;

    n_row = 0;
    n_row_alloc = 0;

    while (read_rec_bin (logfile, rec_struct, &rec_idx) > 0) /*Read a logfile record into its corresponding log struct*/
    {
        if (rec_idx == rec_new_row_idx)
        {
            if (n_row == n_row_alloc)
                tbl_realloc (l_tbl_rec, tbl_col_ptr, &n_row_alloc);

            for (field_idx = 0; field_idx < n_fields; field_idx++)
            {
                switch (col[field_idx].field_type)
                {
                  case FT_CHAR:
                    *(char *)tbl_col_ptr[field_idx] = *(char *)log_field_val[field_idx];
                    tbl_col_ptr[field_idx] = (void *)((char *)tbl_col_ptr[field_idx] + 1);
                    break;
                  case FT_STRING:
                    len = strnlen ((char *) log_field_val[field_idx], col[field_idx].max_str_sz - 1);
                    *(char **)tbl_col_ptr[field_idx] = (char *) malloc ((len + 1) * sizeof (char));
                    strncpy (*(char **)tbl_col_ptr[field_idx], (char *) log_field_val[field_idx], len);
                    (*(char **)tbl_col_ptr[field_idx])[len] = '\0';
                    tbl_col_ptr[field_idx] = (void *)((char **)tbl_col_ptr[field_idx] + 1);
                    break;
                  case FT_INT:
                    *(int *)tbl_col_ptr[field_idx] = *(int *)log_field_val[field_idx];
                    tbl_col_ptr[field_idx] = (void *)((int *)tbl_col_ptr[field_idx] + 1);
                    break;
                  case FT_SHORT:
                    *(short *)tbl_col_ptr[field_idx] = *(short *)log_field_val[field_idx];
                    tbl_col_ptr[field_idx] = (void *)((short *)tbl_col_ptr[field_idx] + 1);
                    break;
                  case FT_LONG:
                    *(long *)tbl_col_ptr[field_idx] = *(long *)log_field_val[field_idx];
                    tbl_col_ptr[field_idx] = (void *)((long *)tbl_col_ptr[field_idx] + 1);
                    break;
                  case FT_DOUBLE:
                    *(double *)tbl_col_ptr[field_idx] = *(double *)log_field_val[field_idx];
                    tbl_col_ptr[field_idx] = (void *)((double *)tbl_col_ptr[field_idx] + 1);
                    break;
                }
            }

            n_row++;
        }
    }

    l_tbl_rec->nrow = n_row;
    return l_tbl_rec;
}

static int
tbl_realloc (Ttbl_rec *tbl_rec, void *tbl_col_ptr[], long *n_row_alloc)
{
    int i;

    for (i = 0; i < tbl_rec->ncol; i++)
    {
        switch (tbl_rec->col[i].field_type)
        {
          case FT_CHAR:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (char));
            /* Reposition tbl pointer to start of added memory*/
            tbl_col_ptr[i] = (char *) tbl_rec->col[i].val + *n_row_alloc;
            break;
          case FT_STRING:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (char *));
            tbl_col_ptr[i] = (char **) tbl_rec->col[i].val + *n_row_alloc;
            break;
          case FT_INT:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (int));
            tbl_col_ptr[i] = (int *) tbl_rec->col[i].val + *n_row_alloc;
            break;
          case FT_SHORT:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (short));
            tbl_col_ptr[i] = (short *) tbl_rec->col[i].val + *n_row_alloc;
            break;
          case FT_LONG:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (long));
            tbl_col_ptr[i] = (long *) tbl_rec->col[i].val + *n_row_alloc;
            break;
          case FT_DOUBLE:
            tbl_rec->col[i].val = realloc (tbl_rec->col[i].val, (N_ROW_ALLOC + *n_row_alloc) * sizeof (double));
            tbl_col_ptr[i] = (double *) tbl_rec->col[i].val + *n_row_alloc;
            break;
        }

    }

    *n_row_alloc += N_ROW_ALLOC;

    return 0;
}

static int
init_tbl (Tfield_meta *log_field_meta[], int n_fields, Ttbl_rec **pl_tbl_rec)
{
    int i;

    l_tbl_rec = (Ttbl_rec *) malloc (sizeof (Ttbl_rec));

    l_tbl_rec->ncol = n_fields;
    l_tbl_rec->nrow = 0;
    l_tbl_rec->col  = (Ttbl_col *) malloc (n_fields * sizeof (Ttbl_col));

    for (i = 0; i < n_fields; i++)
    {
        strncpy (l_tbl_rec->col[i].name, log_field_meta[i]->name, 30);

        l_tbl_rec->col[i].field_type = log_field_meta[i]->field_type;
        l_tbl_rec->col[i].max_str_sz = log_field_meta[i]->str_sz;
        l_tbl_rec->col[i].val        = NULL;
    }

    pl_tbl_rec = &l_tbl_rec;

    return 0;
}

static int
init_field_lists (char *field_names[], Trec_meta *rec_meta[],
                  void *log_field_val[], Tfield_meta *log_field_meta[])
{
    int fn, i, j;

    if (!field_names)
        return -1;

    for (fn = 0; *field_names[fn] != '\0'; fn++)
    {
        /* Check whether field exists */
        for (i = 0; i < N_LOG_REC; i++)
        {
            for (j = 0; j < rec_meta[i]->nfield; j++)
            {
                if (strncmp (rec_meta[i]->field_meta[j].name, field_names[fn], 30) == 0)
                {
                    log_field_meta[fn] = rec_meta[i]->field_meta + j;
                    log_field_val[fn]  = log_field_meta[fn]->val;  /* pointer to value in struct */

                    goto nextfield;
                }
            }
        }
        /* When here, the field was not found.*/
        fprintf (stderr, "Field name not found <%s>\n", field_names[fn]);
        return -2;

        nextfield:
        ;
    }

    return fn;
}

static int
init_struct_lists (Trec_meta *rec_meta[], void *rec_struct[N_LOG_REC])
{
    int i;

    rec_meta[meta_bp.rec_type] = &meta_bp;
    rec_meta[meta_bv.rec_type] = &meta_bv;
    rec_meta[meta_kp.rec_type] = &meta_kp;
    rec_meta[meta_tg1.rec_type] = &meta_tg1;
    rec_meta[meta_tg2.rec_type] = &meta_tg2;
    rec_meta[meta_nn.rec_type] = &meta_nn;
    rec_meta[meta_pd.rec_type] = &meta_pd;
    rec_meta[meta_kpexp.rec_type] = &meta_kpexp;
    rec_meta[meta_kptls.rec_type] = &meta_kptls;
    rec_meta[meta_vpm.rec_type] = &meta_vpm;
    rec_meta[meta_vpt.rec_type] = &meta_vpt;
    rec_meta[meta_embpar1.rec_type] = &meta_embpar1;
    rec_meta[meta_embpar2.rec_type] = &meta_embpar2;
    rec_meta[meta_embpar3.rec_type] = &meta_embpar3;
    rec_meta[meta_vid.rec_type] = &meta_vid;
    rec_meta[meta_val.rec_type] = &meta_val;
    rec_meta[meta_spcl.rec_type] = &meta_spcl;
    rec_meta[meta_spkf.rec_type] = &meta_spkf;
    rec_meta[meta_splo.rec_type] = &meta_splo;
    rec_meta[meta_spbt.rec_type] = &meta_spbt;
    rec_meta[meta_spuv.rec_type] = &meta_spuv;
    rec_meta[meta_sh.rec_type] = &meta_sh;
    rec_meta[meta_sd.rec_type] = &meta_sd;
    rec_meta[meta_stnp.rec_type] = &meta_stnp;
    rec_meta[meta_stat.rec_type] = &meta_stat;
    rec_meta[meta_adhd.rec_type] = &meta_adhd;
    rec_meta[meta_addt.rec_type] = &meta_addt;
    rec_meta[meta_dtlso.rec_type] = &meta_dtlso;
    rec_meta[meta_dtlsan.rec_type] = &meta_dtlsan;
    rec_meta[meta_dtlsav.rec_type] = &meta_dtlsav;

    for (i = 0; i < N_LOG_REC; i++)
        rec_struct[i] = rec_meta[i]->rec; /*pointer to corresponding structure*/

    return 0;
}

int
free_tbl (void)
{
    int i, j;

    if (!l_tbl_rec)
        return -1;

    for (i = 0; i < l_tbl_rec->ncol; i++)
    {
        if (l_tbl_rec->col[i].field_type == FT_STRING)
        {
            for (j = 0; j < l_tbl_rec->nrow; j++)
                if (*(char **)l_tbl_rec->col[i].val + j)
                    free (*((char **)l_tbl_rec->col[i].val + j));
        }

        free (l_tbl_rec->col[i].val);
    }

    free (l_tbl_rec->col);

    free (l_tbl_rec);

    l_tbl_rec = NULL;
}
