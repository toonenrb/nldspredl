/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOG_C
#include "log.h"
#include "log_meta.h"

FILE     *g_log_file;
int      g_log_level;

static char l_sep[2];
static char l_string_quote[2];

static char l_begin_byte = 2;
static char l_end_byte = 3;

static char l_begin_record = 2;
static char l_end_record = 3;

int
open_log_file (const char *file_name, int log_level, char *sep, char *string_quote)
{
    if (!file_name)
        return 1;

    g_log_file = fopen (file_name, "w+");
    if (!g_log_file)
        return -1;

    g_log_level = log_level;
    strncpy (l_sep, sep, 1);
    l_sep[1] = '\0';
    strncpy (l_string_quote, string_quote, 1);
    l_string_quote[1] = '\0';

    return 0;
}

int
open_bin_log_file (const char *file_name, int log_level)
{
    if (!file_name)
        return 1;

    g_log_file = fopen (file_name, "w+");
    if (!g_log_file)
        return -1;

    g_log_level = log_level;

    return 0;
}

int
close_log_file (void)
{
    if (g_log_file)
        fclose (g_log_file);
    g_log_level = 0;
    return 0;
}

int
log_ascii (Trec_meta *m)
{
    int i;
    Tfield_meta *fm;

    if (!g_log_file)
        return 1;

    fprintf (g_log_file, "%s", m->name);

    for (i = 0; i < m->nfield; i++)
    {
        fm = m->field_meta + i;
        switch (fm->field_type)
        {
          case FT_CHAR:
            fprintf (g_log_file, "%s%c", l_sep, *(char *)fm->val);
            break;
          case FT_STRING:
            fprintf (g_log_file, "%s%s%s%s", l_sep, l_string_quote, (char *)fm->val, l_string_quote);
            break;
          case FT_SHORT:
            fprintf (g_log_file, "%s%hd", l_sep, *(short *)fm->val);
            break;
          case FT_INT:
            fprintf (g_log_file, "%s%d", l_sep, *(int *)fm->val);
            break;
          case FT_LONG:
            fprintf (g_log_file, "%s%ld", l_sep, *(long *)fm->val);
            break;
          case FT_DOUBLE:
            fprintf (g_log_file, "%s%e", l_sep, *(double *)fm->val);
            break;
        }
    }

    fprintf (g_log_file, "\n");
    return 0;
}

int
log_bin (int rec_type, void *s, int rec_size)
{
    if (!g_log_file)
        return 1;

    fwrite (&l_begin_byte, sizeof (l_begin_byte), 1, g_log_file);

    fwrite (&rec_type, sizeof (int), 1, g_log_file);

    fwrite (&rec_size, sizeof (int), 1, g_log_file);

    fwrite (s, rec_size, 1, g_log_file);

    fwrite (&l_end_byte, sizeof (l_end_byte), 1, g_log_file);
    
    return 0;
}

/* read_bin reads the next record from a binary log file.
 * Input is an array of pointers to record structures where the index of each structure corresponds to its
 * Tlog_rec_type number.
 * When successful, the function the fills the index of the structure that was successfully read.
 */
int
read_rec_bin (FILE *i_log_file, void *rec_list[], int *idx)
{
    static char one_byte;
    static int  rec_size, rec_type, items_read;

    if (!i_log_file)
        i_log_file = g_log_file;

    if (!i_log_file)
        return 1;
    *idx = -1;

    if (feof (i_log_file))
        return 0;

    one_byte = 0;
    if ((items_read = fread (&one_byte, sizeof (one_byte), 1, i_log_file)) < 1)
        return -1;;

    if (one_byte != 2)
        return -2;

    if ((items_read = fread (&rec_type, sizeof (int), 1, i_log_file)) < 1)
        return -3;;

    if (rec_type < 0 || rec_type >= N_LOG_REC)
        return -4;

    rec_size = 0;
    if ((items_read = fread (&rec_size, sizeof (int), 1, i_log_file)) < 1)
        return -5;

    if (rec_size < 1)
        return -6;

    if ((items_read = fread (rec_list[rec_type], rec_size, 1, i_log_file)) < 1)
        return -7;

    one_byte = 0;
    if ((items_read = fread (&one_byte, sizeof (one_byte), 1, i_log_file)) < 1)
        return -8;

    if (one_byte != 3)
        return -9;

    *idx = rec_type;

    return rec_size;
}

int
log_message (const char *format, ...)
{
    va_list arglist;

    if (!g_log_file)
        return 1;
    if (!(g_log_level & LOG_MSGS))
        return 2;

    fprintf (g_log_file, "MSG%s%s", l_sep, l_string_quote);
    va_start (arglist, format);
    vfprintf (g_log_file, format, arglist);
    va_end (arglist);

    fprintf (g_log_file, "%s\n", l_string_quote);
    return 0;
}
