/* Copyright (c) 1999,2000 Cobalt Networks, Inc. */
/* $Id: i18n.h,v 1.1.1.1 2003/06/10 22:42:01 iceblink Exp $ */

#ifndef COMMON_I18N_H
#define COMMON_I18N_H

#include "crfs_func.h"

int init_i18n(char *lang);
char *i18n_lookup_str(unsigned int id);
int i18n_parse_file(CO_FILE *fp);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
