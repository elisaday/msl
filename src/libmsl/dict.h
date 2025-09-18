/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_DICT__
#define __H_DICT__

#include "matrix.h"

int D_alloc(matrix_t mat, uint32_t size, dict_s** out);
int D_index_ref(matrix_t mat, dict_s* d, obj_s* index, obj_s** out);

#endif // __H_DICT__

