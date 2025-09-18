/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "list.h"
#include "vec.h"
#include "err.h"
#include "pool.h"

int LI_alloc(matrix_t mat, uint32_t size, list_s** out) {
	list_s* list = POOL_list_alloc(&mat->pool_list, size);
	CHECK_MALLOC(list);

	*out = list;
	return 0;
}

int LI_index_ref(matrix_t mat, list_s* l, obj_s* index, obj_s** out) {
	int32_t n;
	obj_s* r;

	if (index->type != MAT_OT_INT32)
		return -1;

	n = index->int32;

	if (n < 0 || (uint32_t)n >= V_SIZE(l->v)) {
		return -1;
	}

	r = &V_AT(l->v, n, obj_s);
	*out = r;
	return 0;
}

