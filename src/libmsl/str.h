/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#ifndef __STRING_H__
#define __STRING_H__

#include "obj.h"

// 创建和销毁字符串表
int S_init_mat_str_table(mat_str_table_s* table);
int S_free_mat_str_table(mat_str_table_s* table);

// 创建字符串
int S_create_str(mat_str_table_s* table, const char* s, string_s** out);
int S_create_real(mat_str_table_s* table, real_t r, string_s** out);
int S_create_int(mat_str_table_s* table, int32_t i, string_s** out);
int S_conc_str(mat_str_table_s* table, const char* s1, const char* s2, string_s** out);

// 根据给定的C风格字符串，查找内容相同的string_s对象
int S_find_str(mat_str_table_s* table, const char* s, string_s** out);

// 根据给定的C风格字符串，查找内容相同的string_s对象。如果不存在，那么创建一个新的
int S_get_str(mat_str_table_s* table, const char* s, string_s** out);

// 获取字符串在表中的索引
int S_get_str_idx(mat_str_table_s* table, string_s* s, uint32_t* idx);

// 根据索引获取字符串
int S_get_str_by_idx(mat_str_table_s* table, uint32_t idx, string_s** out);

// 获取字符串的哈希值
uint32_t S_hash(string_s* s);

// 比较两个字符串
int S_compare_eq(string_s* s1, string_s* s2);

int S_sweep(mat_str_table_s* table);

#define S_CSTR(s) ((s)->str)

#endif

