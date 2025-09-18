/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_VM__
#define __H_VM__

#include "obj.h"
#include "mod.h"
#include "matrix.h"

int VM_init(matrix_t mat);
void VM_free(matrix_t mat);

int VM_exec_file(matrix_t mat, string_s* file_name, mod_s** out, uint32_t* mod_idx);
int VM_exec_str(matrix_t mat, const char* str, uint32_t size, uint32_t mod_idx);
int VM_add_mod(matrix_t mat, string_s* name, mod_s** out, uint32_t* mod_idx);
int VM_get_mod_idx(matrix_t mat, string_s* name, uint32_t* idx);
int VM_get_mod(matrix_t mat, string_s* name, mod_s** out);
int VM_get_mod_by_idx(matrix_t mat, uint32_t idx, mod_s** out);
int VM_exec_mod(matrix_t mat, mod_s* mod, vec_s* code);
int VM_call(matrix_t mat, mod_s* mod, uint32_t param_num);

#endif

