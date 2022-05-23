/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef HEAP_H
#define HEAP_H

int heapsort (void **, int, double (*) (void *, void *) );
/*                                 ptr1    ptr2
 * pointer to function: function should return value <= 0 if
 * pointervalue1 - pointervalue2 <= 0
 * else: value > 0
 */
#endif
