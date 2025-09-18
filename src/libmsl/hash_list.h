/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#ifndef __H_HASH_LIST__
#define __H_HASH_LIST__

/**
    一个数组和一个哈希表组成对象
    在哈希表中，将对象名字映射成数组下标
    再在数组中通过下标去访问具体对象
*/

// 初始化和销毁
int HL_init(hash_list_s* hl, int size);
void HL_free(hash_list_s* hl);

// 返回值为对象在数组中的下标，如果返回-1，表示失败
int32_t HL_set_obj(hash_list_s* hl, string_s* name, obj_s* obj);
int32_t HL_get_obj_idx(hash_list_s* hl, string_s* name);

// 通过对象名字直接取得对象
int HL_get_obj_direct(hash_list_s* hl, string_s* name, obj_s* obj);

// 通过对象在数组中的下标取得对象
int HL_get_obj(hash_list_s* hl, int32_t idx, obj_s* obj);

// 获取对象引用，此引用获取以后，不能保存，需要马上使用，以防止失效
int HL_ref_obj(hash_list_s* hl, int32_t idx, obj_s** ref);

hash_node_s* HL_first(hash_list_s* hl);
hash_node_s* HL_next(hash_list_s* hl, hash_node_s* pnode);

#define HL_SIZE(hl) (V_SIZE(hl.obj))

#endif
