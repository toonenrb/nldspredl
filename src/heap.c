/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * Create a heap sorted array of values *
 *
 */
#include "heap.h"

void
siftdown (void *k[], int s, int e, double (*cmp)(void *, void *)) /*'s'tart, 'e'nd*/
{
    void *t; /*temp*/
    int r, c, w; /*root,child,swap*/

    r = s;
    while ((r * 2 + 1) <= e)
    {
        c = r * 2 + 1;
        w = r;
        if ((*cmp) (*(k + w), *(k + c))<=0)
            w = c;
        if (((c + 1) <= e) && ((*cmp) (*(k + w),*(k + c + 1)) <=0) )
            w = c + 1;
        if (w != r)
        {
            t = *(k + r);
            *(k + r) = *(k + w);
            *(k + w) = t;
            r = w;
        }
        else
            return;
    }
}

int 
heapsort (void *k[], int n, double (*cmp)(void *, void *))
{
    int     s, end;
    void    *t;

    /*make max-heap*/
    for (s = (int) (n - 2) / 2; s >= 0; s--)
        siftdown (k, s, n-1, (double (*)(void *, void *)) cmp);

    end = n - 1;

    /*sort*/
    while (end > 0)
    {
        /*swap*/
        t = *(k + end);
        *(k + end) = *k;
        *k = t;
        end--;
        siftdown (k, 0, end, (double (*)(void *, void *)) cmp);
    }

    return 0;
}
