/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "matrix.h"
#include "vm.h"
#include "err.h"
#include "str.h"
#include "hash.h"
#include "vec.h"
#include "ins.h"
#include "parse.h"
#include "debug.h"
#include "hash_list.h"
#include "list.h"
#include "builtins.h"

/* method */
int MAT_init(matrix_t* mat) {
	matrix_t m;
	assert(mat);

	m = malloc(sizeof(matrix_s));
	CHECK_MALLOC(m);

	if (VM_init(m) != 0)
		return -1;

	*mat = m;
	return 0;
}

int MAT_free(matrix_t mat) {
	VM_free(mat);
	free(mat);
	P_clear();
	return 0;
}

int MAT_exec_file(matrix_t mat, const char* file_name, uint32_t* mod_idx) {
	int ret;
	string_s* mod_name;
	mod_s* mod;
	assert(mat);
	assert(file_name);

	ret = S_get_str(&mat->strs_nogc, file_name, &mod_name);
	CHECK_RESULT(ret);

	ret = VM_exec_file(mat, mod_name, &mod, mod_idx);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_add_mod(matrix_t mat, const char* mod_name, uint32_t* mod_idx) {
	int ret;
	string_s* name;
	mod_s* mod;
	assert(mat);
	assert(mod_name);
	assert(mod_idx);

	ret = S_get_str(&mat->strs_nogc, mod_name, &name);
	CHECK_RESULT(ret);

	ret = VM_add_mod(mat, name, &mod, mod_idx);
	CHECK_RESULT(ret);

	ret = BU_import(mat, mod);
	CHECK_RESULT(ret);

	mod->init = 1;
	ret = 0;
exit0:
	return ret;
}

int MAT_exec_str(matrix_t mat, const char* str, uint32_t size, uint32_t mod_idx) {
	int ret;
	assert(mat);
	assert(str);

	ret = VM_exec_str(mat, str, size, mod_idx);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_disasm(matrix_t mat, const char* mod_name, MAT_disasm_callback cb) {
	int ret;
	string_s* name;
	uint32_t mod_idx;
	mod_s* mod;
	assert(mat);
	assert(mod_name);

	ret = S_create_str(&mat->strs_gc, mod_name, &name);
	CHECK_RESULT(ret);

	ret = VM_add_mod(mat, name, &mod, &mod_idx);
	CHECK_RESULT(ret);

	if (ret == 0) {
		ret = BU_import(mat, mod);
		CHECK_RESULT(ret);

		ret = P_parse_file(mat, mod);
		CHECK_RESULT(ret);
	}

	ret = INS_disasm(mod, &mat->strs_nogc, cb);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_get_mod_obj_idx(matrix_t mat, uint32_t mod_idx, const char* name, uint32_t* idx) {
	int ret;
	mod_s* mod;
	string_s* obj_name;
	assert(mat);
	assert(name);
	assert(idx);

	ret = VM_get_mod_by_idx(mat, mod_idx, &mod);
	CHECK_RESULT(ret);

	ret = S_create_str(&mat->strs_gc, name, &obj_name);
	CHECK_RESULT(ret);

	ret = MOD_get_obj_idx(mod, obj_name, idx);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_push_mod_obj(matrix_t mat, uint32_t mod_idx, uint32_t obj_idx) {
	int ret;
	obj_s* o;
	mod_s* mod;

	ret = VM_get_mod_by_idx(mat, mod_idx, &mod);
	CHECK_RESULT(ret);

	o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	ret = MOD_get_obj_by_idx(mod, obj_idx, o);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_push_none(matrix_t mat) {
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	o->type = MAT_OT_NONE;
	return 0;
}

int MAT_push_int(matrix_t mat, int i) {
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	o->type = MAT_OT_INT32;
	o->int32 = (int32_t)i;
	return 0;
}

int MAT_push_float(matrix_t mat, float f) {
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	o->type = MAT_OT_REAL;
	o->real = (real_t)f;
	return 0;
}

int MAT_push_str(matrix_t mat, const char* s) {
	int ret;
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top;
	o->type = MAT_OT_STR;
	ret = S_create_str(&mat->strs_gc, s, &o->str);
	CHECK_RESULT(ret);
	mat->stack_top++;
	ret = 0;
exit0:
	return ret;
}

int MAT_push_str_obj(matrix_t mat, mat_str_t s) {
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	o->type = MAT_OT_STR;
	o->str = s;
	return 0;
}

int MAT_push_ext(matrix_t mat, mat_ext_header_s* ext) {
	obj_s* o = &V_AT(mat->stack, 0, obj_s) + mat->stack_top++;
	o->type = MAT_OT_EXT;
	o->ext = ext;
	return 0;
}

int MAT_call(matrix_t mat, uint32_t mod_idx, uint32_t param_num) {
	int ret;
	mod_s* mod;

	assert(mat);

	ret = VM_get_mod_by_idx(mat, mod_idx, &mod);
	CHECK_RESULT(ret);

	ret = VM_call(mat, mod, param_num);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int MAT_pop(matrix_t mat, uint32_t n) {
	if (mat->stack_top >= n) {
		mat->stack_top -= n;
		return 0;
	}

	return -1;
}

mat_obj_t MAT_get_stack_obj(matrix_t mat, int pos) {
	uint32_t idx;

	if (pos > 0)
		idx = mat->stack_base + pos - 1;
	else if (mat->stack_top >= (uint32_t)(-pos))
		idx = mat->stack_top - (uint32_t)(-pos);

	return &V_AT(mat->stack, 0, obj_s) + idx;
}

int MAT_get_param_num(matrix_t mat) {
	return mat->stack_top - mat->stack_base;
}

mat_obj_type_e MAT_obj_type(mat_obj_t o) {
	return o->type;
}

const char* MAT_to_str(matrix_t mat, mat_obj_t o) {
	string_s* str;

	if (O_to_str(mat, o, &str) != 0)
		return NULL;

	assert(str);
	return str->str;
}

int MAT_to_str_obj(matrix_t mat, mat_obj_t o, mat_str_t* s) {
	int ret;
	ret = O_to_str(mat, o, s);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_to_int32(matrix_t mat, mat_obj_t o, int32_t* i) {
	int ret;
	ret = O_to_int32(o, i);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_to_float(matrix_t mat, mat_obj_t o, float* f) {
	int ret;
	ret = O_to_real(o, f);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

void* MAT_to_addr(matrix_t mat, mat_obj_t o) {
	switch (o->type) {
		case MAT_OT_INT32:
			return &o->int32;

		case MAT_OT_REAL:
			return &o->real;

		case MAT_OT_STR:
			return &o->str->str;

		default:
			return NULL;
	}
}

int MAT_print(mat_obj_t o) {
	return O_print(o, 0);
}

int MAT_reg_func(matrix_t mat, mat_mod_t mod, const char* name, matrix_api_t func) {
	int ret;
	obj_s o;
	string_s* str;

	ret = S_get_str(&mat->strs_nogc, name, &str);
	CHECK_RESULT(ret);
	o.type = MAT_OT_C_FUNC;
	o.c_func = func;
	ret = HL_set_obj(&mod->objs, str, &o);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_reg_int(matrix_t mat, mat_mod_t mod, const char* name, int n) {
	int ret;
	obj_s o;
	string_s* str;

	ret = S_get_str(&mat->strs_nogc, name, &str);
	CHECK_RESULT(ret);
	o.type = MAT_OT_INT32;
	o.int32 = n;
	ret = HL_set_obj(&mod->objs, str, &o);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_reg_float(matrix_t mat, mat_mod_t mod, const char* name, float f) {
	int ret;
	obj_s o;
	string_s* str;

	ret = S_get_str(&mat->strs_nogc, name, &str);
	CHECK_RESULT(ret);

	o.type = MAT_OT_REAL;
	o.real = f;
	ret = HL_set_obj(&mod->objs, str, &o);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_reg_str(matrix_t mat, mat_mod_t mod, const char* name, const char* s) {
	int ret;
	obj_s o;
	string_s* str;

	ret = S_get_str(&mat->strs_nogc, name, &str);
	CHECK_RESULT(ret);

	o.type = MAT_OT_STR;
	ret = S_get_str(&mat->strs_nogc, s, &o.str);
	CHECK_RESULT(ret);
	ret = HL_set_obj(&mod->objs, str, &o);
	CHECK_RESULT(ret);
	ret = 0;
exit0:
	return ret;
}

int MAT_list_len(matrix_t mat, mat_obj_t list, uint32_t* len) {
	if (list->type != MAT_OT_LIST)
		return -1;

	*len = V_SIZE(list->list->v);
	return 0;
}

int MAT_list_index(matrix_t mat, mat_obj_t list, uint32_t idx, mat_obj_t* out) {
	if (list->type != MAT_OT_LIST)
		return -1;

	if (idx >= V_SIZE(list->list->v))
		return -1;

	*out = &V_AT(list->list->v, idx, obj_s);
	return 0;
}
