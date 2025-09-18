/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "pool.h"
#include "err.h"
#include "vec.h"
#include "hash.h"

static void free_list(list_s* head) {
	while (head) {
		list_s* next = head->next;
		V_free(&head->v);
		free(head);
		head = next;
	}
}

static void cache_list(pool_list_s* pl, list_s* list) {
	uint32_t size;
	int i;
	int max_idx;

	if (V_SIZE(list->v) > 32) {
		max_idx = 3;
		size = pl->size[0] + pl->size[1] + pl->size[2];
	}
	else if (V_SIZE(list->v) > 16) {
		max_idx = 2;
		size = pl->size[0] + pl->size[1];
	}
	else {
		max_idx = 1;
		size = pl->size[0];
	}

	// 找出最小的一个，将list放入
	for (i = 0; i < max_idx; i++) {
		if (pl->size[i] * max_idx <= size) {
			V_clear(&list->v);
			list->next = pl->free[i];
			pl->free[i] = list;
			pl->size[i]++;
			return;
		}
	}

	assert(0);
}

static list_s* alloc_list(uint32_t n, uint32_t size) {
	int ret;
	list_s* head = NULL;
	uint32_t s = size;

	while (s--) {
		list_s* list = malloc(sizeof(list_s));
		CHECK_MALLOC(list);

		list->gc_mark = 0;
		ret = V_init(&list->v, sizeof(obj_s), size);
		CHECK_RESULT(ret);

		list->next = head;
		head = list;
	}

	return head;

exit0:
	free_list(head);
	return NULL;
}

static void free_dict(dict_s* head) {
	while (head) {
		dict_s* next = head->next;
		H_free(&head->h);
		free(head);
		head = next;
	}
}

static void cache_dict(pool_dict_s* pd, dict_s* dict) {
	uint32_t size;
	int i;
	int max_idx;

	if (H_SIZE(dict->h) > 32) {
		max_idx = 3;
		size = pd->size[0] + pd->size[1] + pd->size[2];
	}
	else if (H_SIZE(dict->h) > 16) {
		max_idx = 2;
		size = pd->size[0] + pd->size[1];
	}
	else {
		max_idx = 1;
		size = pd->size[0];
	}

	// 找出最小的一个，将list放入
	for (i = 0; i < max_idx; i++) {
		if (pd->size[i] * max_idx <= size) {
			H_clear(&dict->h);
			dict->next = pd->free[i];
			pd->free[i] = dict;
			pd->size[i]++;
			return;
		}
	}

	assert(0);
}

static dict_s* alloc_dict(uint32_t n, uint32_t size) {
	int ret;
	dict_s* head = NULL;
	uint32_t s = size;

	while (s--) {
		dict_s* dict = malloc(sizeof(dict_s));
		CHECK_MALLOC(dict);

		dict->gc_mark = 0;
		ret = H_init(&dict->h, size);
		CHECK_RESULT(ret);

		dict->next = head;
		head = dict;
	}

	return head;

exit0:
	free_dict(head);
	return NULL;
}

int POOL_list_init(pool_list_s* pl) {
	memset(pl->free, 0, sizeof(pl->free));
	memset(pl->size, 0, sizeof(pl->size));
	pl->used = NULL;
	return 0;
}

void POOL_list_free(pool_list_s* pl) {
	int i;

	for (i = 0; i < _countof(pl->free); ++i) {
		free_list(pl->free[i]);
		pl->free[i] = NULL;
		pl->size[i] = 0;
	}

	if (pl->used) {
		free_list(pl->used);
		pl->used = NULL;
	}
}

list_s* POOL_list_alloc(pool_list_s* pl, uint32_t size) {
	list_s* ret;
	int i;

	if (size > 32) {
		ret = alloc_list(1, size);
		ret->next = pl->used;
		pl->used = ret;
		return ret;
	}

	if (size > 16) {
		i = 2;
		size = 32;
	}
	else if (size > 8) {
		i = 1;
		size = 16;
	}
	else {
		i = 0;
		size = 8;
	}

	if (pl->free[i] == NULL) {
		pl->free[i] = alloc_list(POOL_ALLOC_LIST, size);
		pl->size[i] = POOL_ALLOC_LIST;
	}

	ret = pl->free[i];
	pl->free[i] = pl->free[i]->next;
	pl->size[i]--;

	ret->next = pl->used;
	pl->used = ret;
	return ret;
}

void POOL_list_sweep(pool_list_s* pl) {
	list_s* list = pl->used;
	list_s* prev = NULL;

	while (list) {
		if (list->gc_mark == 0) {
			if (prev == NULL) {
				list_s* node = list;
				list = list->next;
				cache_list(pl, node);
				pl->used = list;
			}
			else {
				list_s* node = list;
				prev->next = list->next;
				cache_list(pl, node);
				list = prev->next;
			}
		}
		else {
			list->gc_mark = 0;
			prev = list;
			list = list->next;
		}
	}
}

int POOL_dict_init(pool_dict_s* pd) {
	memset(pd->free, 0, sizeof(pd->free));
	memset(pd->size, 0, sizeof(pd->size));
	pd->used = NULL;
	return 0;
}

void POOL_dict_free(pool_dict_s* pd) {
	int i;

	for (i = 0; i < _countof(pd->free); ++i) {
		if (pd->free[i]) {
			free_dict(pd->free[i]);
			pd->free[i];
		}
	}

	if (pd->used) {
		free_dict(pd->used);
		pd->used = NULL;
	}
}

dict_s* POOL_dict_alloc(pool_dict_s* pd, uint32_t size) {
	dict_s* ret;
	int i;

	if (size > 32) {
		ret = alloc_dict(1, size);
		ret->next = pd->used;
		pd->used = ret;
		return ret;
	}

	if (size > 16) {
		i = 2;
		size = 32;
	}
	else if (size > 8) {
		i = 1;
		size = 16;
	}
	else {
		i = 0;
		size = 8;
	}

	if (pd->free[i] == NULL) {
		pd->free[i] = alloc_dict(POOL_ALLOC_DICT, size);
		pd->size[i] = POOL_ALLOC_DICT;
	}

	ret = pd->free[i];
	pd->free[i] = pd->free[i]->next;
	pd->size[i]--;

	ret->next = pd->used;
	pd->used = ret;
	return ret;
}

void POOL_dict_sweep(pool_dict_s* pd) {
	dict_s* dict = pd->used;
	dict_s* prev = NULL;

	while (dict) {
		if (dict->gc_mark == 0) {
			if (prev == NULL) {
				dict_s* node = dict;
				dict = dict->next;
				cache_dict(pd, node);
				pd->used = dict;
			}
			else {
				dict_s* node = dict;
				prev->next = dict->next;
				cache_dict(pd, node);
				dict = prev->next;
			}
		}
		else {
			dict->gc_mark = 0;
			prev = dict;
			dict = dict->next;
		}
	}
}