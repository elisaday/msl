/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "str.h"
#include "vec.h"
#include "err.h"

static void init_string(string_s* str) {
	str->gc_mark = 0;
	str->size = 0;
	str->hash = 0;
}

/* method */
int S_init_mat_str_table(mat_str_table_s* table) {
	int ret;
	assert(table);

	ret = V_init(&table->strs, sizeof(string_s*), DEFAULT_STR_TABLE_SIZE);
	CHECK_RESULT(ret);

	ret = V_init(&table->free_idx, sizeof(uint32_t), DEFAULT_STR_TABLE_SIZE);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

int S_free_mat_str_table(mat_str_table_s* table) {
	uint32_t i;
	assert(table);

	for (i = 0; i < V_SIZE(table->strs); ++i)
		free(V_AT(table->strs, i, string_s*));

	V_free(&table->strs);
	V_free(&table->free_idx);
	return 0;
}

static int add_to_table(mat_str_table_s* table, string_s* s) {
	if (V_SIZE(table->free_idx) > 0) {
		uint32_t idx = V_BACK(table->free_idx, uint32_t);
		V_SIZE(table->free_idx)--;
		V_AT(table->strs, idx, string_s*) = s;
		return 0;
	}

	V_PUSH_BACK(table->strs, s, string_s*);
	return 0;
}

int S_create_str(mat_str_table_s* table, const char* s, string_s** out) {
	uint32_t size;
	string_s* str;
	assert(table);
	assert(out);

	size = (uint32_t)strlen(s);
	str = malloc(sizeof(string_s) + size + 1);
	CHECK_MALLOC(str);

	strcpy(str->str, s);
	str->size = size;
	str->hash = 0;
	add_to_table(table, str);
	*out = str;
	return 0;
}

int S_create_real(mat_str_table_s* table, real_t r, string_s** out) {
	uint8_t buf[256];
	sprintf(buf, REAL_FMT, r);
	return S_create_str(table, buf, out);
}

int S_create_int(mat_str_table_s* table, int32_t i, string_s** out) {
	uint8_t buf[256];
	sprintf(buf, "%d", i);
	return S_create_str(table, buf, out);
}

int S_conc_str(mat_str_table_s* table, const char* s1, const char* s2, string_s** out) {
	uint32_t l1, l2;
	string_s* str;
	assert(table);
	assert(out);

	l1 = (uint32_t)strlen(s1);
	l2 = (uint32_t)strlen(s2);
	str = malloc(sizeof(string_s) + l1 + l2 + 1);
	CHECK_MALLOC(str);

	strcpy(str->str, s1);
	strcpy(str->str + l1, s2);
	str->size = l1 + l2;
	str->hash = 0;
	add_to_table(table, str);
	*out = str;
	return 0;
}

int S_find_str(mat_str_table_s* table, const char* s, string_s** out) {
	uint32_t i;
	assert(table);
	assert(out);

	for (i = 0; i < V_SIZE(table->strs); ++i) {
		string_s* str = V_AT(table->strs, i, string_s*);

		if (str && strcmp(S_CSTR(str), s) == 0) {
			*out = str;
			return 0;
		}
	}

	return -1;
}

int S_get_str(mat_str_table_s* table, const char* s, string_s** out) {
	string_s* str;
	assert(table);
	assert(out);

	if (S_find_str(table, s, &str) == 0) {
		assert(str);
		*out = str;
		return 0;
	}

	return S_create_str(table, s, out);
}

uint32_t S_hash(string_s* s) {
	uint32_t len, hash;
	char* p;

	assert(s);
	p = s->str;
	len = s->size;

	if (s->hash)
		return s->hash;

	if (len == 0) {
		s->hash = -1;
		return 0;
	}

	hash = *p << 7;

	while (--len > 0)
		hash = (1000003 * hash) ^ *p++;

	hash ^= s->size;

	if (!hash)
		hash = (uint32_t)(-1);

	s->hash = hash;
	return hash;
}

int S_compare_eq(string_s* s1, string_s* s2) {
	if (s1->size != s2->size)
		return 0;

	if (s1 == s2)
		return 1;

	assert(s1->str);
	assert(s2->str);
	return strcmp(s1->str, s2->str) == 0;
}

int S_get_str_by_idx(mat_str_table_s* table, uint32_t idx, string_s** out) {
	string_s* str;
	assert(table);
	assert(out);

	if (idx >= V_SIZE(table->strs))
		return -1;

	str = V_AT(table->strs, idx, string_s*);
	*out = str;
	return 0;
}

int S_get_str_idx(mat_str_table_s* table, string_s* s, uint32_t* idx) {
	uint32_t i;

	assert(table);
	assert(s);
	assert(idx);

	for (i = 0; i < V_SIZE(table->strs); ++i) {
		string_s* str = V_AT(table->strs, i, string_s*);

		if (S_compare_eq(str, s)) {
			*idx = i;
			return 0;
		}
	}

	return -1;
}

int S_sweep(mat_str_table_s* table) {
	uint32_t i;

	for (i = 0; i < V_SIZE(table->strs); i++) {
		string_s* s = V_AT(table->strs, i, string_s*);

		if (s->str == NULL)
			continue;

		if (s->gc_mark == 0) {
			free(s);
			V_AT(table->strs, i, string_s*) = NULL;
			V_PUSH_BACK(table->free_idx, i, uint32_t);
		}
		else
			s->gc_mark = 0;
	}

	return 0;
}
