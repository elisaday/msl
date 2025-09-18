/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "gc.h"
#include "vec.h"
#include "str.h"
#include "pool.h"
#include "hash_list.h"
#include "hash.h"

static void mark_obj(obj_s* o) {
	// 待处理：这里改为不递归
	if (o->type == MAT_OT_STR)
		o->str->gc_mark = 1;
	else if (o->type == MAT_OT_LIST) {
		uint32_t i;
		o->list->gc_mark = 1;

		for (i = 0; i < V_SIZE(o->list->v); ++i) {
			obj_s* child = &V_AT(o->list->v, i, obj_s);
			mark_obj(child);
		}
	}
	else if (o->type == MAT_OT_DICT) {
		hash_node_s* node;
		o->dict->gc_mark = 1;
		node = H_first(&o->dict->h);

		while (node) {
			mark_obj(&node->key);
			mark_obj(&node->val);
			node = H_next(&o->dict->h, node);
		}
	}
}

static void mark_stack(matrix_t mat) {
	obj_s* stack = &V_AT(mat->stack, 0, obj_s);
	uint32_t stack_top = mat->stack_top;

	while (stack_top--) {
		obj_s* o = stack + stack_top;
		mark_obj(o);
	}
}

static void mark_mod(mod_s* mod) {
	uint32_t i;

	for (i = 0; i < HL_SIZE(mod->objs); ++i) {
		obj_s obj;

		if (HL_get_obj(&mod->objs, i, &obj) != 0) {
			assert(0);
			continue;
		}

		mark_obj(&obj);
	}
}

static void mark_all_mod(matrix_t mat) {
	uint32_t i;

	for (i = 0; i < HL_SIZE(mat->mods); ++i) {
		obj_s obj;

		if (HL_get_obj(&mat->mods, i, &obj) != 0) {
			assert(0);
			continue;
		}

		assert(obj.type == MAT_OT_MOD);
		mark_mod(obj.mod);
	}
}

static void mark(matrix_t mat) {
	mark_stack(mat);
	mark_all_mod(mat);
}

static void sweep(matrix_t mat) {
	S_sweep(&mat->strs_gc);
	POOL_list_sweep(&mat->pool_list);
	POOL_dict_sweep(&mat->pool_dict);
}

int GC_run_once(matrix_t mat) {
	mark(mat);
	sweep(mat);
	return 0;
}
