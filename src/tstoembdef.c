/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "tstoembdef.h"

typedef enum { LD_TRAIL = 0, LD_SHIFT = 1 } Trange_method;
typedef enum { VT_COORD, VT_PRE, VT_ADDIT } Tvar_type;
typedef struct
{
    char *var_name;
    int  from, to, step;
    Trange_method method;
    Tvar_type     var_type;
} Tlag_def_range;

Tlag_def_range
*str_to_range (char *str, int *_n);

void
free_range (Tlag_def_range *ldr);
Temb_lag_def *
range_to_lag_def (Tlag_def_range *ldr, int n_range_def,
                  Temb_lag_def *current_eld, int n_skip, bool first_of_ldr, int *_n);

#ifdef MINGW
/*mingw does not have strtok_r in its string.h*/
/* 
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *  http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 * (Declaration that it's public domain):
 *  http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* strtok_r( char *str, const char *delim, char **nextp)
{
    char *ret;

    if (str == NULL)
    {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0')
    {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str)
    {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
}
#endif

Temb_lag_def *
add_emb_lag_def (Temb_lag_def **peld, int *n_lag_def)
{
    Temb_lag_def *new;

    *peld = (Temb_lag_def *) realloc(*peld, (*n_lag_def + 1) * sizeof(Temb_lag_def));
    new = *peld + *n_lag_def;

    new->n_co_lag    = 0;
    new->n_pre_lag   = 0;
    new->n_addit_lag = 0;
    new->co_lag    = NULL;
    new->pre_lag   = NULL;
    new->addit_lag = NULL;

    new->n_ranges  = 0;
    new->range_dim = NULL;
    new->range_tau = NULL;

    (*n_lag_def)++;

    return new;  /*pointer to new entry*/
}

Tlag_var *
add_lag_var (Tlag_var **plag_var, int *n_lag, char *var_name, int lag)
{
    Tlag_var *new;

    *plag_var = (Tlag_var *) realloc(*plag_var, (*n_lag + 1) * sizeof(Tlag_var));
    new = *plag_var + *n_lag;

    new->var_name = var_name;
    new->lag      = lag;
    (*n_lag)++;

    return new;
}

int
add_range_par (Temb_lag_def *peld, int dim, int tau)
{
    peld->range_dim = realloc (peld->range_dim, (peld->n_ranges + 1) * sizeof (int));
    peld->range_tau = realloc (peld->range_tau, (peld->n_ranges + 1) * sizeof (int));
    *(peld->range_dim + peld->n_ranges) = dim;
    *(peld->range_tau + peld->n_ranges) = tau;
    peld->n_ranges++;
}

Temb_lag_def *
str_to_lag_def (char *lag_def_str, int *_n_lag_def)
{
    /* String is not checked for validity. */
    /* coordvarname1,lag1,lag2,..,lagN;coordvarname2,lag1,lag2,..lagM:prevarname1,lag1,lag2,..,lagL;
     * prevarname2,lag1,..,lagK:additvarname1,lag1,..,lagJ;additvarname2,...lagI| etc for second embedding
     * etc   \0 terminated
     */
    char         *lag_def_brk, *save_lag_def;
    char         *var_type_brk, *save_var_type;
    char         *var_brk, *save_var;
    char         *lag_brk, *save_lag;
    bool         first_lag, first_var;
    int          emb_vars;
    char         *var_name, *endptr;
    int          ival;
    int          n_lag_def = 0;
    Temb_lag_def *eld = NULL, *peld;
    Tlag_var     *lag_var;


    for (lag_def_brk = strtok_r (lag_def_str, "|", &save_lag_def);
             lag_def_brk; lag_def_brk = strtok_r (NULL, "|", &save_lag_def))
    {
        peld = NULL;
        emb_vars = 0;
        for (var_type_brk = strtok_r (lag_def_brk, ":", &save_var_type);
                 var_type_brk; var_type_brk = strtok_r (NULL, ":", &save_var_type))
        {
            for (var_brk = strtok_r (var_type_brk, ";", &save_var); var_brk; var_brk = strtok_r (NULL, ";", &save_var))
            {
                first_lag = true;
                for (lag_brk = strtok_r (var_brk, ",", &save_lag); lag_brk; lag_brk = strtok_r (NULL, ",", &save_lag))
                {
                    if (first_lag)
                    {
                        var_name  = lag_brk;
                        first_lag = false;

                        if (!peld)
                            peld = add_emb_lag_def (&eld, &n_lag_def);
                    }
                    else
                    {
                        ival = strtod (lag_brk, &endptr);
                        if (endptr == lag_brk)
                        {
                            printf ("Error when converting lag value: <%s> <%s>\n", var_name, lag_brk);
                            free_lag_def (eld, n_lag_def);
                            return NULL;
                        }

                        if (emb_vars == 0)
                            add_lag_var (&peld->co_lag, &peld->n_co_lag, var_name, ival);
                        else if (emb_vars == 1)
                            add_lag_var (&peld->pre_lag, &peld->n_pre_lag, var_name, ival);
                        else if (emb_vars == 2)
                            add_lag_var (&peld->addit_lag, &peld->n_addit_lag, var_name, ival);
                    }
                }
            }
            emb_vars++;
        }
    }
    *_n_lag_def = n_lag_def;
    return eld;
}

Tlag_def_range *
str_to_range (char *str, int *_n)
{
    char *var_type_brk, *save_var_type;
    char *var_brk, *save_var;
    char *det_brk, *save_det;
    int  n_det;
    int  n = 0;
    Tvar_type var_type = VT_COORD;

    Tlag_def_range  *ldr = NULL;

    for (var_type_brk = strtok_r (str, ":", &save_var_type);
             var_type_brk; var_type_brk = strtok_r (NULL, ":", &save_var_type))
    {
        for (var_brk = strtok_r (var_type_brk, ";", &save_var); var_brk; var_brk = strtok_r (NULL, ";", &save_var))
        {
            ldr = realloc (ldr, (n + 1) * sizeof (Tlag_def_range));;
            ldr[n].var_type = var_type;

            n_det = 0;
            for (det_brk = strtok_r (var_brk, ",", &save_det); det_brk; 
                    det_brk = strtok_r (NULL, ",", &save_det))
            {
                switch (n_det)
                {
                  case 0:
                    ldr[n].var_name = det_brk;
                    break;
                  case 1:
                    ldr[n].from   = atoi(det_brk);
                    ldr[n].to     = ldr[n].from; /*default*/
                    ldr[n].step   = 1;           /*default*/
                    ldr[n].method = LD_SHIFT;    /*default*/
                    break;
                  case 2:
                    ldr[n].to = atoi(det_brk);
                    break;
                  case 3:
                    ldr[n].step = atoi(det_brk);
                    break;
                  case 4:
                    if (strcmp (det_brk, "shift") == 0)
                        ldr[n].method = LD_SHIFT;
                    else
                        ldr[n].method = LD_TRAIL;
                    break;
                  case 5:
                    fprintf (stderr, "Error in lag definitions.\n");
                    if (ldr)
                        free (ldr);
                    return NULL;
                }

                n_det++;
            }
            n++;
        }

        if (var_type == VT_ADDIT)
            break;
        else
            var_type++;
    }

    *_n = n;
    return ldr;
}

void
free_range (Tlag_def_range *ldr)
{
    if (ldr)
        free (ldr);
}

int
copy_emb_lag_def (Temb_lag_def *to, Temb_lag_def *from, bool deep)
{
    to->n_co_lag    = from->n_co_lag;
    to->n_pre_lag   = from->n_pre_lag;
    to->n_addit_lag = from->n_addit_lag;
    to->n_ranges    = from->n_ranges;

    if (deep)
    {
        to->co_lag    = (Tlag_var *) malloc (from->n_co_lag * sizeof (Tlag_var));
        memcpy (to->co_lag, from->co_lag, from->n_co_lag * sizeof (Tlag_var));

        to->pre_lag   = (Tlag_var *) malloc (from->n_pre_lag * sizeof (Tlag_var));
        memcpy (to->pre_lag, from->pre_lag, from->n_pre_lag * sizeof (Tlag_var));

        to->addit_lag = (Tlag_var *) malloc (from->n_addit_lag * sizeof (Tlag_var));
        memcpy (to->addit_lag, from->addit_lag, from->n_addit_lag * sizeof (Tlag_var));

        to->range_dim = (int *) malloc (from->n_ranges * sizeof (int));
        memcpy (to->range_dim, from->range_dim, from->n_ranges * sizeof (int));

        to->range_tau = (int *) malloc (from->n_ranges * sizeof (int));
        memcpy (to->range_tau, from->range_tau, from->n_ranges * sizeof (int));
    }
    else
    {
        to->co_lag     = from->co_lag;
        to->pre_lag    = from->pre_lag;
        to->addit_lag  = from->addit_lag;
        to->range_dim  = from->range_dim;
        to->range_tau  = from->range_tau;
    }

    return 0;
}

Temb_lag_def *
range_to_lag_def (Tlag_def_range *ldr, int n_range_def, Temb_lag_def *current_eld, int n_skip, bool first_of_ldr, int *_n)
{
    Temb_lag_def *new_eld = NULL, *peld;
    int i, j, n, n_new;

    if (n_range_def == 0)
        return current_eld;

    /*prevent infinite loops and exceptions.*/
    if (ldr->step == 0)
        ldr->step = 1;
    if ((double) (ldr->to - ldr->from) / (double) ldr->step < 0.0)
        ldr->step = -ldr->step;

    n_new = 1 + abs ((ldr->to - ldr->from) / ldr->step);

    new_eld = current_eld;

    if (first_of_ldr)
    {
        int n_tmp;
        n_tmp = n_skip;
        for (i = 0; i < n_new; i++)
            add_emb_lag_def (&new_eld, &n_tmp);
        n = n_tmp - n_skip;
    }
    else
    {
        /* Multiply original amount of definitions with new amount and copy existing defs*/
        if (n_new > 1) /*else new lags can be added to existing lag def's*/
        {
            new_eld = (Temb_lag_def *) realloc (new_eld, (n_skip + *_n * n_new) * sizeof (Temb_lag_def));
            for (i = *_n - 1; i > -1; i--)
                for (j = (i > 0? 0: 1); j < n_new; j++)
                    copy_emb_lag_def (&new_eld[n_skip + i * n_new + j], &new_eld[n_skip + i], j > 0);
        }

        n = *_n * n_new;
    }

    peld = new_eld + n_skip;
    for (i = 0; i < (first_of_ldr? 1: *_n); i++)
    {
        for (j = ldr->from; j <= ldr->to; j += ldr->step)
        {
            if (ldr->method == LD_SHIFT)
            {
                if (ldr->var_type == VT_COORD)
                {
                    add_lag_var (&peld->co_lag, &peld->n_co_lag, ldr->var_name, j);
                    add_range_par (peld, 1, ldr->step);
                }
                else if (ldr->var_type == VT_PRE)
                    add_lag_var (&peld->pre_lag, &peld->n_pre_lag, ldr->var_name, j);
                else
                    add_lag_var (&peld->addit_lag, &peld->n_addit_lag, ldr->var_name, j);
            }
            else
            {
                int k;
                for (k = ldr->from; k <= j; k += ldr->step)
                {
                    if (ldr->var_type == VT_COORD)
                        add_lag_var (&peld->co_lag, &peld->n_co_lag, ldr->var_name, k);
                    else if (ldr->var_type == VT_PRE)
                        add_lag_var (&peld->pre_lag, &peld->n_pre_lag, ldr->var_name, k);
                    else
                        add_lag_var (&peld->addit_lag, &peld->n_addit_lag, ldr->var_name, k);
                }

                if (ldr->var_type == VT_COORD)
                    add_range_par (peld, (j - ldr->from) / ldr->step + 1, ldr->step);
            }
            peld++;
        }
    }

    /*now add next variable*/
    *_n = n;
    ldr++;
    n_range_def--;
    return range_to_lag_def (ldr, n_range_def, new_eld, n_skip, false, _n);
}

Temb_lag_def
*str_range_to_lag_def(char *lag_range_str, int *_n_lag_def)
{
    char *lag_range_brk, *save_lag_range;
    int  n_lag_def, n_lag_def_tot = 0, n_range_def;
    Tlag_def_range *lag_def_range;
    Temb_lag_def   *eld = NULL;

    for (lag_range_brk = strtok_r (lag_range_str, "|", &save_lag_range);
             lag_range_brk; lag_range_brk = strtok_r (NULL, "|", &save_lag_range))
    {
        lag_def_range = str_to_range (lag_range_brk, &n_range_def);
        if (!lag_def_range)
        {
            free_lag_def (eld, n_lag_def_tot);
            return NULL;
        }

        n_lag_def = 0;
        if ((eld = range_to_lag_def (lag_def_range, n_range_def, eld, n_lag_def_tot, true, &n_lag_def)) == NULL)
        {
            fprintf (stderr, "Error when processing range definitions.\n");
            free_range (lag_def_range);
            free_lag_def (eld, n_lag_def_tot);
            return NULL;
        }
        
        n_lag_def_tot += n_lag_def;

        free_range (lag_def_range);
    }

    *_n_lag_def = n_lag_def_tot;
    return eld;
}

void
free_lag_def(Temb_lag_def *eld, int n_lag_def)
{
    Temb_lag_def *peld;
    Tlag_var     *lag;

    if (!eld)
        return;

    for (peld = eld; peld < eld + n_lag_def; peld++)
    {
        if (peld)
        {
            if (peld->co_lag)
                 free(peld->co_lag);
            if (peld->pre_lag)
                 free(peld->pre_lag);
            if (peld->addit_lag)
                 free(peld->addit_lag);
            if (peld->range_dim)
                 free(peld->range_dim);
            if (peld->range_tau)
                 free(peld->range_tau);
        }
    }

    free (eld);
}
