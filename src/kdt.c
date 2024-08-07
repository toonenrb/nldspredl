/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * KD-Tree algorithm
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <stdbool.h>
#include <math.h>
/*#include "heap.h" replaced by qsort*/
#include "kdt.h"

typedef struct
{
    void    *obj;
    double  *vec;
    double  val;
    long    id;
} Tsorted;

TkdtNode * kdbranch (Tsorted ***, int, int, long, int);
int nnf (void *, TkdtNode *, int, int, void *[], double *, double *, double *, bool *,
         double * (*)(void *), bool (*)(void *, void *), bool);

#if 0
double compare (Tsorted *a, Tsorted *b)  /*to pass to heapsort*/
{
    return ((Tsorted *)a)->val - ((Tsorted *)b)->val;
}
#endif

int compare_sorted (const void *a, const void *b)  /*to pass to qsort*/
{
    double x = (*(Tsorted **) a)->val;
    double y = (*(Tsorted **) b)->val;

    if (x > y)
        return 1;
    else if (x < y)
        return -1;

    return 0;
}

/* kdtree: make balanced k-d tree for array of pointers to objects containing type double position vectors */
/* p: an array of pointers to objects, which contain a vector.
 * n: the size of p.
 * k: the dimension of the vector in the objects.
 * getvec: a function that extracts the k-dimensional vector from the object.
 */
TkdtNode *kdtree (void *p[], long n, int k, double * (*getvec)(void *))
{
    int      d;
    long     i, j;
    Tsorted  ***sl; /* sortlist points to array of k pointers to n-dimensional sort structs array */
    Tsorted  **Ps;
    Tsorted  *Psh;  /* assist in assigning values to pointers */
    void     *Po;
    TkdtNode *Pnd;
    long     n_valid; /*number of valid objects (without NA's)*/
    double   *vec;

    if (n <= 0)
        return NULL;
    if ((sl = (Tsorted ***) malloc ((k + 1) * sizeof (Tsorted **))) == NULL)    /*last row is for temp values*/
        return NULL;
    Ps = (Tsorted **) malloc (k * sizeof (Tsorted *));

    for (d = 0; d < k + 1; d++)
    {
        sl[d] = (Tsorted **) malloc (n * sizeof (Tsorted *));
        if (d < k)
        {
            Ps[d] = (Tsorted *) malloc (n * sizeof (Tsorted));
            for (i = 0; i < n; i++)
                *(sl[d] + i) = Ps[d] + i;
        }
    }

    /* Fill the sort structures with coordinate values for each dimension */
    n_valid = 0;
    for (i = 0; i < n; i++)
    {
        Po = *(p + i);
        vec = getvec (Po);

        /* Check for NA's*/
        for (j = 0; j < k; j++)
            if (isnan(vec[j]))
                break;

        if (j < k)
            continue;

        for (d = 0; d < k; d++)
        {
            Psh = *(*(sl + d) + n_valid);
            Psh->obj = Po;
            Psh->vec = vec;
            Psh->val = *(Psh->vec + d);
            Psh->id  = n_valid; /*Was i. Check if this goes well*/
        }

        n_valid++;
    }

    /* Sort the arrays with pointers to vectors */
    for(d = 0; d < k; d++)
    {
#if 0
        if ((res = heapsort( (void **) *(sl+d), n_valid, (double (*)(void *, void *)) compare )) < 0)
            return NULL;
#endif
        qsort (*(sl+d), n_valid, sizeof(Tsorted *), compare_sorted);
    }

    /* And build the tree */
    Pnd = kdbranch (sl, 0, k, n_valid, 0);

    /* Release the sort structures */
    for (d = 0; d < k; d++)
        free (Ps[d]);
    for (d = 0; d < (k + 1); d++)
        free (*(sl + d));
    free(sl);
    free(Ps);

    return Pnd;
}

TkdtNode * kdbranch (Tsorted ***sl, int st, int k, long n, int dm) /*dm = rotated dim*/
{
    Tsorted **Psfr, **Psto, **Ps0, **Pstmp, **Pli, **Pri, **Psend;
    TkdtNode *nd;
    int     d;     /*dim*/
    int     id;
    long    m;     /*median position*/
    double  mval;  /*median value*/

    nd = (TkdtNode *) malloc(sizeof(TkdtNode));

    if (n == 1) /*leaf*/
    {
        nd->l = nd->r = NULL;
        nd->vec = (*(*sl + st))->vec;
        nd->obj = (*(*sl + st))->obj;
        nd->dm  = dm;
        nd->val = *(nd->vec + dm);
        return nd;
    }

    Ps0   = *sl + st;
    Pstmp = *(sl + k) + st;
    Psend = Ps0 + n;

    for (Psfr = Ps0; Psfr < Psend; Psfr++)
    {
        /*copy 0 array to temp array in row k*/
        *Pstmp = *Psfr;
        Pstmp++;
    }

    /* Find median value index m of dimension 0 */
    Pstmp = *(sl + k) + st;
    for (m = (int) n / 2; m > 0; m--)
        if ((*(Pstmp + m - 1))->val < (*(Pstmp + m))->val)
            break;

    nd->vec = (*(Pstmp + m))->vec;
    nd->obj = (*(Pstmp + m))->obj;
    nd->dm  = dm;
    nd->val = *(nd->vec + dm);
    mval    = nd->val;
    id      = (*(Pstmp + m))->id;

    /* Split arrays and rotate dimensions */
    for(d = 1; d < k; d++)
    {
        Pli   = *(sl + d - 1) + st; /*start of left subdivision*/
        Pri   = Pli + m + 1;      /*start of right subdivision*/
        Psend = *(sl + d) + st + n;
        for (Psfr = *(sl + d) + st; Psfr < Psend; Psfr++)
        {
            if ((*Psfr)->id == id)
                continue;

            if (((*Psfr)->vec[dm]) < mval)
            {
                Psto = Pli;
                Pli++;
            }
            else 
            {
                Psto = Pri;
                Pri++;
            }
            *Psto = *Psfr;
        }
    }
    /* Put temp array back into last dimension */
    Psto  = *(sl + k - 1) + st;
    Psend = *(sl + k) + st + n;
    for (Pstmp = (*(sl + k) + st); Pstmp < Psend; Pstmp++)
    {
        *Psto = *Pstmp;
        Psto++;
    }

    nd->l = nd->r = NULL;
    if (m > 0) 
        nd->l = kdbranch (sl, st, k, m, (dm + 1) % k);
    if ((n - m - 1) > 0) 
        nd->r = kdbranch (sl, st + m + 1, k, n - m - 1, (dm + 1) % k);

    return nd;
}

bool ball_within_bounds (double *tg, int k, double *hr_l, double *hr_h, double *sqdst)
{
    int i;
    double dsq;
    
    for (i = 0; i < k; i++)
    {
        dsq = *(tg + i) - *(hr_l + i);
        dsq *= dsq;
        if (dsq <= *sqdst)
            return false;
        dsq = *(tg + i) - *(hr_h + i);
        dsq *= dsq;
        if(dsq <= *sqdst)
            return false;
    }

    return true;
}

bool bounds_overlap_ball (double *tg, int k, double *hr_l, double *hr_h, double *sqdst)
{
    double pp, d, _sqdst = 0.0;
    int    i;

    for (i = 0; i < k; i++)
    {
        pp = *(tg + i);
        if (pp < *(hr_l + i))
            d = pp - *(hr_l + i);
        else if (pp > *(hr_h + i))
            d = *(tg + i) - *(hr_h + i);
        else 
            d = 0.0;

        _sqdst += d * d;
    }
    if (_sqdst < *sqdst)
        return true;
    return false;
}


int nnf (void *tgob /*target obj*/, TkdtNode *nd, int k, int n, void *rs[], double *sqdst, 
         double *hr_l, double *hr_h, bool *bwb, double * (*getvec)(void *), bool (*exclude)(void *, void *),
         bool object_only_once)
{
    double *pcv, *ptv, d, sqd = 0.0, hr_tmp;
    int    i, j;
    double *tg; /*target vector*/

    tg = getvec (tgob);

    /*traverse down*/
    if (*(tg + nd->dm) < nd->val)
    {
        if (nd->l)
        {
            hr_tmp         = *(hr_h + nd->dm);
            *(hr_h + nd->dm) = nd->val;
            nnf (tgob, nd->l, k, n, rs, sqdst, hr_l, hr_h, bwb,
                    (double * (*)(void *)) getvec, (bool (*)(void *, void *)) exclude, object_only_once);
            *(hr_h + nd->dm) = hr_tmp;
        }
    }
    else
    {
        if (nd->r)
        {
            hr_tmp         = *(hr_l + nd->dm);
            *(hr_l + nd->dm) = nd->val;
            nnf (tgob,nd->r, k, n, rs, sqdst, hr_l, hr_h, bwb,
                    (double * (*)(void *)) getvec, (bool (*)(void *, void *)) exclude, object_only_once);
            *(hr_l + nd->dm) = hr_tmp;
        }
    }

    if (*bwb) \
        return 0; /*lower results already had ball within bounds*/

    /*check nodes own value against result set         */
    /*replace item in result set: order: large to small*/
    pcv = nd->vec;
    for (ptv = tg; ptv < (tg + k); ptv++)
    {
        d = *ptv - *pcv;
        sqd += d * d;
        pcv++;
        if (sqd > *sqdst)
            break;
    }

    if (!exclude (tgob, nd->obj))
    {
        for (i = 0; i < n; i++)
            if (sqd > *(sqdst + i))
                break;

        if (i > 0)
        {
            if (!(object_only_once && *(rs + i - 1) == nd->obj))
            {
                for (j = 0; j < i; j++)
                {
                    if (j == (i - 1))
                    {
                        *(sqdst + j) = sqd;
                        *(rs + j)    = nd->obj;
                    }
                    else
                    {
                        *(sqdst + j) = *(sqdst + j + 1);
                        *(rs + j)    = *(rs + j + 1);
                    }
                }
            }
        }
    }

    /*recursive call on further son, if necessary*/
    if (*(tg + nd->dm) < nd->val)
    {
        if (nd->r)
        {
            hr_tmp         = *(hr_l + nd->dm);
            *(hr_l + nd->dm) = nd->val;
            if (bounds_overlap_ball (tg, k, hr_l, hr_h, sqdst))
                nnf (tgob, nd->r, k, n, rs, sqdst, hr_l, hr_h, bwb,
                        (double * (*)(void *)) getvec,(bool (*)(void *, void *)) exclude, object_only_once);
            *(hr_l + nd->dm) = hr_tmp;
        }
    }
    else
    {
        if (nd->l)
        {
            hr_tmp         = *(hr_h + nd->dm);
            *(hr_h + nd->dm) = nd->val;
            if (bounds_overlap_ball (tg, k, hr_l, hr_h, sqdst))
                nnf (tgob,nd->l, k, n, rs, sqdst, hr_l, hr_h, bwb,
                        (double * (*)(void *)) getvec,(bool (*)(void *, void *)) exclude, object_only_once);
            *(hr_h + nd->dm) = hr_tmp;
        }
    }

    *bwb = ball_within_bounds (tg, k, hr_l, hr_h, sqdst);

    return 0;
}

static double *l_hr_l = NULL, *l_hr_h = NULL;  /*hyperrectangle boundaries*/
static int    l_k_al = 0;

/* kdt_nn: Find n nearest neighbors, with the possibility of providing a function to exclude
 *         some objects from the result set.
 *
 * tgob: the target object.
 * nd: the node where the search will start (usually the root).
 * k: the dimension of the space.
 * n: the number of nearest neighbors to return.
 * rs: the result set, consisting of pointers to objects. Length = n. Allocation in calling function.
 *     If such a pointer is NULL, there were no more objects.
 * sqdst: an array of length n, containing the square distances to tgob  of the objects in 
 *        the result set. Allocation in calling function.
 * getvec: function that retrieves the vector from the object.
 * exclude: function that excludes objects, based upon their relation to the target.
 * object_only_once: If the tree contains nodes that point to the same object, only one of
 *                   those nodes will be selected.
 * Returns: the number of nodes found.
 */
int kdt_nn (void *tgob /*target obj*/, TkdtNode *nd, int k, int n /*n nearest neighb.*/, 
            void *rs[], double *sqdst,
            double * (*getvec)(void *), bool (*exclude)(void *, void *),
            bool object_only_once)
{
    int  i;
    int  n_missing;
    bool bwb = false;

    if (k != l_k_al)
    {
        l_hr_l = (double *) realloc (l_hr_l, k * sizeof(double)); /*lowest*/
        l_hr_h = (double *) realloc (l_hr_h, k * sizeof(double)); /*highest*/
        l_k_al = k;
    }

    for (i = 0; i < k; i++)
    {
        *(l_hr_l + i) = -DBL_MAX;
        *(l_hr_h + i) = DBL_MAX;
    }

    for (i = 0; i < n; i++) 
    {
        *(sqdst + i) = DBL_MAX;
        *(rs + i)    = NULL;
    }

    nnf (tgob, nd, k, n, rs, sqdst, l_hr_l, l_hr_h, &bwb,
          (double * (*)(void *)) getvec, (bool (*)(void *, void *)) exclude, object_only_once);

    for (n_missing = 0; n_missing < n; n_missing++)
        if (rs[n_missing])
            break;

    return n - n_missing;

}

int insert_node (TkdtNode *node, TkdtNode *branch, int k)
{
    if (node->vec[branch->dm] < branch->val)
    {
        if (branch->l)
            return insert_node (node, branch->l, k);
        else
            branch->l = node;
    }
    else
    {
        if (branch->r)
            return insert_node (node, branch->r, k);
        else
            branch->r = node;
    }

    node->dm = (branch->dm + 1) % k;
    node->val = node->vec[node->dm];

    return 0; 
}

/* kdt_insert: Insert a node in an existing tree */
int kdt_insert (void *obj, TkdtNode *nd, int k, double * (*getvec)(void *))
{
    TkdtNode *ndi;
    double   *vec;
    int i;

    if (obj == NULL)
        return -1;

    if (nd == NULL)
        return -2;

    vec = getvec(obj);
    if (vec == NULL)
        return -3;

    for (i = 0; i < k; i++)
    {
        if (isnan(vec[i]))
            return -4;
    }

    /* Create the node.*/
    ndi = (TkdtNode *) malloc(sizeof(TkdtNode));
    ndi->l = NULL;
    ndi->r = NULL;
    ndi->vec = vec;
    ndi->obj = obj;

    return (insert_node (ndi, nd, k));
}

void free_kdt (TkdtNode *nd)
{
    if (nd->l)
        free_kdt (nd->l);
    if (nd->r)
        free_kdt (nd->r);
    
    free (nd);

    if (l_hr_l)
        free (l_hr_l);
    
    l_hr_l = NULL;
    
    if (l_hr_h)
        free (l_hr_h);
    
    l_hr_h = NULL;
    l_k_al = 0;
}

int kdt_print (TkdtNode *nd, int k, int lvl)
{
    int i;

    if (nd == NULL) return 0;
    for (i = 0; i < lvl; i++) printf("  ");
    printf("%d", nd->dm);
    for (i = 0; i < k; i++) printf(", %f", nd->vec[i]);
    printf("\n");
    for (i = 0; i < lvl; i++) printf("  ");
    printf("l\n");
    kdt_print (nd->l, k, lvl + 1);
    for (i = 0; i < lvl; i++) printf("  ");
    printf("r\n");
    kdt_print (nd->r, k, lvl + 1);
    return 0;
}
