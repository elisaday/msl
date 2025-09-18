/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "hash_list.h"
#include "vec.h"
#include "hash.h"
#include "err.h"


/* method */
int HL_init(hash_list_s* hl, int size) {
	assert(hl);

	if (V_init(&hl->obj, sizeof(obj_s), size) != 0)
		return -1;

	if (H_init(&hl->hash_obj, size) != 0)
		return -1;

	return 0;
}

void HL_free(hash_list_s* hl) {
	assert(hl);
	V_free(&hl->obj);
	H_free(&hl->hash_obj);
}

int32_t HL_set_obj(hash_list_s* hl, string_s* name, obj_s* obj) {
	int32_t idx;
	assert(hl);
	assert(name);
	assert(obj);

	idx = HL_get_obj_idx(hl, name);

	if (idx < 0) {
		obj_s key, val;
		key.type = MAT_OT_STR;
		key.str = name;
		val.type = MAT_OT_INT32;
		val.int32 = V_SIZE(hl->obj);

		if (H_set(&hl->hash_obj, &key, &val) != 0)
			return -1;

		V_PUSH_BACK(hl->obj, *obj, obj_s);
		return V_SIZE(hl->obj) - 1;
	}

	V_AT(hl->obj, idx, obj_s) = *obj;
	return idx;
}

int32_t HL_get_obj_idx(hash_list_s* hl, string_s* name) {
	obj_s key, val;
	assert(hl);
	assert(name);

	key.type = MAT_OT_STR;
	key.str = name;

	if (H_get(&hl->hash_obj, &key, &val) != 0)
		return -1;

	return val.int32;
}

int HL_get_obj_direct(hash_list_s* hl, string_s* name, obj_s* obj) {
	int idx;
	assert(hl);
	assert(name);
	assert(obj);

	idx = HL_get_obj_idx(hl, name);

	if (idx < 0)
		return -1;

	*obj = V_AT(hl->obj, idx, obj_s);
	return 0;
}

int HL_get_obj(hash_list_s* hl, int32_t idx, obj_s* obj) {
	if (idx < 0 || (uint32_t)idx >= V_SIZE(hl->obj))
		return -1;

	*obj = V_AT(hl->obj, idx, obj_s);
	return 0;
}

int HL_ref_obj(hash_list_s* hl, int32_t idx, obj_s** ref) {
	if (idx < 0 || (uint32_t)idx >= V_SIZE(hl->obj))
		return -1;

	*ref = &V_AT(hl->obj, idx, obj_s);
	return 0;
}

hash_node_s* HL_first(hash_list_s* hl) {
	return H_first(&hl->hash_obj);
}

hash_node_s* HL_next(hash_list_s* hl, hash_node_s* pnode) {
	return H_next(&hl->hash_obj, pnode);
}