/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_DEBUG__
#define __H_DEBUG__

int DBG_init(matrix_t mat);
void DBG_free(matrix_t mat);
int DBG_add_mod(matrix_t mat, mod_s* mod);
int DBG_add_op_line(matrix_t mat, mod_s* mod, func_s* func, uint32_t op_pos, uint32_t line);
uint32_t DBG_get_line(matrix_t mat, uint32_t* op);

#endif // __H_DEBUG__
