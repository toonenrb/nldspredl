
/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 * logtobl traverses a binary nlpre log file and outputs a table according to the definitions provided by the caller.
 * These definitions consist of an array of field names, and one record name that specifies when a new table record
 * has to be created. Note: the field names start with the name of the record they themselves are part of, so no need
 * to specify the name of the record to which each field belongs.
 * The log_to_tbl function traverses the complete log file. The value of each field is stored in their respective structs.
 * When the new table record name has been encountered, all required struct values are output as a new record.
 * 
 * The output table is constructed as follows:
 *   The program allocates memory for each column of the table.
 *   Struct ttbl_rec contains the number of columns and number of rows. It also provides a pointer to an array of
 *     Ttbl_col structs.
 *   Each Ttbl_col struct contains the name of the column (the field name), the type of the column, and
 *     a pointer to the allocated memory. When the column is a string type, this pointer will point to an array
 *     of pointers to strings.
 */

#ifndef LOGTOTBL_H
#define LOGTOTBL_H
#include <stdio.h>
#include "log_meta.h"
typedef struct {
    char        name[MAX_COLNAME_LOG];
    Tfield_type field_type;
    int         max_str_sz;  /*in case of string*/
    void        *val;   /*For strings, this will point to an array of pointers to char.                        */
} Ttbl_col;             /*For all other types it will point to an array of the corresponding type              */

typedef struct {
    int         ncol;
    long        nrow;
    Ttbl_col    *col;
} Ttbl_rec;

Ttbl_rec *
log_to_tbl (FILE *logfile,
            char *field_names[], /*zero length name (= "") = terminator*/
            char *rec_new_row);

int
free_tbl (void);
#endif
