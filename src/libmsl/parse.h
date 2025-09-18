/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_PARSE__
#define __H_PARSE__

#include "matrix.h"
#include "mod.h"

int P_parse_file(matrix_t mat, mod_s* mod);
int P_parse_str(matrix_t mat, mod_s* mod, const char* str, uint32_t size);
int P_clear();

#endif

