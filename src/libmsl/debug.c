/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "debug.h"
#include "err.h"
#include "vec.h"
#include "str.h"

static uint32_t get_pos(matrix_t mat, uint32_t* op, mod_s** mod, func_s** func) {
	uint32_t i;

	for (i = 0; i < V_SIZE(mat->dbg_info); ++i) {
		dbg_info_s* info = &V_AT(mat->dbg_info, i, dbg_info_s);

		if (op >= info->ip_begin && op < info->ip_end) {
			*mod = info->mod;
			*func = info->func;
			return op - info->ip_begin;
		}
	}

	return 0;
}

int DBG_init(matrix_t mat) {
	int ret;

	ret = V_init(&mat->dbg_info, sizeof(dbg_info_s), DEFAULT_VM_MOD_NUM);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

void DBG_free(matrix_t mat) {
	V_free(&mat->dbg_info);
}

int DBG_add_mod(matrix_t mat, mod_s* mod) {
	uint32_t i;
	dbg_info_s* dbg;
	V_PUSH_BACK_GET(mat->dbg_info, dbg, dbg_info_s);
	dbg->mod = mod;
	dbg->func = NULL;
	dbg->ip_begin = &V_AT(mod->code, 0, uint32_t);
	dbg->ip_end = dbg->ip_begin + V_SIZE(mod->code);

	for (i = 0; i < V_SIZE(mod->funcs); ++i) {
		func_s* func = V_AT(mod->funcs, i, func_s*);
		V_PUSH_BACK_GET(mat->dbg_info, dbg, dbg_info_s);
		dbg->mod = mod;
		dbg->func = func;
		dbg->ip_begin = &V_AT(func->code, 0, uint32_t);
		dbg->ip_end = dbg->ip_begin + V_SIZE(func->code);
	}

	return 0;
}

int DBG_add_op_line(matrix_t mat, mod_s* mod, func_s* func, uint32_t op_pos, uint32_t line) {
	if (func) {
		op_line_s* opl;
		V_PUSH_BACK_GET(func->op_line, opl, op_line_s);
		opl->op_pos = op_pos;
		opl->line = line;
	}
	else {
		op_line_s* opl;
		V_PUSH_BACK_GET(mod->op_line, opl, op_line_s);
		opl->op_pos = op_pos;
		opl->line = line;
	}

	return 0;
}

uint32_t DBG_get_line(matrix_t mat, uint32_t* op) {
	uint32_t i;
	mod_s* mod;
	func_s* func;
	op_line_s* opl;
	uint32_t op_pos = get_pos(mat, op, &mod, &func);

	if (op_pos == 0)
		return 0;

	if (func) {

		for (i = 0; i < V_SIZE(func->op_line) - 1; ++i) {
			op_line_s* opl = &V_AT(func->op_line, i, op_line_s);
			op_line_s* opl_next = &V_AT(func->op_line, i + 1, op_line_s);

			if (op_pos >= opl->op_pos && op_pos < opl_next->op_pos)
				return opl->line;
		}

		opl = &V_AT(func->op_line, i, op_line_s);
		return opl->line;
	}
	else {
		for (i = 0; i < V_SIZE(mod->op_line) - 1; ++i) {
			op_line_s* opl = &V_AT(mod->op_line, i, op_line_s);
			op_line_s* opl_next = &V_AT(mod->op_line, i + 1, op_line_s);

			if (op_pos >= opl->op_pos && op_pos < opl->op_pos)
				return opl->line;
		}

		opl = &V_AT(mod->op_line, i, op_line_s);
		return opl->line;
	}
}

