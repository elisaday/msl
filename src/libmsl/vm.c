/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "vm.h"
#include "vec.h"
#include "str.h"
#include "hash_list.h"
#include "ins.h"
#include "parse.h"
#include "err.h"
#include "list.h"
#include "dict.h"
#include "debug.h"
#include "pool.h"
#include "gc.h"
#include "builtins.h"
#include "hash.h"

#define GET_REF_OBJECT(offset) \
	mod_pos = (int32_t)(*ip++);\
	pos = (int32_t)(*ip++);\
	n = *ip++;\
	if (pos >= 0) {\
		if (mod_pos == -1) {\
			ret = HL_ref_obj(&mod->objs, pos, &ro);\
			CHECK_RESULT(ret);\
		}\
		else {\
			ret = HL_ref_obj(&mod->objs, mod_pos, &ro);\
			CHECK_RESULT(ret);\
			if (ro->type != MAT_OT_MOD) {\
				E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Invalid module object.");\
				return -1;\
			}\
			ret = HL_ref_obj(&ro->mod->objs, pos, &ro);\
			CHECK_RESULT(ret);\
		}\
	}\
	else {\
		if (mod_pos != -1) {\
			E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "The module name is used by a local object.");\
			return -1;\
		}\
		assert(mat->frame_top > 0);\
		idx = (uint32_t)(- (pos + 1));\
		f = frame + mat->frame_top - 1;\
		if (idx < f->func->param_num)\
			ro = stack + f->stack_base - f->param_num + idx;\
		else\
			ro = stack + f->stack_base + 1 + (idx - f->param_num);\
	}\
	if (n > 0) {\
		for (i = 0; i < n; ++i) {\
			o = stack + mat->stack_top - offset - n + i;\
			if (ro->type == MAT_OT_LIST)\
				ret = LI_index_ref(mat, ro->list, o, &ro);\
			else if (ro->type == MAT_OT_DICT)\
				ret = D_index_ref(mat, ro->dict, o, &ro);\
			else {\
				E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "The object cannot be indexed.");\
				return -1;\
			}\
			if (ret == -1) {\
				E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "The index is invalid.");\
				return -1;\
			}\
		}\
	}

#define GET_REF_OBJECT_ASSIGN \
	GET_REF_OBJECT(1)
#define GET_REF_OBJECT_EXPRESSION \
	GET_REF_OBJECT(0)

static load_dll(matrix_t mat, mod_s* mod, string_s* name) {
	int ret;
	char path[MAX_PATH];
	typedef int (*PFN_IMPORT)(matrix_t mat, mat_mod_t mod);
	PFN_IMPORT pfn;

#if defined(PLATFORM_WINDOWS)
	sprintf(path, "%s.dll", name->str);
	mod->handle = LoadLibrary(path);

	if (!mod->handle) {
		E_log("Load dll \"%s\" failed", name->str);
		return -1;
	}

	pfn = (PFN_IMPORT)GetProcAddress(mod->handle, "import");

	if (!pfn) {
		E_log("Cannot find \"import\" in dll \"%s\"", name->str);
		return -1;
	}

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	sprintf(path, "%s.so", name->str);
	mod->handle = dlopen(path, RTLD_NOW);

	if (!mod->handle) {
		E_log("Load dll \"%s\" failed", name->str);
		return -1;
	}

	pfn = (PFN_IMPORT)dlsym(mod->handle, "import");

	if (!pfn) {
		char* err = dlerror();
		E_log("Cannot find \"import\" in dll \"%s\". Reason: %s", name->str, err);
		return -1;
	}

#else
#error "DLL not support."
#endif

	ret = pfn(mat, mod);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

static int judge_mod_type(string_s* name) {
	char path[MAX_PATH];
	FILE* fp;
	sprintf(path, "%s.mat", name->str);
	fp = fopen(path, "r");

	if (fp)
		return 0;

#if defined(PLATFORM_WINDOWS)
	sprintf(path, "%s.dll", name->str);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	sprintf(path, "%s.so", name->str);
#else
#error "DLL not support."
#endif

	fp = fopen(path, "r");

	if (fp)
		return 1;

	return -1;
}

int VM_init(matrix_t mat) {
	int ret;
	uint32_t idx;
	mod_s* builtins;
	string_s* name;

	ret = S_init_mat_str_table(&mat->strs_gc);
	CHECK_RESULT(ret);

	ret = S_init_mat_str_table(&mat->strs_nogc);
	CHECK_RESULT(ret);

	ret = HL_init(&mat->mods, DEFAULT_VM_MOD_NUM);
	CHECK_RESULT(ret);

	ret = V_init(&mat->stack, sizeof(obj_s), DEFAULT_STACK_SIZE);
	CHECK_RESULT(ret);
	V_SIZE(mat->stack) = DEFAULT_STACK_SIZE;
	mat->stack_top = 0;
	mat->stack_base = 0;

	ret = V_init(&mat->call_frames, sizeof(call_frame_s), DEFALT_FUNC_FRAME_SIZE);
	CHECK_RESULT(ret);
	V_SIZE(mat->call_frames) = DEFALT_FUNC_FRAME_SIZE;
	mat->frame_top = 0;

	ret = DBG_init(mat);
	CHECK_RESULT(ret);

	ret = POOL_list_init(&mat->pool_list);
	CHECK_RESULT(ret);

	ret = POOL_dict_init(&mat->pool_dict);
	CHECK_RESULT(ret);

	ret = S_get_str(&mat->strs_nogc, "__builtins__", &name);
	CHECK_RESULT(ret);

	ret = VM_add_mod(mat, name, &builtins, &idx);
	CHECK_RESULT(ret);

	ret = BU_import(mat, builtins);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

void VM_free(matrix_t mat) {
	uint32_t i;

	POOL_dict_free(&mat->pool_dict);
	POOL_list_free(&mat->pool_list);
	DBG_free(mat);
	V_free(&mat->call_frames);
	V_free(&mat->stack);

	for (i = 0; i < HL_SIZE(mat->mods); ++i) {
		obj_s obj;
		mod_s* mod;

		if (HL_get_obj(&mat->mods, i, &obj) != 0) {
			assert(0);
			continue;
		}

		assert(obj.type == MAT_OT_MOD);
		mod = obj.mod;
		MOD_free(mod);
		free(mod);
	}

	HL_free(&mat->mods);
	S_free_mat_str_table(&mat->strs_nogc);
	S_free_mat_str_table(&mat->strs_gc);
}

int VM_exec_file(matrix_t mat, string_s* file_name, mod_s** out, uint32_t* mod_idx) {
	int ret;
	mod_s* mod;
	assert(mat);
	assert(file_name);

	ret = VM_add_mod(mat, file_name, &mod, mod_idx);
	CHECK_RESULT(ret);

	if (ret == 1 && mod->init)
		return 0;

	assert(mod);

	ret = judge_mod_type(file_name);
	CHECK_RESULT(ret);

	if (ret == 0) {
		ret = BU_import(mat, mod);
		CHECK_RESULT(ret);

		ret = P_parse_file(mat, mod);
		CHECK_RESULT(ret);

		ret = DBG_add_mod(mat, mod);
		CHECK_RESULT(ret);

		ret = VM_exec_mod(mat, mod, &mod->code);
		CHECK_RESULT(ret);
	}
	else {
		ret = load_dll(mat, mod, file_name);
		CHECK_RESULT(ret);
	}

	mod->init = 1;
	*out = mod;
	ret = 0;
exit0:
	return ret;
}

int VM_exec_str(matrix_t mat, const char* str, uint32_t size, uint32_t mod_idx) {
	int ret;
	mod_s* mod;
	assert(mat);
	assert(str);

	ret = VM_get_mod_by_idx(mat, mod_idx, &mod);
	CHECK_RESULT(ret);

	V_SIZE(mod->code) = 0;

	ret = P_parse_str(mat, mod, str, size);
	CHECK_RESULT(ret);

	ret = DBG_add_mod(mat, mod);
	CHECK_RESULT(ret);

	ret = VM_exec_mod(mat, mod, &mod->code);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int VM_add_mod(matrix_t mat, string_s* name, mod_s** out, uint32_t* mod_idx) {
	int ret;
	int32_t idx;
	mod_s* mod = NULL;
	obj_s obj;

	assert(mat);
	assert(name);
	assert(out);
	assert(mod_idx);

	idx = HL_get_obj_idx(&mat->mods, name);

	if (idx >= 0) {
		HL_get_obj(&mat->mods, idx, &obj);
		assert(obj.type == MAT_OT_MOD);
		*mod_idx = idx;
		*out = obj.mod;
		return 1;
	}

	mod = malloc(sizeof(mod_s));
	obj.type = MAT_OT_MOD;
	obj.mod = mod;

	ret = MOD_init(mod, name);
	CHECK_RESULT(ret);

	idx = HL_set_obj(&mat->mods, name, &obj);
	CHECK_CONDITION(idx >= 0);

	*out = mod;
	*mod_idx = idx;
	ret = 0;
exit0:

	if (ret != 0 && mod) {
		MOD_free(mod);
		free(mod);
	}

	return ret;
}

int VM_get_mod_idx(matrix_t mat, string_s* name, uint32_t* idx) {
	int32_t n;
	assert(mat);
	assert(name);
	assert(idx);

	n = HL_get_obj_idx(&mat->mods, name);

	if (n < 0)
		return -1;

	*idx = n;
	return 0;
}

int VM_get_mod(matrix_t mat, string_s* name, mod_s** out) {
	int ret;
	obj_s obj;
	assert(mat);
	assert(name);
	assert(out);

	ret = HL_get_obj_direct(&mat->mods, name, &obj);
	CHECK_RESULT(ret);

	assert(obj.type == MAT_OT_MOD);
	*out = obj.mod;
	ret = 0;
exit0:
	return ret;
}

int VM_get_mod_by_idx(matrix_t mat, uint32_t idx, mod_s** out) {
	int ret;
	obj_s obj;
	assert(mat);
	assert(out);

	ret = HL_get_obj(&mat->mods, idx, &obj);
	CHECK_RESULT(ret);

	assert(obj.type == MAT_OT_MOD);
	*out = obj.mod;
	ret = 0;
exit0:
	return ret;
}

static void dump_stack(matrix_t mat) {
	obj_s* stack = &V_AT(mat->stack, 0, obj_s);
	uint32_t top = mat->stack_top;

	if (top == 0) {
		printf("================================\n");
		return;
	}

	while (top--) {
		obj_s* o = stack + top;

		O_dump(o);
	}

	printf("================================\n");
}

int VM_exec_mod(matrix_t mat, mod_s* mod, vec_s* code) {
	int ret;
	uint32_t* ip = (uint32_t*)&V_AT(*code, 0, uint32_t);
	uint32_t* ip_begin = ip;
	uint32_t* ip_end = ip + V_SIZE(*code);
	uint32_t* op;

	obj_s* stack = &V_AT(mat->stack, 0, obj_s);
	obj_s* stack_end = stack + V_SIZE(mat->stack);

	call_frame_s* frame = &V_AT(mat->call_frames, 0, call_frame_s);
	call_frame_s* frame_end = frame + V_SIZE(mat->call_frames);

	obj_s* o;
	obj_s* r;
	obj_s* ro;
	string_s* s;
	string_s* mod_name;
	call_frame_s* f;
	uint32_t idx;
	uint32_t n, i;
	int32_t mod_pos;
	int32_t pos;
	mod_s* m;
	list_s* l;
	dict_s* d;

	uint32_t ret_addr;
	uint32_t stack_base;
	func_s* func;

	for (;;) {
		//dump_stack(mat);
		op = ip;

		//GC_run_once(mat);

		switch (*ip++) {
			case IT_NOP: {
				continue;
			}

			case IT_EXIT: {
				return 0;
			}

			case IT_IMPORT: {
				idx = *ip++;

				ret = S_get_str_by_idx(&mat->strs_nogc, idx, &mod_name);
				CHECK_RESULT(ret);

				if (*op == IT_IMPORT) {
					ret = VM_exec_file(mat, mod_name, &m, &idx);
					CHECK_RESULT(ret);
				}

				continue;
			}

			case IT_PUSH_NONE: {
				stack[mat->stack_top++].type = MAT_OT_NONE;
				continue;
			}

			case IT_PUSH_STRING: {
				o = stack + mat->stack_top++;
				n = *ip++;
				ret = S_get_str_by_idx(&mat->strs_nogc, n, &s);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Get string from string table failed. Index is %d.", n);
					return -1;
				}

				o->type = MAT_OT_STR;
				o->str = s;
				continue;
			}

			case IT_PUSH_INT: {
				o = stack + mat->stack_top++;
				o->type = MAT_OT_INT32;
				o->int32 = *ip++;
				continue;
			}

			case IT_PUSH_REAL: {
				o = stack + mat->stack_top++;
				o->type = MAT_OT_REAL;
				o->real = *(float*)ip++;
				continue;
			}

			case IT_POP: {
				mat->stack_top--;
				continue;
			}

			case IT_ASSIGN: {
				GET_REF_OBJECT_ASSIGN;
				o = stack + mat->stack_top - 1;
				*ro = *o;
				mat->stack_top -= (n + 1);
				continue;
			}

			case IT_ASSIGN_ADD: {
				GET_REF_OBJECT_ASSIGN;
				o = stack + mat->stack_top - 1;
				ret = O_add(mat, ro, o, ro);


				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot add %s and %s.", O_type_to_str(ro->type), O_type_to_str(o->type));
					return -1;
				}

				mat->stack_top -= (n + 1);
				continue;
			}

			case IT_ASSIGN_SUB: {
				GET_REF_OBJECT_ASSIGN;
				o = stack + mat->stack_top - 1;
				ret = O_sub(ro, o, ro);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot sub %s and %s.", O_type_to_str(ro->type), O_type_to_str(o->type));
					return -1;
				}

				mat->stack_top -= (n + 1);
				continue;
			}

			case IT_ASSIGN_MUL: {
				GET_REF_OBJECT_ASSIGN;
				o = stack + mat->stack_top - 1;
				ret = O_mul(mat, ro, o, ro);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot mul %s and %s.", O_type_to_str(ro->type), O_type_to_str(o->type));
					return -1;
				}

				mat->stack_top -= (n + 1);
				continue;
			}

			case IT_ASSIGN_DIV: {
				GET_REF_OBJECT_ASSIGN;
				o = stack + mat->stack_top - 1;
				ret = O_div(ro, o, ro);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot div %s and %s.", O_type_to_str(ro->type), O_type_to_str(o->type));
					return -1;
				}

				mat->stack_top -= (n + 1);
				continue;
			}

			case IT_ASSIGN_MOD: {
				assert(0);
			}

			case IT_ASSIGN_EXP: {
				assert(0);
			}

			case IT_ASSIGN_AND: {
				assert(0);
			}

			case IT_ASSIGN_OR: {
				assert(0);
			}

			case IT_ASSIGN_XOR: {
				assert(0);
			}

			case IT_ASSIGN_SHIFT_LEFT: {
				assert(0);
			}

			case IT_ASSIGN_SHIFT_RIGHT: {
				assert(0);
			}

			case IT_FALSE_JMP: {
				o = stack + --mat->stack_top;

				if (o->type == MAT_OT_NONE)
					ip = ip_begin + *ip;
				else
					ip++;

				continue;
			}

			case IT_TRUE_JMP: {
				o = stack + --mat->stack_top;

				if (o->type != MAT_OT_NONE)
					ip = ip_begin + *ip;
				else
					ip++;

				continue;
			}

			case IT_JMP: {
				ip = ip_begin + *ip;
				continue;
			}

			case IT_MINUS: {
				o = stack + mat->stack_top - 1;

				if (o->type == MAT_OT_INT32)
					o->int32 = -o->int32;
				else if (o->type == MAT_OT_REAL)
					o->real = -o->real;
				else {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot minus %s.", O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_INC: {
				if (o->type == MAT_OT_INT32)
					o->int32 = o->int32++;
				else if (o->type == MAT_OT_REAL)
					o->real = o->real++;
				else {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot inc %s.", O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_DEC: {
				if (o->type == MAT_OT_INT32)
					o->int32 = o->int32--;
				else if (o->type == MAT_OT_REAL)
					o->real = o->real--;
				else {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot dec %s.", O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_LNOT: {
				assert(0);
			}

			case IT_BNOT: {
				assert(0);
			}

			case IT_ADD: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_add(mat, r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot add %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_SUB: {
				o = stack + mat->stack_top - 1;
				r = stack + mat->stack_top - 2;

				ret = O_sub(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot sub %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				mat->stack_top--;
				continue;
			}

			case IT_MUL: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_mul(mat, r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot mul %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_DIV: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_div(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot div %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_MOD: {
				assert(0);
			}

			case IT_EXP: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_exp(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot exp %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_BXOR: {
				assert(0);
			}

			case IT_BOR: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_bitwise_or(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot bitwise or %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_BAND: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_bitwise_and(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot bitwise and %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_SHL: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_shift_left(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot shift left %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_SHR: {
				o = stack + --mat->stack_top;
				r = stack + mat->stack_top - 1;

				ret = O_shift_right(r, o, r);

				if (ret != 0) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot shift right %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				continue;
			}

			case IT_LOR: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				if (r->type != MAT_OT_NONE || o->type != MAT_OT_NONE)
					r->type = MAT_OT_INT32;
				else
					r->type = MAT_OT_NONE;

				mat->stack_top--;
				continue;
			}

			case IT_LAND: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				if (r->type == MAT_OT_NONE || o->type == MAT_OT_NONE)
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_NEQ: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				if (O_compare_eq(r, o))
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_EQ: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				if (!O_compare_eq(r, o))
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_GE: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				ret = O_compare_ge(r, o);

				if (ret == -1) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot compare %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				if (ret == 0)
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_LE: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				ret = O_compare_gt(r, o);

				if (ret == -1) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot compare %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				if (ret == 1)
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_GT: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				ret = O_compare_gt(r, o);

				if (ret == -1) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot compare %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				if (ret == 0)
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_LT: {
				r = stack + mat->stack_top - 2;
				o = stack + mat->stack_top - 1;

				ret = O_compare_ge(r, o);

				if (ret == -1) {
					E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Cannot compare %s and %s.", O_type_to_str(r->type), O_type_to_str(o->type));
					return -1;
				}

				if (ret == 1)
					r->type = MAT_OT_NONE;
				else
					r->type = MAT_OT_INT32;

				mat->stack_top--;
				continue;
			}

			case IT_CALL: {
				assert(mat->stack_top > 0);
				n = *ip;
				ro = stack + mat->stack_top - 1 - n;

				if (ro->type == MAT_OT_FUNC) {
					ret_addr = ip - ip_begin + 1;
					stack_base = mat->stack_top;

					o = stack + mat->stack_top++;

					func = ro->func;
					o->type = MAT_OT_INT32;
					o->int32 = ret_addr;

					f = frame + mat->frame_top++;
					f->func = func;
					f->stack_base = stack_base;
					f->param_num = n;

					n = HL_SIZE(func->objs) - func->param_num;

					while (n-- > 0) {
						obj_s* o = stack + mat->stack_top++;
						o->type = MAT_OT_DUMMY;
					}

					ip_begin = &V_AT(func->code, 0, uint32_t);
					ip = ip_begin;
					ip_end = ip_begin + V_SIZE(func->code);
					continue;
				}
				else if (ro->type == MAT_OT_C_FUNC) {
					matrix_api_t c_func;
					mat->stack_base = mat->stack_top - n;
					c_func = ro->c_func;
					ret = c_func(mat);
					mat->stack_base = 0;

					if (ret != 0) {
						E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Call C function failed.");
						return -1;
					}

					o = stack + mat->stack_top - 1;
					*ro = *o;
					mat->stack_top -= (n + 1);
					ip++;
					continue;
				}
				else if (ro->type == MAT_OT_EXT) {
					mat_ext_header_s* ext = ro->ext;

					if (ext->call) {
						mat->stack_base = mat->stack_top - n;
						ret = ext->call(mat, ext);
						mat->stack_base = 0;

						if (ret != 0) {
							E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Call C function failed.");
							return -1;
						}

						o = stack + mat->stack_top - 1;
						*ro = *o;
						mat->stack_top -= (n + 1);
						ip++;
						continue;
					}
				}

				E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "The object is not callable.");
				return -1;
			}

			case IT_RET: {
				f = frame + --mat->frame_top;
				o = stack + mat->stack_top - (HL_SIZE(f->func->objs) - f->func->param_num) - 1;
				assert(o->type == MAT_OT_INT32);
				ret_addr = o->int32;
				n = HL_SIZE(f->func->objs) - f->func->param_num + f->param_num + 1;
				mat->stack_top -= n;

				o = stack + mat->stack_top - 1;
				o->type = MAT_OT_NONE;

				if (ret_addr == 0) // 表示从C代码调用过来的
					return 0;
				else {
					if (mat->frame_top == 0) {
						ip_begin = &V_AT(mod->code, 0, uint32_t);
						ip = ip_begin + ret_addr;
						ip_end = ip_begin + V_SIZE(mod->code);
					}
					else {
						f = frame + mat->frame_top - 1;
						ip_begin = &V_AT(f->func->code, 0, uint32_t);
						ip = ip_begin + ret_addr;
						ip_end = ip_begin + V_SIZE(f->func->code);
					}
				}

				continue;
			}

			case IT_RET_RESULT: {
				r = stack + mat->stack_top - 1;
				f = frame + --mat->frame_top;
				o = stack + mat->stack_top - (HL_SIZE(f->func->objs) - f->func->param_num) - 2;
				assert(o->type == MAT_OT_INT32);
				ret_addr = o->int32;
				n = HL_SIZE(f->func->objs) - f->func->param_num + f->param_num + 2;
				mat->stack_top -= n;

				*(stack + mat->stack_top - 1) = *r;

				if (ret_addr == 0)
					return 0;
				else {
					if (mat->frame_top == 0) {
						ip_begin = &V_AT(mod->code, 0, uint32_t);
						ip = ip_begin + ret_addr;
						ip_end = ip_begin + V_SIZE(mod->code);
					}
					else {
						f = frame + mat->frame_top - 1;
						ip_begin = &V_AT(f->func->code, 0, uint32_t);
						ip = ip_begin + ret_addr;
						ip_end = ip_begin + V_SIZE(f->func->code);
					}
				}

				continue;
			}

			case IT_MAKE_LIST: {
				n = *ip++;
				ret = LI_alloc(mat, n, &l);
				CHECK_RESULT(ret);
				assert(V_CAP(l->v) >= n);

				if (n > 0) {
					o = &V_AT(l->v, 0, obj_s);
					memcpy(o, stack + mat->stack_top - n, sizeof(obj_s) * n);
					mat->stack_top -= n;
					o = stack + mat->stack_top++;
					o->type = MAT_OT_LIST;
					o->list = l;
					V_SIZE(l->v) = n;
				}

				continue;
			}

			case IT_MAKE_DICT: {
				n = *ip++;
				i = n;
				ret = D_alloc(mat, n, &d);
				CHECK_RESULT(ret);

				while (n) {
					o = stack + mat->stack_top - n * 2;
					ro = o + 1;
					ret = H_set(&d->h, o, ro);

					if (ret == -1) {
						E_rt_err(mat, mod->name->str, DBG_get_line(mat, op), "Make dict failed. %s %s", O_type_to_str(o->type), O_type_to_str(ro->type));
						return -1;
					}

					n--;
				}

				mat->stack_top -= i * 2;
				o = stack + mat->stack_top++;
				o->type = MAT_OT_DICT;
				o->dict = d;
				continue;
			}

			case IT_REF_OBJ: {
				GET_REF_OBJECT_EXPRESSION;
				o = stack + mat->stack_top;
				o->type = MAT_OT_OBJ_REF;
				o->obj_ref = ro;
				continue;
			}

			case IT_PUSH_OBJ: {
				GET_REF_OBJECT_EXPRESSION;
				mat->stack_top -= n;
				o = stack + mat->stack_top++;
				*o = *ro;
				continue;
			}

			default:
				assert(0);
				return -1;
		}
	}

	ret = 0;
exit0:
	return ret;
}

int VM_call(matrix_t mat, mod_s* mod, uint32_t param_num) {
	int ret;
	int n = param_num;
	obj_s* ro;
	obj_s* o;
	obj_s* stack;
	uint32_t stack_base;
	func_s* func;
	call_frame_s* f;
	call_frame_s* frame = &V_AT(mat->call_frames, 0, call_frame_s);

	assert(mat);
	assert(mat->stack_top > 0);

	mat->stack_top = 0;
	mat->frame_top = 0;

	stack = &V_AT(mat->stack, 0, obj_s);
	ro = stack + mat->stack_top - 1 - n;

	if (ro->type != MAT_OT_FUNC) {
		E_rt_err(mat, "from C code", 0, "The object is not callable.");
		return -1;
	}

	stack_base = mat->stack_top;
	o = stack + mat->stack_top++;

	func = ro->func;
	o->type = MAT_OT_INT32;
	o->int32 = 0; // 0表示是从C代码里调用过来的

	f = frame + mat->frame_top++;
	f->func = func;
	f->stack_base = stack_base;
	f->param_num = n;

	n = HL_SIZE(func->objs) - func->param_num;

	while (n-- > 0) {
		obj_s* o = stack + mat->stack_top++;
		o->type = MAT_OT_DUMMY;
	}

	ret = VM_exec_mod(mat, mod, &func->code);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}
