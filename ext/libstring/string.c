/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    create: 2013-7-17 23:36
    Copyright (c) 2006-2013 Zeb.  All rights reserved.
*/
#include <stdio.h>
#include <assert.h>
#include "matrix.h"

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

int string_join(matrix_t mat) {
	int ret;
	int i;
	int param_num;
	size_t len = 0;

	param_num = MAT_get_param_num(mat);

	for (i = 1; i <= param_num; ++i) {
		uint32_t l;
		mat_obj_t o = MAT_get_stack_obj(mat, i);

		if (MAT_obj_type(o) != MAT_OT_STR)
			return -1;

		ret = MAT_str_len(mat, o, &l);
		len += l;
	}

	for (i = 1; i <= param_num; ++i) {
		mat_obj_t o = MAT_get_stack_obj(mat, i);

	}

	ret = 0;
exit0:
	return ret;
}

MATRIX_API_EXPORT int import(matrix_t mat, mat_mod_t mod) {
	MAT_reg_func(mat, mod, "join", string_join);
	return 0;
}

