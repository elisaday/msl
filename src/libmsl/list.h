/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_LIST__
#define __H_LIST__

#include "matrix.h"

int LI_alloc(matrix_t mat, uint32_t size, list_s** out);
int LI_index_ref(matrix_t mat, list_s* l, obj_s* index, obj_s** out);

#endif // __H_LIST__
