/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_POOL__
#define __H_POOL__

int POOL_list_init(pool_list_s* pl);
void POOL_list_free(pool_list_s* pl);
list_s* POOL_list_alloc(pool_list_s* pl, uint32_t size);
void POOL_list_sweep(pool_list_s* pl);

int POOL_dict_init(pool_dict_s* pd);
void POOL_dict_free(pool_dict_s* pd);
dict_s* POOL_dict_alloc(pool_dict_s* pd, uint32_t size);
void POOL_dict_sweep(pool_dict_s* pd);

#endif // __H_POOL__
