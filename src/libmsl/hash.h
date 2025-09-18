/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_HASH__
#define __H_HASH__

#include "obj.h"

int H_init(hash_s* h, uint32_t size);
void H_free(hash_s* h);
void H_clear(hash_s* h);
int H_set(hash_s* h, obj_s* key, obj_s* val);
int H_get(hash_s* h, obj_s* key, obj_s* val);

// 获取对象引用，此引用获取以后，不能保存，需要马上使用，以防止失效
int H_get_ref(hash_s* h, obj_s* key, obj_s** ref);

hash_node_s* H_first(hash_s* h);
hash_node_s* H_next(hash_s* h, hash_node_s* prev_node);

void H_print(hash_s* h);

#define H_SIZE(h) (h).used

#endif

