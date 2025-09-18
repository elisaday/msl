/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "dict.h"
#include "err.h"
#include "hash.h"
#include "pool.h"

int D_alloc(matrix_t mat, uint32_t size, dict_s** out) {
	dict_s* dict = POOL_dict_alloc(&mat->pool_dict, size);
	CHECK_MALLOC(dict);

	*out = dict;
	return 0;
}

int D_index_ref(matrix_t mat, dict_s* d, obj_s* index, obj_s** out) {
	int ret = H_get_ref(&d->h, index, out);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}
