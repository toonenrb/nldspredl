/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#include "datalimits.h"
#include "tsdat.h"
#include "tsfile.h"

#define MAX_LINE_SZ    5000

typedef enum { DT_TM, DT_MAPVAL, DT_MISC, DT_ID, DT_LIB, DT_PRE, DT_BUNDLE } Tdt;

typedef struct
{
    char    nm[MAX_COLNAME_SZ];
    Tdt     tp;

} Tmeta_col;

int
csv_parse ( char *line, char *list[], int size );

int
open_head (FILE *, bool, char *, char *, char *, char *, char *,
           char [][MAX_COLNAME_SZ], char [][MAX_COLNAME_SZ], Tmeta_col *, int *);
int
read_dat (FILE *, char *, Tmeta_col *, int, Tfdat *);

Tfdat *
csv_to_fdat (FILE *fi, bool head, char *sep,
             char *tm_col, char *id_col, char *data_cols, char *in_lib_col, char *in_pre_col, char *bundle_cols)
{
    Tmeta_col mcol[MAX_COLS];
    char      col_exp_all[MAX_COLS][MAX_COLNAME_SZ];
    char      col_bundle[MAX_COLS][MAX_COLNAME_SZ];
    char      *nm;
    int       colno, n_col;
    Tfdat     *dt;

    if (!sep)
        return NULL;
    for (nm = strtok (data_cols, sep), colno = 0; nm && colno < MAX_COLS - 1; nm = strtok (NULL, sep), colno++)
    {
        if (strlen(nm) >= MAX_COLNAME_SZ)
        {
            printf ("Name of column too big <%d>, max <%d>\n", (int) strlen(nm), MAX_COLNAME_SZ-1);
            return NULL;
        }
        strcpy (col_exp_all[colno],nm);
    }

    *col_exp_all[colno] = '\0';

    for (nm = strtok (bundle_cols, sep), colno=0; nm && colno < MAX_COLS - 1; nm = strtok (NULL,sep), colno++)
    {
        if (strlen(nm) >= MAX_COLNAME_SZ)
        {
            printf ("Name of column too big <%d>, max <%d>\n", (int) strlen(nm), MAX_COLNAME_SZ-1);
            return NULL;
        }
        strcpy (col_bundle[colno],nm);
    }

    *col_bundle[colno] = '\0';

    if (open_head (fi, head, sep, tm_col, id_col, in_lib_col, in_pre_col,
                   col_bundle, col_exp_all, mcol, &n_col) != 0)
        return NULL;

    dt = (Tfdat *) malloc (sizeof(Tfdat));
    if (read_dat (fi, sep, mcol, n_col, dt) != 0)
    {
        free_fdat (dt);
        return NULL;
    }
    return dt;
}

int
open_head (FILE *fi, bool head, char *sep,
           char *tm_col, char * id_col, char * in_lib_col, char * in_pre_col, char col_bundle[][MAX_COLNAME_SZ],
           char col_exp_all[][MAX_COLNAME_SZ], Tmeta_col *mcol, int *n_colf)
{
    char ln[MAX_LINE_SZ];
    char *rt, *pc;
    char *collist[MAX_COLS + 1];
    int  n_col, col, c, rc;

    if ((rt = fgets (ln, MAX_LINE_SZ, fi)) == NULL)
    {
        printf ("Could not read header line of file.\n");
        return -1;
    }
    if (ln[strlen(ln) - 1] != '\n')
    {
        printf ("First line longer than max line size <%d>.\n", (int) MAX_LINE_SZ);
        return -1;
    }
    for (pc = ln; pc < (ln + strlen(ln)); pc++)
        if (*pc == '\n') *pc = '\0';

    n_col = csv_parse (ln, collist, MAX_COLS + 1);

    if (n_col > MAX_COLS)
    {
        printf ("Too many columns in file.\n");
        return -1;
    }

    for (col = 0; col < n_col; col++)
    {
        if (head)
        {
            if (strlen(collist[col]) >= MAX_COLNAME_SZ)
            {
                printf ("Column name too big <%s>.\n", collist[col]);
                return -1;
            }
            strcpy (mcol[col].nm, collist[col]);
        }
        else
            sprintf (mcol[col].nm, "%d", col);

        /* Check column use.*/
        mcol[col].tp = DT_MISC;
        for (c = 0; c < MAX_COLS && *col_exp_all[c] != '\0'; c++)
        {
            if ((rc = strcmp (col_exp_all[c], mcol[col].nm)) == 0)
            {
                mcol[col].tp = DT_MAPVAL;
                break;
            }
        }

        for (c = 0; c < MAX_COLS && *col_bundle[c] != '\0'; c++)
        {
            if ((rc = strcmp(col_bundle[c], mcol[col].nm)) == 0)
            {
                mcol[col].tp = DT_BUNDLE;
                break;
            }
        }

        if (tm_col)
            if ((rc = strcmp (tm_col, mcol[col].nm)) == 0)
                mcol[col].tp = DT_TM;

        if (id_col)
            if ((rc = strcmp (id_col, mcol[col].nm)) == 0)
                mcol[col].tp = DT_ID;

        if (in_lib_col)
            if ((rc = strcmp (in_lib_col, mcol[col].nm)) == 0)
                mcol[col].tp = DT_LIB;

        if (in_pre_col)
            if ((rc = strcmp (in_pre_col, mcol[col].nm)) == 0)
                mcol[col].tp = DT_PRE;
    }
    *n_colf = n_col;

    if (!head)
        rewind (fi);

    return 0;
}

int
read_dat (FILE *fi, char *sep, Tmeta_col *mc, int n_colf, Tfdat *dt)
{
    char   ln[MAX_LINE_SZ];
    char   *pc, *chk;
    int    nct, ncr, i, nmap, nbundle; /*#col total, #col read*/
    long   msz = 200, sz, tidx;        /*malloc size, total size, time index*/
    double *dat_val, *bundle_val;
    bool   tm_avail;
    long   lno = 0;
    char   *collist[MAX_COLS + 1];
    char   *previd="q2$%Hbnj";         /*some random sequence*/
    bool   id_used = false, in_lib_used = false, in_pre_used = false;

    /* Initialize data struct */
    dt->n_dat    = 0;
    dt->n_col    = 0;
    dt->t      = NULL;
    dt->id     = NULL;
    dt->in_lib = NULL;
    dt->in_pre = NULL;
    dt->dat    = NULL;
    dt->bundle       = NULL;
    dt->bundle_lab   = NULL;
    dt->n_bundle_col = 0;
    nbundle          = 0;

    for (i = 0; i < n_colf; i++)
    {
        if (mc[i].tp == DT_MAPVAL)
            dt->n_col++;
        if (mc[i].tp == DT_ID)
            id_used = true;
        if (mc[i].tp == DT_LIB)
            in_lib_used = true;
        if (mc[i].tp == DT_PRE)
            in_pre_used = true;
        if (mc[i].tp == DT_BUNDLE)
            dt->n_bundle_col++;
    }

    dt->lab = (char **) malloc (dt->n_col * sizeof(char *));
    nmap = 0;
    for (i = 0; i < n_colf; i++)
    {
        if (mc[i].tp == DT_MAPVAL)
        {
            dt->lab[nmap] = (char *) malloc ((strlen (mc[i].nm) + 1) * sizeof(char));
            strcpy (dt->lab[nmap], mc[i].nm);
            nmap++;
        }
    }

    if (dt->n_bundle_col)
    {
        dt->bundle_lab = (char **) malloc (dt->n_bundle_col * sizeof(char *));
        nbundle = 0;
        for (i = 0; i < n_colf; i++)
        {
            if (mc[i].tp == DT_BUNDLE)
            {
                dt->bundle_lab[nbundle] = (char *) malloc ((strlen(mc[i].nm) + 1) * sizeof(char));
                strcpy (dt->bundle_lab[nbundle], mc[i].nm);
                nbundle++;
            }
        }
    }

    sz   = 0;
    tidx = 0;
    while (fgets (ln, MAX_LINE_SZ, fi))
    {
        lno++;
        if (ln[strlen(ln) - 1] != '\n')
        {
            printf ("Line longer than max line size <%d>.\n", (int) MAX_LINE_SZ);
            return -1;
        }

        if (tidx >= sz)
        {
            dt->t      = (long *) realloc(dt->t, (sz + msz) * sizeof(long));

            dt->dat = (double *) realloc(dt->dat, (sz + msz) * nmap * sizeof(double));
            dat_val = dt->dat + tidx * nmap;

            if (nbundle)
            {
                dt->bundle = (double *) realloc (dt->bundle, (sz + msz) * nbundle * sizeof(double));
                bundle_val = dt->bundle + tidx * nbundle;
            }

            if (in_lib_used)
                dt->in_lib = (bool *) realloc (dt->in_lib, (sz + msz) * sizeof(bool));
            if (in_pre_used)
                dt->in_pre = (bool *) realloc(dt->in_pre, (sz + msz) * sizeof(bool));

            if (id_used)
                dt->id = (char **) realloc (dt->id, (sz + msz) * sizeof(char *));

            sz += msz;
        }

        nct = csv_parse (ln, collist, MAX_COLS + 1);

        if (nct != n_colf)
        {
            printf ("Number of columns not equal to number on first line. "
                    "Lineno <%ld>, col <%d>, first line <%d>\n", lno, nct, n_colf);
            return -1;
        }

        tm_avail = false;
        for (ncr = 0; ncr < nct; ncr++)
        {
            pc = collist[ncr];   /*pc points to column value*/
            if (mc[ncr].tp == DT_TM)
            {
                dt->t[tidx] = (long) strtol (pc, &chk, 10);

                if (chk == pc)
                {
                    printf ("Error while converting string time value <%s> to long.\n", pc);
                    return -1;
                }

                tm_avail = true;
            }
            else if (mc[ncr].tp == DT_ID)
            {
                if (strcmp(previd, pc) != 0)
                {
                    previd = (char *) malloc ((strlen(pc) + 1) * sizeof(char));
                    strcpy (previd, pc);
                }
                dt->id[tidx] = previd;
            }
            else if (mc[ncr].tp == DT_LIB)
            {
                if (strcmp ("1", pc) == 0)
                    dt->in_lib[tidx] = true;
                else
                    dt->in_lib[tidx] = false;
            }
            else if (mc[ncr].tp == DT_PRE)
            {
                if (strcmp ("1", pc) == 0)
                    dt->in_pre[tidx] = true;
                else
                    dt->in_pre[tidx] = false;
            }
            else if (mc[ncr].tp == DT_BUNDLE)
            {
                *bundle_val = strtod (pc, &chk);
                if (chk == pc)
                    *bundle_val = NAN;
                bundle_val++;
            }
            else if (mc[ncr].tp == DT_MAPVAL)
            {
                if (strlen(pc) >= 2 && strncmp (pc, "NA", 2) == 0)
                {
                    *dat_val = NAN;
                }
                else
                {
                    *dat_val = strtod (pc, &chk);
                    if (chk == pc)
                    {
#if 0
                        printf("Error while converting string value <%s> to double.\n", pc);
                        return -1;
#endif
                        *dat_val = NAN;
                    }
                }
                dat_val++;
            }
            /* other types are skipped */
        }

        if (!tm_avail)
           dt->t[tidx] = tidx;

        tidx++;
    }

    dt->n_dat = tidx;

    return 0;
}

int 
csv_parse (char *line, char *list[], int size)
{
    char *p;
    char *dp;
    int inquote;
    int na;
    char prevc = ',';
 
    dp = NULL;
    inquote = 0;
    na = 0;
    prevc = ',';\

    for (p = line; *p != '\0'; prevc = *p, p++) 
    {
        if (prevc == ',' && !inquote) 
        {
            if (dp != NULL)
                *dp = '\0';
            if (na >= size)
                return na;
            list[na++] = p;
            dp = p;
            if (*p == '"') 
            {
                inquote = 1;
                continue;
            }
        }
        if (inquote && *p == '"')
        {
            if ( p[1] != '"' )
                inquote = 0;
            p++;
        }
        if (*p != ',' || inquote)
            *dp++ = *p; 
    }
    if (dp != NULL)
        *dp = '\0';
    if (na < size)
        list[na] = NULL;
 
    return na;
}

int free_fdat (Tfdat *dt)
{
    int  i;
    char *previd = NULL;

    if (!dt)
        return 0;

    if (dt->id)
    {
        for (i = 0; i < dt->n_dat; i++)
        {
            if (previd != dt->id[i])
            {
                previd = dt->id[i];
                if (dt->id[i]) free (dt->id[i]);
            }
        }
        free (dt->id);
    }
    free (dt->t);
    for (i = 0; i < dt->n_col; i++)
        free (dt->lab[i]);
    for (i = 0; i < dt->n_bundle_col; i++)
        free (dt->bundle_lab[i]);

    if (dt->in_lib) free (dt->in_lib);
    if (dt->in_pre) free (dt->in_pre);
    if (dt->bundle_lab) free (dt->bundle_lab);

    free (dt->lab);
    free (dt->dat);
    if (dt->bundle) free (dt->bundle);
    free (dt);
}
