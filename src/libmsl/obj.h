/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_OBJECT__
#define __H_OBJECT__

#include "header.h"
#include "config.h"
#include "matrix.h"

typedef struct string_s {
	uint8_t gc_mark;
	uint32_t size;
	uint32_t hash;
	char str[0];
} string_s;

typedef struct obj_s {
	mat_obj_type_e type;
	union {
		int32_t int32;
		real_t real;
		struct string_s* str;
		struct func_s* func;
		struct obj_s* obj_ref;
		struct mod_s* mod;
		matrix_api_t c_func;
		struct list_s* list;
		struct dict_s* dict;
		mat_ext_header_s* ext;
	};
} obj_s;

typedef struct vec_s {
	void* p;
	uint32_t item_size;
	uint32_t size;
	uint32_t cap;
} vec_s;

typedef struct mat_str_table_s {
	vec_s strs; // string_s
	vec_s free_idx; // uint32_t
} mat_str_table_s;

typedef struct hash_node_s {
	uint32_t hash;
	obj_s key;
	obj_s val;
} hash_node_s;

typedef struct hash_s {
	uint32_t used;
	uint32_t mask;
	vec_s nodes; // hash_node_s
} hash_s;

typedef struct hash_list_s {
	vec_s obj; // obj_s
	hash_s hash_obj; // map name to object index
} hash_list_s;

typedef struct op_line_s {
	uint32_t op_pos;
	uint32_t line;
} op_line_s;

typedef struct func_s {
	string_s* name;
	uint32_t param_num; // 函数定义的参数个数
	vec_s code; // uint32_t
	vec_s op_line; // op_line_s
	hash_list_s objs;
} func_s;

typedef struct list_s {
	uint8_t gc_mark;
	vec_s v; // obj_s
	struct list_s* next;
} list_s;

typedef struct pool_list_s {
	list_s* free[3];
	uint32_t size[3];
	list_s* used;
} pool_list_s;

typedef struct dict_s {
	uint8_t gc_mark;
	hash_s h;
	struct dict_s* next;
} dict_s;

typedef struct pool_dict_s {
	dict_s* free[3];
	uint32_t size[3];
	dict_s* used;
} pool_dict_s;

typedef struct call_frame_s {
	func_s* func;
	uint32_t stack_base;
	uint32_t param_num; // 实际传入参数个数
} call_frame_s;

typedef struct mod_s {
	string_s* name;
#if defined(PLATFORM_WINDOWS)
	HMODULE handle;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	void* handle;
#endif
	int init; // 是否初始化过
	vec_s code; // uint32_t
	vec_s op_line; // op_line_s
	vec_s funcs; // func_s*
	hash_list_s objs;
} mod_s;

typedef struct dbg_info_s {
	mod_s* mod;
	func_s* func;
	uint32_t* ip_begin;
	uint32_t* ip_end;
} dbg_info_s;

typedef struct matrix_s {
	mat_str_table_s strs_gc;
	mat_str_table_s strs_nogc;
	hash_list_s mods; // mod_s
	vec_s stack; // obj_s
	uint32_t stack_top;
	uint32_t stack_base;
	vec_s call_frames; // call_frame_s
	uint32_t frame_top;
	vec_s dbg_info;
	pool_list_s pool_list;
	pool_dict_s pool_dict;
} matrix_s;

int O_compare_eq(obj_s* o1, obj_s* o2);
int O_compare_gt(obj_s* o1, obj_s* o2);
int O_compare_ge(obj_s* o1, obj_s* o2);
int O_dump(obj_s* obj);
int O_print(obj_s* obj, int quotes);

const char* O_type_to_str(mat_obj_type_e type);

int O_add(matrix_t mat, obj_s* o1, obj_s* o2, obj_s* r);
int O_sub(obj_s* o1, obj_s* o2, obj_s* r);
int O_mul(matrix_t mat, obj_s* o1, obj_s* o2, obj_s* r);
int O_div(obj_s* o1, obj_s* o2, obj_s* r);
int O_exp(obj_s* o1, obj_s* o2, obj_s* r);
int O_shift_left(obj_s* o1, obj_s* o2, obj_s* r);
int O_shift_right(obj_s* o1, obj_s* o2, obj_s* r);
int O_bitwise_or(obj_s* o1, obj_s* o2, obj_s* r);
int O_bitwise_and(obj_s* o1, obj_s* o2, obj_s* r);

int O_to_str(matrix_t mat, obj_s* o, string_s** s);
int O_to_real(obj_s* o, real_t* r);
int O_to_int32(obj_s* o, int32_t* r);

#endif

