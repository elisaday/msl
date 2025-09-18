/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    create: 2013-6-7 22:53
    Copyright (c) 2006-2013 Zeb.  All rights reserved.
*/
#include <stdio.h>
#include <stdlib.h>
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

int math_randint(matrix_t mat) {
	int ret;
	int32_t start, end;

	mat_obj_t o_start = MAT_get_stack_obj(mat, 1);
	mat_obj_t o_end = MAT_get_stack_obj(mat, 2);

	ret = MAT_to_int32(mat, o_start, &start);
	CHECK_RESULT(ret);
	ret = MAT_to_int32(mat, o_end, &end);
	CHECK_RESULT(ret);

	CHECK_CONDITION(start <= end);
	MAT_push_int(mat, (int)(rand() % ((end - start) + 1) + start));
	ret = 0;
exit0:
	return ret;
}

MATRIX_API_EXPORT int import(matrix_t mat, mat_mod_t mod) {
	MAT_reg_func(mat, mod, "randint", math_randint);
	MAT_reg_float(mat, mod, "pi", 3.14159265359f);
	return 0;
}

