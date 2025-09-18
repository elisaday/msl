/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_MODULE__
#define __H_MODULE__

#include "obj.h"
#include "matrix.h"

int MOD_init(mod_s* mod, string_s* name);
int MOD_init_dll(matrix_t mat, mod_s* mod, string_s* name);
void MOD_free(mod_s* mod);

int MOD_get_obj_by_idx(mod_s* mod, uint32_t idx, obj_s* obj);
int MOD_get_obj_idx(mod_s* mod, string_s* name, uint32_t* idx);
int MOD_get_obj(mod_s* mod, string_s* name, obj_s* obj);

#endif

