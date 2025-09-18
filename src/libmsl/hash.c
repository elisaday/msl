/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "hash.h"
#include "str.h"
#include "vec.h"
#include "err.h"

#define MIN_HASH_SIZE 8

/*
    from python 2.5
    j = (5*j) + 1 + perturb;
    perturb >>= PERTURB_SHIFT;
    use j % 2**i as the next table index;
 */
#define PERTURB_SHIFT 5

static uint32_t get_hash_number(obj_s* key) {
	assert(key);

	switch (key->type) {
		case MAT_OT_INT32: {
			if (key->int32)
				return (uint32_t)key->int32;
			else
				return (uint32_t)(-1);
		}

		case MAT_OT_STR:
			return S_hash(key->str);

		default:
			return 0;
	}

	return 0;
}


static hash_node_s* get_hash_node(vec_s* nodes, uint32_t mask, obj_s* key, uint32_t hash) {
	uint32_t perturb = 0;
	uint32_t i = hash & mask;
	hash_node_s* node = 0;

	assert(nodes);
	assert(key);

	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		node = &V_AT(*nodes, (i & mask), hash_node_s);

		if (node->key.type == MAT_OT_DUMMY || O_compare_eq(&node->key, key))
			return node;
	}

	assert(0);
	return 0;
}

static hash_node_s* find_hash_node(vec_s* nodes, uint32_t mask, obj_s* key, uint32_t hash) {
	uint32_t perturb;
	uint32_t i = hash & mask;
	hash_node_s* node;
	int count = V_SIZE(*nodes);

	assert(nodes);
	assert(key);

	for (perturb = hash; count > 0; perturb >>= PERTURB_SHIFT, --count) {
		i = (i << 2) + i + perturb + 1;
		node = &V_AT(*nodes, (i & mask), hash_node_s);

		if (node->key.type != MAT_OT_DUMMY && O_compare_eq(&node->key, key))
			return node;
	}

	return NULL;
}

static int resize_hash(hash_s* h) {
	uint32_t new_size;
	vec_s new_vec;
	uint32_t i;

	assert(h);

	new_size = (h->mask + 1) << 1;

	if (V_init(&new_vec, sizeof(hash_node_s), new_size) != 0)
		return 0;

	V_SIZE(new_vec) = new_size;
	h->mask = new_size - 1;
	h->used = 0;

	for (i = 0; i < V_SIZE(h->nodes); ++i) {
		hash_node_s* node;
		hash_node_s* new_node;

		node = &V_AT(h->nodes, i, hash_node_s);

		if (node->key.type != MAT_OT_DUMMY) {
			new_node = get_hash_node(&new_vec, h->mask, &node->key, node->hash);
			assert(new_node);
			assert(new_node->key.type == MAT_OT_DUMMY);

			new_node->hash = node->hash;
			new_node->key = node->key;
			new_node->val = node->val;
			h->used++;
		}
	}

	V_free(&h->nodes);
	h->nodes = new_vec;
	return 0;
}

/* method */
int H_init(hash_s* h, uint32_t size) {
	uint32_t s = MIN_HASH_SIZE;

	while (s < size)
		s <<= 1;

	h->mask = s - 1;
	h->used = 0;

	if (V_init(&h->nodes, sizeof(hash_node_s), s) != 0)
		return -1;

	V_SIZE(h->nodes) = s;
	return 0;
}

void H_free(hash_s* h) {
	assert(h);
	V_free(&h->nodes);
	h->mask = 0;
	h->used = 0;
}

void H_clear(hash_s* h) {
	V_clear(&h->nodes);
	h->used = 0;
}

int H_set(hash_s* h, obj_s* key, obj_s* val) {
	uint32_t hash = 0;
	uint32_t perturb = 0;
	hash_node_s* node = 0;
	assert(h);
	assert(key);
	assert(val);

	hash = get_hash_number(key);

	if (!hash)
		return -1;

	node = get_hash_node(&h->nodes, h->mask, key, hash);
	assert(node);

	if (node->key.type == MAT_OT_DUMMY) {
		node->hash = hash;
		node->key = *key;
		node->val = *val;
		h->used++;

		if (h->used * 3 > (h->mask + 1) * 2)
			return resize_hash(h);

		return 0;
	}
	else {
		node->hash = hash;
		node->val = *val;
		return 0;
	}

	return -1;
}

int H_get(hash_s* h, obj_s* key, obj_s* val) {
	obj_s* ref;
	assert(h);
	assert(key);
	assert(val);

	if (H_get_ref(h, key, &ref) != 0)
		return -1;

	*val = *ref;
	return 0;
}

int H_get_ref(hash_s* h, obj_s* key, obj_s** ref) {
	uint32_t hash;
	hash_node_s* node;

	assert(h);
	assert(key);
	assert(ref);

	hash = get_hash_number(key);

	if (!hash)
		return -1;

	node = find_hash_node(&h->nodes, h->mask, key, hash);

	if (node == NULL)
		return -1;

	*ref = &node->val;
	return 0;
}

hash_node_s* H_first(hash_s* h) {
	uint32_t i;
	assert(h);

	for (i = 0; i < V_SIZE(h->nodes); ++i) {
		hash_node_s* node = &V_AT(h->nodes, i, hash_node_s);

		if (node->key.type != MAT_OT_DUMMY)
			return node;
	}

	return NULL;
}

hash_node_s* H_next(hash_s* h, hash_node_s* prev_node) {
	uint32_t i;
	uint32_t start;
	hash_node_s* phead;

	assert(h);
	assert(prev_node);
	assert(V_SIZE(h->nodes));

	phead = &V_AT(h->nodes, 0, hash_node_s);

	if (prev_node < phead)
		return NULL;

	start = (uint32_t)(prev_node - phead + 1);

	for (i = start; i < V_SIZE(h->nodes); ++i) {
		prev_node = &V_AT(h->nodes, i, hash_node_s);

		if (prev_node->key.type != MAT_OT_DUMMY)
			return prev_node;
	}

	return NULL;
}

void H_print(hash_s* h) {
	uint32_t i;
	int first = 1;
	assert(h);
	printf("{");

	for (i = 0; i < V_SIZE(h->nodes); i++) {
		hash_node_s* node = &V_AT(h->nodes, i, hash_node_s);

		if (node->key.type == MAT_OT_DUMMY)
			continue;

		if (first)
			first = 0;
		else
			printf(", ");

		O_dump(&node->key);
		printf(":");
		O_dump(&node->val);
	}

	printf("}");
}

