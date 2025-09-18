/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    create: 2006-10-27 23:56
    Copyright (c) 2006-2013 Zeb.  All rights reserved.
*/

#ifndef __H_MATRIX__
#define __H_MATRIX__

#define MSL_VER "0.2.0"

#ifdef _WIN32
    #ifdef libmsl_EXPORTS
        #define MATRIX_API __declspec(dllexport)
    #else
        #define MATRIX_API __declspec(dllimport)
    #endif

    #define MATRIX_API_EXPORT __declspec(dllexport)
#else
    #define MATRIX_API
    #define MATRIX_API_EXPORT
#endif

#ifdef _WIN32
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

// 对象类型定义
typedef enum {
    MAT_OT_DUMMY = 0, // 必须为0
    MAT_OT_NONE,
    MAT_OT_INT32,
    MAT_OT_REAL,
    MAT_OT_STR,
    MAT_OT_FUNC,
    MAT_OT_C_FUNC,
    MAT_OT_MOD,
    MAT_OT_OBJ_REF,
    MAT_OT_LIST,
    MAT_OT_DICT,
    MAT_OT_EXT, // 自定义类型

    MAT_OT_MAX,
} mat_obj_type_e;

typedef struct matrix_s* matrix_t;
typedef struct obj_s* mat_obj_t;
typedef struct mod_s* mat_mod_t;
typedef struct string_s* mat_str_t;
typedef struct list_s* mat_list_t;

// C扩展接口类型
typedef int (*matrix_api_t)(matrix_t);

// 自定义类型扩展
typedef struct mat_ext_header_s mat_ext_header_s;
typedef int (*mat_ext_callable)(matrix_t, mat_ext_header_s*);
struct mat_ext_header_s {
	mat_ext_callable call;
};

// 初始化
MATRIX_API int MAT_init(matrix_t* mat);

// 销毁
MATRIX_API int MAT_free(matrix_t mat);

// 直接运行一个代码文件
MATRIX_API int MAT_exec_file(matrix_t mat, const char* file_name, uint32_t* mod_idx);
MATRIX_API int MAT_add_mod(matrix_t mat, const char* mod_name, uint32_t* mod_idx);
MATRIX_API int MAT_exec_str(matrix_t mat, const char* str, uint32_t size, uint32_t mod_idx);

// 反汇编一个指定模块
typedef void (*MAT_disasm_callback)(char* s);
MATRIX_API int MAT_disasm(matrix_t mat, const char* mod_name, MAT_disasm_callback cb);

// 获取模块内某对象的索引
MATRIX_API int MAT_get_mod_obj_idx(matrix_t mat, uint32_t mod_idx, const char* name, uint32_t* idx);

// 往运行栈里压入对象
MATRIX_API int MAT_push_mod_obj(matrix_t mat, uint32_t mod_idx, uint32_t obj_idx);
MATRIX_API int MAT_push_none(matrix_t mat);
MATRIX_API int MAT_push_int(matrix_t mat, int i);
MATRIX_API int MAT_push_float(matrix_t mat, float f);
MATRIX_API int MAT_push_str(matrix_t mat, const char* s);
MATRIX_API int MAT_push_str_obj(matrix_t mat, mat_str_t s);
MATRIX_API int MAT_push_ext(matrix_t mat, mat_ext_header_s* ext);

// 调用一个函数
MATRIX_API int MAT_call(matrix_t mat, uint32_t mod_idx, uint32_t param_num);

// 从运行栈里弹出对象
MATRIX_API int MAT_pop(matrix_t mat, uint32_t n);

// 从运行栈里获取一个对象
MATRIX_API mat_obj_t MAT_get_stack_obj(matrix_t mat, int pos);

// 获取参数个数
MATRIX_API int MAT_get_param_num(matrix_t mat);

// 获取对象的类型
MATRIX_API mat_obj_type_e MAT_obj_type(mat_obj_t o);

// 转换对象到指定类型
MATRIX_API const char* MAT_to_str(matrix_t mat, mat_obj_t o);
MATRIX_API int MAT_to_str_obj(matrix_t mat, mat_obj_t o, mat_str_t* s);
MATRIX_API int MAT_to_int32(matrix_t mat, mat_obj_t o, int32_t* i);
MATRIX_API int MAT_to_float(matrix_t mat, mat_obj_t o, float* f);
MATRIX_API void* MAT_to_addr(matrix_t mat, mat_obj_t o);

// 打印一个对象
MATRIX_API int MAT_print(mat_obj_t o);

// 注册接口
MATRIX_API int MAT_reg_func(matrix_t mat, mat_mod_t mod, const char* name, matrix_api_t func);
MATRIX_API int MAT_reg_int(matrix_t mat, mat_mod_t mod, const char* name, int n);
MATRIX_API int MAT_reg_float(matrix_t mat, mat_mod_t mod, const char* name, float f);
MATRIX_API int MAT_reg_str(matrix_t mat, mat_mod_t mod, const char* name, const char* s);

// 数组操作接口
MATRIX_API int MAT_list_len(matrix_t mat, mat_obj_t list, uint32_t* len);
MATRIX_API int MAT_list_index(matrix_t mat, mat_obj_t list, uint32_t idx, mat_obj_t* out);

#endif

