/*
 * Copyright (c) 2022 Roelof Bart Toonen
 * License: MIT license (spdx.org MIT)
 *
 */

#ifndef MKEMBED_H
#define MKEMBED_H
#include "embed.h"

Tembed *
create_embed (Tfdat *fdat, Temb_lag_def *eld, int emb_num);

int
free_embed (Tembed *emb);

int
log_embed_init (char *_sep, char *fnam, int _logLvl);

int
log_embed_close ();

int
log_embed (Tembed *emb);

#endif
