/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "log_meta.h"

#ifndef LOG_C
/* TODO: make this a static in log.c and acces with functions.*/
extern FILE *g_log_file;
extern int   g_log_level;
#endif

/* define LOG_HUMAN (e.g. as gcc option, to log human readable ascii instead of binary. Use when testing. */

#ifdef LOG_HUMAN
#define LOGREC(rectype, pstruct, recsize, pmeta) \
    log_ascii (pmeta)
#else
#define LOGREC(rectype, pstruct, recsize, pmeta) \
    log_bin (rectype, pstruct, recsize)
#endif

typedef enum { LOG_STATISTICS   = 0x0001, /*STAT, ADHD, ADDT            */
               LOG_EMB_PAR      = 0x0002, /*EMBPAR1, EMBPAR2, EMBPAR3   */
               LOG_EMB_VEC      = 0x0004, /*VID, VAL                    */
               LOG_FN_PAR       = 0x0008, /*KPEXP, KPTLS                */
               LOG_NEAR_NEIGH   = 0x0010, /*TG1, NN                     */
               LOG_SET_PAR      = 0x0020, /*SPCL, SPKF, SPLO, SPBT, SPUV*/
               LOG_LIB_SET      = 0x0040, /*SH, SD                      */
               LOG_PRE_SET      = 0x0080, /*SH, SD                      */
               LOG_PREDICTED    = 0x0100, /*TG2, PD                     */
               LOG_MSGS         = 0x0200, /*MSG                         */
               LOG_VAR_PAR      = 0x0400, /*VPM, VPT                    */
               LOG_BUNDLE_PAR   = 0x0800  /*BP                          */
} Tlog_levels;

int
open_log_file (const char *file_name, int log_level, char *sep, char *string_quote);

int
open_bin_log_file (const char *file_name, int log_level);

int
close_log_file (void);

int
log_message (const char *format, ...);

int
log_bin (int rec_type, void *s, int rec_size);

int
log_ascii (Trec_meta *m);

int
read_rec_bin (FILE *i_log_file, void *rec_list[], int *idx);

#endif
