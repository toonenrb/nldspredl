
/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef KDT_H
#define KDT_H
#include <stdbool.h>


typedef struct kdtNode
{
    struct kdtNode *l, *r;
    double *vec;
    void   *obj;
    int    dm;
    double val;
} TkdtNode;

TkdtNode *kdtree (void **, long, int, double * (*)(void *));
void free_kdt (TkdtNode *);
int kdt_nn (void *, TkdtNode *, int, int, void **, double *,
            double * (*)(void *), bool (*)(void *, void *), bool);
int kdt_insert (void *obj, TkdtNode *nd, int k, double * (*getvec)(void *));

/*for debugging*/
int kdt_print (TkdtNode *, int, int);

#endif
