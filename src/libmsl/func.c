/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "func.h"
#include "err.h"
#include "hash_list.h"
#include "config.h"
#include "vec.h"

int F_init(func_s* func) {
	int ret;

	ret = V_init(&func->code, sizeof(uint32_t), DEFAULT_FUNC_CODE_SIZE);
	CHECK_RESULT(ret);

	ret = V_init(&func->op_line, sizeof(op_line_s), DEFAULT_FUNC_CODE_SIZE);
	CHECK_RESULT(ret);

	ret = HL_init(&func->objs, DEFAULT_FUNC_OBJ_SIZE);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

void F_free(func_s* func) {
	HL_free(&func->objs);
	V_free(&func->op_line);
	V_free(&func->code);
}
