/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "matrix.h"
#include "header.h"

static uint64_t get_cur_tick() {
#if defined(PLATFORM_WINDOWS)
	FILETIME ft;
	ULARGE_INTEGER li;
	GetSystemTimeAsFileTime(&ft);
	li.LowPart  = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return li.QuadPart / 10000;
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)((uint64_t) tv.tv_sec * 1000 + (uint64_t) tv.tv_usec / 1000);
#endif
}

static void sleep_ms(uint64_t ms) {
#if defined(PLATFORM_WINDOWS)
	Sleep((DWORD)ms);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	usleep(ms * 1000);
#endif
}

static int info(matrix_t mat) {
	printf("author: zeb <zebbey@gmail.com>\n");
	printf("license: LGPL\n");
	MAT_push_none(mat);
	return 0;
}

static int print(matrix_t mat) {
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	MAT_print(o);
	MAT_push_none(mat);
	return 0;
}

static int println(matrix_t mat) {
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	MAT_print(o);
	printf("\n");
	MAT_push_none(mat);
	return 0;
}

static int tick(matrix_t mat) {
	MAT_push_int(mat, (int)get_cur_tick());
	return 0;
}

static int _sleep(matrix_t mat) {
	int ret;
	uint32_t i;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	if (MAT_obj_type(o) != MAT_OT_INT32)
		return -1;

	ret = MAT_to_int32(mat, o, &i);

	if (ret != 0)
		return ret;

	sleep_ms((uint64_t)i);
	MAT_push_none(mat);
	return 0;
}

static int abs_num(matrix_t mat) {
	int ret;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	if (MAT_obj_type(o) == MAT_OT_INT32) {
		int i;
		ret = MAT_to_int32(mat, o, &i);

		if (ret != 0)
			return ret;

		MAT_push_int(mat, abs(i));
	}
	else if (MAT_obj_type(o) == MAT_OT_REAL) {
		float f;
		ret = MAT_to_float(mat, o, &f);

		if (ret != 0)
			return ret;

#if defined(_MSC_VER) && (_MSC_VER == 1310)
		MAT_push_float(mat, (float)fabs(f));
#else
		MAT_push_float(mat, fabsf(f));
#endif
	}
	else
		return -1;

	return 0;
}

static int cast_int(matrix_t mat) {
	int ret;
	int32_t i;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	ret = MAT_to_int32(mat, o, &i);

	if (ret != 0)
		return ret;

	MAT_push_int(mat, (int)i);
	return 0;
}

static int cast_float(matrix_t mat) {
	int ret;
	real_t f;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	ret = MAT_to_float(mat, o, &f);

	if (ret != 0)
		return ret;

	MAT_push_float(mat, (float)f);
	return 0;
}

static int cast_str(matrix_t mat) {
	int ret;
	mat_str_t s;
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	ret = MAT_to_str_obj(mat, o, &s);

	if (ret != 0)
		return ret;

	MAT_push_str_obj(mat, s);
	return 0;
}

#if defined(PLATFORM_WINDOWS)
#define MAX_ENVIRON_LEN 32767
static int env(matrix_t mat) {
	int n;
	const char* k;
	char path[MAX_ENVIRON_LEN];
	mat_obj_t o = MAT_get_stack_obj(mat, 1);

	if (o == NULL)
		return -1;

	k = MAT_to_str(mat, o);

	if (k == NULL)
		return -1;

	n = GetEnvironmentVariable(k, path, MAX_ENVIRON_LEN);
	if (n == 0 || n == MAX_ENVIRON_LEN)
		return -1;

	path[n] = '\0';
	MAT_push_str(mat, path);
	return 0;
}
#endif

int BU_import(matrix_t mat, mat_mod_t mod) {
	MAT_reg_func(mat, mod, "info", info);
	MAT_reg_func(mat, mod, "print", print);
	MAT_reg_func(mat, mod, "println", println);
	MAT_reg_func(mat, mod, "tick", tick);
	MAT_reg_func(mat, mod, "sleep", _sleep);
	MAT_reg_func(mat, mod, "abs", abs_num);
	MAT_reg_func(mat, mod, "int", cast_int);
	MAT_reg_func(mat, mod, "float", cast_float);
	MAT_reg_func(mat, mod, "str", cast_str);

#if defined(PLATFORM_WINDOWS)
	MAT_reg_func(mat, mod, "env", env);
#endif
	return 0;
}
