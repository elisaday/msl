/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    create: 2013-7-2 22:16
    Copyright (c) 2006-2013 Zeb.  All rights reserved.
*/

#include <stdio.h>
#include <memory.h>
#include <assert.h>

#if defined(linux) || defined(__linux) || defined(__linux__)
#   define PLATFORM_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#   define PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define PLATFORM_MACOS
#endif

/*
    LP64   unix64
    ILP64
    LLP64  win64
    ILP32  win32
    LP32
*/
#if defined(PLATFORM_WINDOWS)
#   if defined(WIN32)
#       define MODEL_ILP32
#   else
#       define MODEL_LLP64
#   endif
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#endif

#if defined(PLATFORM_WINDOWS)
#   include <conio.h>
#   include <windows.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#   include <dlfcn.h>
#else
#   error "unknown platform."
#endif

#include "matrix.h"
#include "ffi.h"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#   define MAX_PATH PATH_MAX
#endif

#define MAX_DLL_NUM 128
#define MAX_FUNC_NUM 128
#define MAX_ARGS 64

#define CHECK_RESULT(ret)\
	do {\
		if ((ret) != 0) {\
			goto exit0;\
		}\
	} while (0);

#define CHECK_CONDITION(cond)\
	do {\
		if (!(cond)) {\
			ret = -1;\
			goto exit0;\
		}\
	} while (0);

enum type_e {
    c_void = 0,
    c_uint8,
    c_sint8,
    c_uint16,
    c_sint16,
    c_uint32,
    c_sint32,
    c_uint64,
    c_sint64,
    c_float,
    c_double,
    c_pointer,

    c_long,
    c_ulong,
};

typedef void (*PFN_FFI)(void);
typedef struct func_s {
	mat_ext_header_s header;

	ffi_cif cif;
	ffi_type* rtype;
	ffi_type* args[MAX_ARGS];
	void* values[MAX_ARGS];
	ffi_abi abi;
	PFN_FFI func;
	enum type_e rt;
} func_s;

typedef struct dll_s {
	char name[MAX_PATH];

#if defined(PLATFORM_WINDOWS)
	HMODULE handle;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	void* handle;
#endif

	func_s func[MAX_FUNC_NUM];
	int func_num;
} dll_s;

struct ctypes_s {
	dll_s dlls[MAX_DLL_NUM];
	int dll_num;
};

static struct ctypes_s C;

static int dll_close(dll_s* dll) {
	if (dll->handle) {
#if defined(PLATFORM_WINDOWS)
		FreeLibrary(dll->handle);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
		dlclose(dll->handle);
#endif
	}

	return 0;
}

static int c_dll(matrix_t mat) {
	int ret;
	const char* path;
	int idx;
	dll_s* dll;

	mat_obj_t o_path = MAT_get_stack_obj(mat, 1);
	path = MAT_to_str(mat, o_path);
	CHECK_CONDITION(path);

	CHECK_CONDITION(C.dll_num < MAX_DLL_NUM);
	idx = C.dll_num;
	dll = &C.dlls[idx];

#if defined(PLATFORM_WINDOWS)
	_snprintf(dll->name, sizeof(dll->name), "%s.dll", path);
	dll->handle = LoadLibrary(dll->name);
	CHECK_CONDITION(dll->handle);

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	snprintf(dll->name, sizeof(dll->name), "%s.so", path);
	dll->handle = dlopen(dll->name, RTLD_NOW);
	CHECK_CONDITION(dll->handle);
#endif

	C.dll_num++;
	MAT_push_int(mat, idx);
	ret = 0;
exit0:
	return ret;
}

static ffi_type* get_ffi_type(matrix_t mat, mat_obj_t o_ret) {
	int ret;
	int32_t t;

	ret = MAT_to_int32(mat, o_ret, &t);
	CHECK_RESULT(ret);

	switch (t) {
		case c_void:
			return &ffi_type_void;

		case c_uint8:
			return &ffi_type_uint8;

		case c_sint8:
			return &ffi_type_sint8;

		case c_uint16:
			return &ffi_type_uint16;

		case c_sint16:
			return &ffi_type_sint16;

		case c_uint32:
			return &ffi_type_uint32;

		case c_sint32:
			return &ffi_type_sint32;

		case c_uint64:
			return &ffi_type_uint64;

		case c_sint64:
			return &ffi_type_sint64;

		case c_float:
			return &ffi_type_float;

		case c_double:
			return &ffi_type_double;

		case c_pointer:
			return &ffi_type_pointer;

		case c_long:
#if defined(MODEL_ILP32)
			return &ffi_type_sint32;
#elif defined(MODEL_ILP64)
			return &ffi_type_sint64;
#endif

		case c_ulong:
#if defined(MODEL_ILP32)
			return &ffi_type_uint32;
#elif defined(MODEL_ILP64)
			return &ffi_type_uint64;
#endif

		default:
			return NULL;
	}

exit0:
	return NULL;
}

static int push_result(matrix_t mat, func_s* func, ffi_arg ret) {
	switch (func->rt) {
		case c_void:
		case c_uint8:
		case c_sint8:
		case c_uint16:
		case c_sint16:
		case c_uint32:
		case c_sint32:
			MAT_push_int(mat, *(int*)&ret);
			return 0;

		case c_uint64:
		case c_sint64:
			assert(0);
			return -1;

		case c_float:
			MAT_push_float(mat, *(float*)&ret);
			return 0;

		case c_double:
			assert(0);
			return -1;

		case c_pointer:
#if defined(MODEL_ILP32)
			MAT_push_int(mat, *(int*)&ret);
			return 0;
#elif defined(MODEL_ILP64)
			assert(0);
			return -1;
#endif

		default:
			MAT_push_none(mat);
			return -1;
	}

	assert(0);
	return -1;
}

static int call_func(matrix_t mat, mat_ext_header_s* header) {
	func_s* func = (func_s*)header;
	ffi_arg ret_obj;

	const char* strs[MAX_ARGS];
	int str_idx = 0;
	int i;
	int param_num = MAT_get_param_num(mat);

	for (i = 0; i < param_num; ++i) {
		mat_obj_t o = MAT_get_stack_obj(mat, i + 1);
		func->values[i] = MAT_to_addr(mat, o);

		if (MAT_obj_type(o) == MAT_OT_STR) {
			strs[str_idx] = (const char*)func->values[i];
			func->values[i] = (void*)&strs[str_idx];
		}
	}

	ffi_call(&func->cif, func->func, &ret_obj, func->values);
	push_result(mat, func, ret_obj);
	return 0;
}

static int make_func(matrix_t mat, func_s* func, ffi_abi abi, mat_obj_t o_ret, mat_obj_t o_param) {
	int ret;
	unsigned int nargs;
	ffi_status ffi_ret;
	unsigned int i;

	ret = MAT_list_len(mat, o_param, (uint32_t*)&nargs);
	CHECK_RESULT(ret);

	func->rtype = get_ffi_type(mat, o_ret);
	CHECK_CONDITION(func->rtype);

	for (i = 0; i < nargs; ++i) {
		mat_obj_t o;
		ret = MAT_list_index(mat, o_param, i, &o);
		CHECK_RESULT(ret);

		func->args[i] = get_ffi_type(mat, o);
		CHECK_CONDITION(func->args[i]);
	}

	ffi_ret = ffi_prep_cif(&func->cif, abi, nargs, func->rtype, func->args);
	CHECK_CONDITION(ffi_ret == FFI_OK);

	MAT_to_int32(mat, o_ret, (int32_t*)&func->rt);
	func->header.call = call_func;
	ret = 0;
exit0:
	return ret;
}

static int c_func(matrix_t mat) {
	int ret;
	int dll_idx;
	int func_idx;
	const char* func_name;
	dll_s* dll;
	func_s* func;
	PFN_FFI pfn;
	ffi_abi abi;

	mat_obj_t o_dll_idx = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_func_name = MAT_get_stack_obj(mat, 2);
	mat_obj_t o_abi = MAT_get_stack_obj(mat, 3);
	mat_obj_t o_ret = MAT_get_stack_obj(mat, 4);
	mat_obj_t o_param = MAT_get_stack_obj(mat, 5);

	ret = MAT_to_int32(mat, o_dll_idx, &dll_idx);
	CHECK_RESULT(ret);

	func_name = MAT_to_str(mat, o_func_name);
	CHECK_CONDITION(func_name);

	dll = &C.dlls[dll_idx];
	CHECK_CONDITION(dll->func_num < MAX_FUNC_NUM);
	func_idx = dll->func_num++;
	func = &dll->func[func_idx];

#if defined(PLATFORM_WINDOWS)
	pfn = (PFN_FFI)GetProcAddress(dll->handle, func_name);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	pfn = (PFN_FFI)dlsym(dll->handle, func_name);
#endif

	CHECK_CONDITION(pfn);

	ret = MAT_to_int32(mat, o_abi, (int32_t*)&abi);
	CHECK_RESULT(ret);

	ret = make_func(mat, func, abi, o_ret, o_param);
	CHECK_RESULT(ret);

	func->func = pfn;
	MAT_push_ext(mat, (mat_ext_header_s*)func);
	ret = 0;
exit0:
	return ret;
}

MATRIX_API_EXPORT int import(matrix_t mat, mat_mod_t mod) {
	memset(&C, 0, sizeof(C));
	MAT_reg_func(mat, mod, "dll", c_dll);
	MAT_reg_func(mat, mod, "func", c_func);

	MAT_reg_int(mat, mod, "void", c_void);
	MAT_reg_int(mat, mod, "uint8", c_uint8);
	MAT_reg_int(mat, mod, "sint8", c_sint8);
	MAT_reg_int(mat, mod, "uint16", c_uint16);
	MAT_reg_int(mat, mod, "sint16", c_sint16);
	MAT_reg_int(mat, mod, "uint32", c_uint32);
	MAT_reg_int(mat, mod, "sint32", c_sint32);
	MAT_reg_int(mat, mod, "uint64", c_uint64);
	MAT_reg_int(mat, mod, "sint64", c_sint64);
	MAT_reg_int(mat, mod, "float", c_float);
	MAT_reg_int(mat, mod, "double", c_double);
	MAT_reg_int(mat, mod, "pointer", c_pointer);

	MAT_reg_int(mat, mod, "long", c_long);
	MAT_reg_int(mat, mod, "ulong", c_ulong);

	MAT_reg_int(mat, mod, "CDECL", FFI_MS_CDECL);
	MAT_reg_int(mat, mod, "STDCALL", FFI_STDCALL);
	return 0;
}

MATRIX_API_EXPORT int close(mat_mod_t mod) {
	int i;

	for (i = 0; i < C.dll_num; ++i) {
		dll_s* dll = &C.dlls[i];
		dll_close(dll);
	}

	return 0;
}

