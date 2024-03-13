
/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Various functions for use in function sources.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "point.h"
#include "log.h"
#include "fn.h"
#include "fn_log.h"

int
log_fn (Tfn_type fn_type)
{
    static struct s_log_kp log_kp;

#ifdef LOG_HUMAN
    ATTACH_META_KP(meta_log_kp, log_kp);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_FN_PAR))
        return 2;

    strncpy(log_kp.fn_type, fn_type == FN_EXP? "exp": "tls", 4);

    LOGREC(LOG_KP, &log_kp, sizeof (log_kp), &meta_log_kp);

    return 0;
}

int
log_nn (Tpoint *target, Tpoint **rs, double *sqdst, int n)
{
    Tpoint  **prs;
    double  *psqdst;
    int     i;
    static struct s_log_tg1 log_tg1;
    static struct s_log_nn log_nn;
#ifdef LOG_HUMAN
    ATTACH_META_TG1(meta_log_tg1, log_tg1);
    ATTACH_META_NN(meta_log_nn, log_nn);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_NEAR_NEIGH))
        return 2;

    log_tg1.target_num = target->vec_num;

    LOGREC(LOG_TG1, &log_tg1, sizeof (log_tg1), &meta_log_tg1);

    psqdst = sqdst;
    prs    = rs;
    for (i = 0; i < n; i++)
        if (*prs) 
        {
            log_nn.seq = i;
            log_nn.num = (*prs)->vec_num;
            log_nn.sqdst = *psqdst;

            LOGREC(LOG_NN, &log_nn, sizeof (log_nn), &meta_log_nn);

            prs++;
            psqdst++;
        }

    return 0;
}

int
log_predicted (Tpoint **pt, int n, int n_val, double *pre_val, int *status)
{
    int    i, j, *s;
    double *p;
    static struct s_log_tg2 log_tg2;
    static struct s_log_pd log_pd;
#ifdef LOG_HUMAN
    ATTACH_META_TG2(meta_log_tg2, log_tg2);
    ATTACH_META_PD(meta_log_pd, log_pd);
#endif

    if (!g_log_file)
        return 1;

    if (!(g_log_level & LOG_PREDICTED))
        return 2;

    p = pre_val;
    s = status;
    for (i = 0; i < n; i++)
    {
        log_tg2.target_num = (*pt)->vec_num;
        LOGREC(LOG_TG2, &log_tg2, sizeof (log_tg2), &meta_log_tg2);


        for (j = 0; j < n_val; j++)
        {
            log_pd.pre_val_num = j;
            log_pd.pre_val     = *p++;               /*predicted*/
            log_pd.obs_val     = (*pt)->pre_val[j];  /*observed*/
            if (status)
                log_pd.status = *s++;
            else 
                log_pd.status = 0;
            LOGREC(LOG_PD, &log_pd, sizeof (log_pd), &meta_log_pd);
        }

        pt++;
    }

    return 0;
}
