/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_VECTOR__
#define __H_VECTOR__

#include "obj.h"

int V_init(vec_s* v, uint32_t item_size, uint32_t cap);
void V_free(vec_s* v);
int V_reserve(vec_s* v, uint32_t cap);
void V_clear(vec_s* v);

// 检查数组空间是否能够容纳一个新元素，如果不够会进行分配
int V_alloc_one(vec_s* v);

#define V_SIZE(v) ((v).size)
#define V_CAP(v) ((v).cap)
#define V_ITEM_SIZE(v) ((v).item_size)
#define V_AT(v, idx, type) (((type*)(v).p)[idx])
#define V_BACK(v, type) (((type*)(v).p)[V_SIZE(v) - 1])

#define V_PUSH_BACK(v, data, type)\
	do {\
		if (V_alloc_one(&v) != 0)\
			return -1;\
		V_AT(v, V_SIZE(v)++, type) = (data);\
	} while (0);

#define V_PUSH_BACK_GET(v, ret, type)\
	do {\
		if (V_alloc_one(&(v)) != 0)\
			return -1;\
		ret = (type*)&V_AT(v, V_SIZE(v)++, type);\
	} while (0);

#endif
