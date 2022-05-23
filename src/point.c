/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Helper functions for points.
 *
 */

#include <stdlib.h>
#include "point.h"

double *
get_co_vec (Tpoint *point)
{
    return point->co_val;
}

long *
get_t_vec (Tpoint *point)
{
    return (point->t);
}

double *
get_pre_vec (Tpoint *point)
{
    return point->pre_val;
}

static Texcl l_excl;
static int   l_var_win;
static int   l_e;

int 
exclude_init (Texcl excl, int var_win, int e)
{
        l_excl    = excl;
        l_var_win = var_win;
        l_e       = e;
}

bool 
exclude (Tpoint *tg, Tpoint *cd)  /*exclude candidate from prediction set for target?*/
{
    if (l_excl & (T_EXCL_TIME_COORD + T_EXCL_TIME_WIN + T_EXCL_SELF) &&
            tg == cd)
        return true;

    if (l_excl & T_EXCL_TIME_COORD)
    {
        /*exclude if vectors share time coordinates*/
        long *t_tg, *t_cd;
        short *vn_tg, *vn_cd;
        vn_tg = tg->co_var_num;
        for (t_tg = tg->t; t_tg < tg->t + l_e; t_tg++)
        {
            vn_cd = cd->co_var_num;
            for (t_cd = cd->t; t_cd < cd->t + l_e; t_cd++)
            {
                if (*t_tg == *t_cd && *vn_tg == *vn_cd)
                    return true;
                vn_cd++;
            }
            vn_tg++;
        }
    }

    if(l_excl & T_EXCL_TIME_WIN)
    {
        /*exclude if vectors are too close in time*/
        if (abs ((*tg->t) - (*cd->t)) < l_var_win)
            return true;
    }

    return false;
}
