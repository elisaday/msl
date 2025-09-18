/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "vec.h"
#include "err.h"

int V_init(vec_s* v, uint32_t item_size, uint32_t size) {
	v->p = NULL;
	v->cap = 0;
	v->size = 0;
	v->item_size = item_size;
	return V_reserve(v, size);
}

void V_free(vec_s* v) {
	if (v->p) {
		free(v->p);
		v->p = NULL;
	}

	v->cap = 0;
	v->size = 0;
	v->item_size = 0;
}

int V_reserve(vec_s* v, uint32_t n) {
	if (n > v->cap) {
		v->p = realloc(v->p, v->item_size * n);
		CHECK_MALLOC(v->p);

		memset((char*)v->p + v->cap * v->item_size, 0, (n - v->cap) * v->item_size);
		v->cap = n;
	}

	return 0;
}

void V_clear(vec_s* v) {
	memset(v->p, 0, v->item_size * v->size);
	v->size = 0;
}

int V_alloc_one(vec_s* v) {
	if (v->size + 1 > v->cap)
		return V_reserve(v, (v->size == 0 ? 16 : (v->size << 1)));

	return 0;
}

