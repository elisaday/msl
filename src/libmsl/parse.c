/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "parse.h"
#include "obj.h"
#include "err.h"
#include "hash.h"
#include "vec.h"
#include "lex.h"
#include "str.h"
#include "ins.h"
#include "func.h"
#include "hash_list.h"
#include "opt.h"
#include "debug.h"
#include "vm.h"
#include "builtins.h"

#define NEXT_TOKEN(t)\
	do {\
		int r = L_read_token(&t);\
		if (r != 0)\
			return r;\
		if (t.tt == TT_STREAM_END)\
			return -2;\
	} while (0)

#define TRY_NEXT_TOKEN(t)\
	do {\
		if (L_read_token(&t) != 0)\
			return -1;\
	} while (0)

#define EXPECT_TOKEN(t, n)\
	do {\
		if (t.tt != n) {\
			E_rt_err(P.mat, P.file_name, t.line, "Token \"%s\" is not expected.", L_token_to_string(&t));\
			return -1;\
		}\
	} while (0)

#define  EXPECT_NEXT_TOKEN(t, n)\
	do {\
		NEXT_TOKEN(t);\
		EXPECT_TOKEN(t, n);\
	} while (0)

#define CUR_CODE_POS V_SIZE(*get_cur_code())
#define ADD_INS(i)  V_PUSH_BACK(*get_cur_code(), (uint32_t)i, uint32_t)
#define SET_INS(pos, i)\
	do {\
		V_AT(*get_cur_code(), pos, uint32_t) = i;\
	} while (0)

#define ADD_OP_LINE(line)\
	do {\
		if (DBG_add_op_line(P.mat, P.mod, P.func, CUR_CODE_POS, line) != 0)\
			return -1;\
	} while (0)

// 循环体解析信息
typedef struct {
	uint32_t enter[MAX_CONTINUE_NUM];
	int enter_size;
	uint32_t exit[MAX_BREAK_NUM];
	int exit_size;
} parse_loop_s;

// 解析状态信息
typedef struct parse_state_s {
	matrix_t mat;
	const char* file_name;
	mod_s* mod;
	func_s* func;
	parse_loop_s loops[MAX_LOOP_NEST];
	int loop_size;
} parse_state_s;

static parse_state_s P;

static int parse_program();
static int parse_statement();
static int parse_get_obj_global(string_s* name, int32_t* id_pos);
static int parse_get_obj_local(string_s* name, int32_t* id_pos);
static int parse_get_obj(string_s* name, int32_t* id_pos);
static int parse_left_value(int32_t* mod_pos, int32_t* id_pos, uint32_t* index_num);
static int parse_assign(operator_e ot, uint32_t line, int32_t mod_pos, int32_t id_pos, uint32_t index_num);
static int parse_left_identifier();
static int parse_right_value(int32_t* mod_pos, int32_t* id_pos, uint32_t* index_num);
static int parse_right_identifier();
static int parse_expression();
static int parse_sub_expression();
static int parse_term();
static int parse_make_list();
static int parse_make_dict();
static int parse_factor();
static int parse_if();
static int parse_then();
static int parse_block();
static int parse_while();
static int parse_function();
static int parse_func_param(func_s* func);
static int parse_func_call();
static int parse_import();

static int init_parser(matrix_t mat, mod_s* mod) {
	int ret;
	assert(mat);
	assert(mod);

	P.mat = mat;
	P.file_name = mod->name->str;
	P.mod = mod;
	P.func = NULL;
	memset(P.loops, 0, sizeof(P.loops));
	P.loop_size = 0;

	ret = L_set_env(mat, mod->name->str, &mat->strs_nogc);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

static int init_parser_str(matrix_t mat, mod_s* mod, const char* str, uint32_t size) {
	int ret;
	assert(mat);
	assert(mod);
	assert(str);

	P.mat = mat;
	P.file_name = NULL;
	P.mod = mod;
	P.func = NULL;
	memset(P.loops, 0, sizeof(P.loops));
	P.loop_size = 0;

	ret = L_set_env_str(mat, mod->name->str, str, size, &mat->strs_nogc);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

static vec_s* get_cur_code() {
	vec_s* code = &P.mod->code;

	if (P.func)
		code = &P.func->code;

	return code;
}

static int parse_program() {
	int ret;
	token_s t;

	while (1) {
		ret = L_read_token(&t);

		if (ret != 0)
			return ret;

		if (t.tt == TT_STREAM_END) {
			ADD_INS(IT_EXIT);
			return 0;
		}
		else if (t.tt == TT_REV_DEF) {
			ret = parse_function();
			CHECK_RESULT(ret);
		}
		else {
			L_unread_token(&t);
			ret = parse_statement();
			CHECK_RESULT(ret);
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_statement() {
	int ret;
	token_s t;
	NEXT_TOKEN(t);

	switch (t.tt) {
		case TT_SEMICOLON:
			break;

		case TT_REV_BREAK: {
			parse_loop_s* loop;

			if (P.loop_size == 0) {
				E_rt_err(P.mat, P.file_name, t.line, "Cannot find loop statement to break.");
				return -1;
			}

			loop = P.loops + P.loop_size - 1;

			if (loop->exit_size >= MAX_BREAK_NUM) {
				E_rt_err(P.mat, P.file_name, t.line, "There are too many \"break\" in the loop statement, the max value is %d.", MAX_BREAK_NUM);
				return -1;
			}

			ADD_OP_LINE(t.line);
			ADD_INS(IT_JMP);
			loop->exit[loop->exit_size++] = CUR_CODE_POS;
			ADD_INS(0);

			EXPECT_NEXT_TOKEN(t, TT_SEMICOLON);
			break;
		}

		case TT_REV_CONTINUE: {
			parse_loop_s* loop;

			if (P.loop_size == 0) {
				E_rt_err(P.mat, P.file_name, t.line, "Cannot find loop statement to break.");
				return -1;
			}

			loop = P.loops + P.loop_size - 1;

			if (loop->enter_size >= MAX_CONTINUE_NUM) {
				E_rt_err(P.mat, P.file_name, t.line, "There is too many \"continue\" in the loop statement, the max value is %d.", MAX_CONTINUE_NUM);
				return -1;
			}

			ADD_OP_LINE(t.line);
			ADD_INS(IT_JMP);
			loop->enter[loop->enter_size++] = CUR_CODE_POS;
			ADD_INS(0);

			EXPECT_NEXT_TOKEN(t, TT_SEMICOLON);
			break;
		}

		case TT_REV_RETURN: {
			if (P.func == NULL) {
				E_rt_err(P.mat, P.file_name, t.line, "\"return\" can only be uesd in function.");
				return -1;
			}

			NEXT_TOKEN(t);

			if (t.tt == TT_SEMICOLON) {
				ADD_OP_LINE(t.line);
				ADD_INS(IT_RET);
				return 0;
			}
			else {
				L_unread_token(&t);

				ret = parse_expression();
				CHECK_RESULT(ret);

				ADD_OP_LINE(t.line);
				ADD_INS(IT_RET_RESULT);
				EXPECT_NEXT_TOKEN(t, TT_SEMICOLON);
				return 0;
			}

			break;
		}

		case TT_REV_IF:
			ret = parse_if();
			CHECK_RESULT(ret);
			break;

		case TT_REV_WHILE:
			ret = parse_while();
			CHECK_RESULT(ret);
			break;

		case TT_IDENTIFIER:
			L_unread_token(&t);
			ret = parse_left_identifier();
			CHECK_RESULT(ret);
			break;

		case TT_REV_IMPORT:
			if (P.func != NULL || P.loop_size > 0) {
				E_rt_err(P.mat, P.file_name, t.line, "\"import\" can only be uesd in global space.");
				return -1;
			}

			ret = parse_import();
			CHECK_RESULT(ret);
			break;

		default:
			E_rt_err(P.mat, P.file_name, t.line, "Unknown statement.");
			return -1;
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_get_obj_global(string_s* name, int32_t* id_pos) {
	obj_s obj;
	int is_func = 0;
	mod_s* mod = P.mod;
	int32_t idx;

	idx = HL_get_obj_idx(&mod->objs, name);

	if (idx < 0) {
		obj.type = MAT_OT_DUMMY;
		idx = HL_set_obj(&mod->objs, name, &obj);

		if (idx < 0)
			return -1;
	}

	*id_pos = idx;
	return 0;
}

static int parse_get_obj_local(string_s* name, int32_t* id_pos) {
	obj_s obj;
	func_s* func = P.func;
	int32_t idx = HL_get_obj_idx(&func->objs, name);

	if (idx < 0) {
		mod_s* mod = P.mod;
		idx = HL_get_obj_idx(&mod->objs, name);

		if (idx < 0) {
			obj.type = MAT_OT_DUMMY;
			idx = HL_set_obj(&func->objs, name, &obj);

			if (idx < 0)
				return -1;
		}
		else {
			*id_pos = idx;
			return 0;
		}
	}

	*id_pos = -idx - 1;
	return 0;
}

static int parse_get_obj(string_s* name, int32_t* id_pos) {
	if (P.func)
		return parse_get_obj_local(name, id_pos);
	else
		return parse_get_obj_global(name, id_pos);
}

static int parse_left_value(int32_t* mod_pos, int32_t* id_pos, uint32_t* index_num) {
	int ret;
	token_s t;
	EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

	*mod_pos = -1;
	ret = parse_get_obj(t.str, id_pos);
	CHECK_RESULT(ret);

	NEXT_TOKEN(t);

	if (t.tt == TT_COLON) {
		int32_t idx;
		obj_s* o;
		*mod_pos = *id_pos;

		EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

		ret = HL_ref_obj(&P.mod->objs, *mod_pos, &o);
		CHECK_RESULT(ret);
		CHECK_CONDITION(o->type == MAT_OT_MOD);

		idx = HL_get_obj_idx(&o->mod->objs, t.str);

		if (idx < 0) {
			obj_s obj;
			obj.type = MAT_OT_DUMMY;
			idx = HL_set_obj(&o->mod->objs, t.str, &obj);

			if (idx < 0)
				return -1;
		}

		*id_pos = idx;
	}
	else {
		L_unread_token(&t);
	}

	NEXT_TOKEN(t);

	// index. array, dict etc...
	while (t.tt == TT_OPEN_BRACE) {
		ret = parse_expression();
		CHECK_RESULT(ret);
		EXPECT_NEXT_TOKEN(t, TT_CLOSE_BRACE);
		(*index_num)++;
		NEXT_TOKEN(t);
	}


	L_unread_token(&t);

	ret = 0;
exit0:
	return ret;
}

static int parse_assign(operator_e ot, uint32_t line, int32_t mod_pos, int32_t id_pos, uint32_t index_num) {
	token_s t;

	switch (ot) {
		case OT_ASSIGN:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN);
			break;

		case OT_ASSIGN_ADD:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_ADD);
			break;

		case OT_ASSIGN_SUB:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_SUB);
			break;

		case OT_ASSIGN_MUL:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_MUL);
			break;

		case OT_ASSIGN_DIV:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_DIV);
			break;

		case OT_ASSIGN_MOD:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_MOD);
			break;

		case OT_ASSIGN_EXP:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_EXP);
			break;

		case OT_ASSIGN_AND:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_AND);
			break;

		case OT_ASSIGN_OR:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_OR);
			break;

		case OT_ASSIGN_XOR:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_XOR);
			break;

		case OT_ASSIGN_SHIFT_LEFT:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_SHIFT_LEFT);
			break;

		case OT_ASSIGN_SHIFT_RIGHT:
			ADD_OP_LINE(line);
			ADD_INS(IT_ASSIGN_SHIFT_RIGHT);
			break;

		default:
			E_rt_err(P.mat, P.file_name, line, "Invalid operator.");
			return -1;
	}

	ADD_INS((uint32_t)mod_pos);
	ADD_INS((uint32_t)id_pos);
	ADD_INS(index_num);

	EXPECT_NEXT_TOKEN(t, TT_SEMICOLON);
	return 0;
}

static int parse_left_identifier() {
	int ret;
	token_s t;
	int32_t mod_pos = 0;
	int32_t id_pos = 0;
	uint32_t index_num = 0;

	ret = parse_left_value(&mod_pos, &id_pos, &index_num);
	CHECK_RESULT(ret);

	NEXT_TOKEN(t);

	if (t.tt == TT_OPERATOR) {
		ret = parse_expression();
		CHECK_RESULT(ret);

		ret = parse_assign(t.ot, t.line, mod_pos, id_pos, index_num);
		CHECK_RESULT(ret);
	}
	else if (t.tt == TT_OPEN_PAREN) {
		ADD_OP_LINE(t.line);
		ADD_INS(IT_PUSH_OBJ);
		ADD_INS((uint32_t)mod_pos);
		ADD_INS((uint32_t)id_pos);
		ADD_INS(index_num);

		ret = parse_func_call();
		CHECK_RESULT(ret);

		EXPECT_NEXT_TOKEN(t, TT_SEMICOLON);
		ADD_OP_LINE(t.line);
		ADD_INS(IT_POP);
	}
	else {
		E_rt_err(P.mat, P.file_name, t.line, "Expect operator or \"(\".");
		return -1;
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_right_value(int32_t* mod_pos, int32_t* id_pos, uint32_t* index_num) {
	int ret;
	token_s t;
	EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

	*mod_pos = -1;
	ret = parse_get_obj(t.str, id_pos);
	CHECK_RESULT(ret);

	NEXT_TOKEN(t);

	if (t.tt == TT_COLON) {
		int32_t idx;
		obj_s* o;
		*mod_pos = *id_pos;

		EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

		ret = HL_ref_obj(&P.mod->objs, *mod_pos, &o);
		CHECK_RESULT(ret);
		CHECK_CONDITION(o->type == MAT_OT_MOD);

		idx = HL_get_obj_idx(&o->mod->objs, t.str);

		if (idx < 0) {
			obj_s obj;
			obj.type = MAT_OT_DUMMY;
			idx = HL_set_obj(&o->mod->objs, t.str, &obj);

			if (idx < 0)
				return -1;
		}

		*id_pos = idx;
	}
	else {
		L_unread_token(&t);
	}

	NEXT_TOKEN(t);

	// index. array, dict etc...
	while (t.tt == TT_OPEN_BRACE)  {
		ret = parse_expression();
		CHECK_RESULT(ret);
		EXPECT_NEXT_TOKEN(t, TT_CLOSE_BRACE);
		(*index_num)++;
		NEXT_TOKEN(t);
	}


	L_unread_token(&t);
	ret = 0;
exit0:
	return ret;
}

static int parse_right_identifier() {
	int ret;
	token_s t;
	int32_t mod_pos = 0;
	int32_t id_pos = 0;
	uint32_t index_num = 0;

	ret = parse_right_value(&mod_pos, &id_pos, &index_num);
	CHECK_RESULT(ret);

	NEXT_TOKEN(t);

	if (t.tt == TT_OPEN_PAREN) {
		ADD_OP_LINE(t.line);
		ADD_INS(IT_PUSH_OBJ);
		ADD_INS((uint32_t)mod_pos);
		ADD_INS((uint32_t)id_pos);
		ADD_INS(index_num);
		ret = parse_func_call();
		CHECK_RESULT(ret);
	}
	else {
		ADD_OP_LINE(t.line);
		ADD_INS(IT_PUSH_OBJ);
		ADD_INS((uint32_t)mod_pos);
		ADD_INS((uint32_t)id_pos);
		ADD_INS(index_num);

		L_unread_token(&t);
		return 0;
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_expression() {
	int ret;
	ret = parse_sub_expression();
	CHECK_RESULT(ret);

	while (1) {
		token_s t;
		NEXT_TOKEN(t);

		if (t.tt != TT_OPERATOR ||
		        (t.ot != OT_NOT_EQUAL &&
		         t.ot != OT_EQUAL &&
		         t.ot != OT_LOGICAL_AND &&
		         t.ot != OT_LOGICAL_OR &&
		         t.ot != OT_GREATER &&
		         t.ot != OT_GREATER_EQUAL &&
		         t.ot != OT_LESS &&
		         t.ot != OT_LESS_EQUAL)) {
			L_unread_token(&t);
			return 0;
		}

		ret = parse_sub_expression();
		CHECK_RESULT(ret);

		switch (t.ot) {
			case OT_EQUAL:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_EQ);
				break;

			case OT_NOT_EQUAL:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_NEQ);
				break;

			case OT_LESS:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_LT);
				break;

			case OT_LESS_EQUAL:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_LE);
				break;

			case OT_GREATER:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_GT);
				break;

			case OT_GREATER_EQUAL:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_GE);
				break;

			case OT_LOGICAL_AND:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_LAND);
				break;

			case OT_LOGICAL_OR:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_LOR);
				break;

			default: {
				E_rt_err(P.mat, P.file_name, t.line, "Unknown operator.");
				return -1;
			}
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_sub_expression() {
	int ret;
	ret = parse_term();
	CHECK_RESULT(ret);

	while (1) {
		token_s t;
		NEXT_TOKEN(t);

		if (t.tt != TT_OPERATOR || t.ot != OT_ADD && t.ot != OT_SUB) {
			L_unread_token(&t);
			return 0;
		}

		ret = parse_term();
		CHECK_RESULT(ret);

		switch (t.ot) {
			case OT_ADD:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_ADD);
				break;

			case OT_SUB:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_SUB);
				break;

			default: {
				E_rt_err(P.mat, P.file_name, t.line, "Unknown operator.");
				return -1;
			}
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_term() {
	int ret;
	ret = parse_factor();
	CHECK_RESULT(ret);

	while (1) {
		token_s t;
		NEXT_TOKEN(t);

		if (t.tt != TT_OPERATOR ||
		        (t.ot != OT_MUL &&
		         t.ot != OT_DIV &&
		         t.ot != OT_MOD &&
		         t.ot != OT_EXP &&
		         t.ot != OT_BITWISE_AND &&
		         t.ot != OT_BITWISE_OR &&
		         t.ot != OT_BITWISE_XOR &&
		         t.ot != OT_BITWISE_SHIFT_RIGHT &&
		         t.ot != OT_BITWISE_SHIFT_LEFT)) {
			L_unread_token(&t);
			return 0;
		}

		ret = parse_factor();
		CHECK_RESULT(ret);

		switch (t.ot) {
			case OT_MUL:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_MUL);
				break;

			case OT_DIV:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_DIV);
				break;

			case OT_MOD:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_MOD);
				break;

			case OT_EXP:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_EXP);
				break;

			case OT_BITWISE_AND:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_BAND);
				break;

			case OT_BITWISE_OR:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_BOR);
				break;

			case OT_BITWISE_XOR:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_BXOR);
				break;

			case OT_BITWISE_SHIFT_LEFT:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_SHL);
				break;

			case OT_BITWISE_SHIFT_RIGHT:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_SHR);
				break;

			default: {
				E_rt_err(P.mat, P.file_name, t.line, "Uunknown operator.");
				return -1;
			}
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_make_list() {
	int ret;
	token_s t;
	uint32_t size = 0;

	while (1) {
		NEXT_TOKEN(t);

		if (t.tt == TT_CLOSE_BRACE) {
			ADD_OP_LINE(t.line);
			ADD_INS(IT_MAKE_LIST);
			ADD_INS(size);
			break;
		}
		else if (t.tt == TT_COMMA) {
			ret = parse_expression();
			CHECK_RESULT(ret);

			size++;
		}
		else if (size == 0) {
			L_unread_token(&t);
			ret = parse_expression();
			CHECK_RESULT(ret);

			size++;
		}
		else {
			EXPECT_TOKEN(t, TT_CLOSE_BRACE);
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_make_dict() {
	int ret;
	token_s t;
	uint32_t size = 0;

	while (1) {
		NEXT_TOKEN(t);

		if (t.tt == TT_CLOSE_CURLY_BRACE) {
			ADD_OP_LINE(t.line);
			ADD_INS(IT_MAKE_DICT);
			ADD_INS(size);
			break;
		}
		else if (t.tt == TT_COMMA) {
			ret = parse_expression();
			CHECK_RESULT(ret);

			EXPECT_NEXT_TOKEN(t, TT_COLON);

			ret = parse_expression();
			CHECK_RESULT(ret);
			size++;
		}
		else if (size == 0) {
			L_unread_token(&t);
			ret = parse_expression();
			CHECK_RESULT(ret);

			EXPECT_NEXT_TOKEN(t, TT_COLON);
			ret = parse_expression();

			CHECK_RESULT(ret);
			size++;
		}
		else {
			EXPECT_TOKEN(t, TT_CLOSE_CURLY_BRACE);
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_factor() {
	int ret;
	operator_e unary = OT_NONE;
	token_s t;
	NEXT_TOKEN(t);

	if (t.tt == TT_OPERATOR) {
		switch (t.ot) {
			case OT_BITWISE_NOT:
			case OT_LOGICAL_NOT:
			case OT_INC:
			case OT_DEC:
			case OT_SUB:
			case OT_ADD:
				unary = t.ot;
				NEXT_TOKEN(t);
				break;

			default: {
				E_rt_err(P.mat, P.file_name, t.line, "Expect a unary operator.");
				return -1;
			}
		}
	}

	switch (t.tt) {
		case TT_OPEN_BRACE: {
			ret = parse_make_list();
			CHECK_RESULT(ret);
			break;
		}

		case TT_OPEN_CURLY_BRACE: {
			ret = parse_make_dict();
			CHECK_RESULT(ret);
			break;
		}

		case TT_REV_NONE:
			ADD_OP_LINE(t.line);
			ADD_INS(IT_PUSH_NONE);
			break;

		case TT_CONST_INT: {
			ADD_OP_LINE(t.line);
			ADD_INS(IT_PUSH_INT);
			ADD_INS((uint32_t)t.int32);
			break;
		}

		case TT_CONST_REAL: {
			ADD_OP_LINE(t.line);
			ADD_INS(IT_PUSH_REAL);
			ADD_INS(*(uint32_t*)&t.real);
			break;
		}

		case TT_CONST_STRING: {
			int32_t idx;

			if (unary != OT_NONE)
				return -1;

			ret = S_get_str_idx(&P.mat->strs_nogc, t.str, &idx);
			CHECK_RESULT(ret);

			ADD_OP_LINE(t.line);
			ADD_INS(IT_PUSH_STRING);
			ADD_INS(idx);
			break;
		}

		case TT_OPEN_PAREN:
			ret = parse_expression();
			CHECK_RESULT(ret);

			EXPECT_NEXT_TOKEN(t, TT_CLOSE_PAREN);
			break;

		case TT_IDENTIFIER:
			L_unread_token(&t);
			ret = parse_right_identifier();
			CHECK_RESULT(ret);
			break;

		default: {
			E_rt_err(P.mat, P.file_name, t.line, "Invalid expression.");
			return -1;
		}
	}

	if (unary != OT_NONE) {
		switch (unary) {
			case OT_BITWISE_NOT:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_BNOT);
				break;

			case OT_LOGICAL_NOT:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_LNOT);
				break;

			case OT_INC:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_INC);
				break;

			case OT_DEC:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_DEC);
				break;

			case OT_SUB:
				ADD_OP_LINE(t.line);
				ADD_INS(IT_MINUS);
				break;

			case OT_ADD:
				break;

			default: {
				E_rt_err(P.mat, P.file_name, t.line, "Invalid unary operator.");
				return -1;
			}
		}
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_if() {
	token_s t;
	int ret;
	int last_jmp;
	uint32_t if_exit[MAX_IF_BLOCK_NUM];
	int if_exit_cnt = 0;
	int i;

	ret = parse_expression();
	CHECK_RESULT(ret);

	ADD_INS(IT_FALSE_JMP);
	last_jmp = CUR_CODE_POS;
	ADD_INS(0);

	ret = parse_then();
	CHECK_RESULT(ret);

	TRY_NEXT_TOKEN(t);

	if (t.tt == TT_REV_ELIF || t.tt == TT_REV_ELSE) {
		ADD_OP_LINE(t.line);
		ADD_INS(IT_JMP);
		if_exit[if_exit_cnt++] = CUR_CODE_POS;
		CHECK_CONDITION(if_exit_cnt < MAX_IF_BLOCK_NUM);
		ADD_INS(0);
	}

	while (t.tt == TT_REV_ELIF) {
		SET_INS(last_jmp, CUR_CODE_POS);

		ret = parse_expression();
		CHECK_RESULT(ret);

		ADD_OP_LINE(t.line);
		ADD_INS(IT_FALSE_JMP);
		last_jmp = CUR_CODE_POS;
		ADD_INS(0);

		ret = parse_then();
		CHECK_RESULT(ret);

		NEXT_TOKEN(t);

		if (t.tt == TT_REV_ELIF || t.tt == TT_REV_ELSE) {
			ADD_OP_LINE(t.line);
			ADD_INS(IT_JMP);
			if_exit[if_exit_cnt++] = CUR_CODE_POS;
			CHECK_CONDITION(if_exit_cnt < MAX_IF_BLOCK_NUM);
			ADD_INS(0);
		}
	}

	SET_INS(last_jmp, CUR_CODE_POS);

	if (t.tt == TT_REV_ELSE) {
		ret = parse_then();
		CHECK_RESULT(ret);
	}
	else
		L_unread_token(&t);

	for (i = 0; i < if_exit_cnt; ++i)
		SET_INS(if_exit[i], CUR_CODE_POS);

	ret = 0;
exit0:
	return ret;
}

static int parse_then() {
	int ret;
	token_s t;
	NEXT_TOKEN(t);

	if (t.tt == TT_OPEN_CURLY_BRACE) {
		L_unread_token(&t);

		ret = parse_block(0, 0);
		CHECK_RESULT(ret);
	}
	else {
		L_unread_token(&t);

		ret = parse_statement();
		CHECK_RESULT(ret);
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_block() {
	int ret;
	token_s t;
	EXPECT_NEXT_TOKEN(t, TT_OPEN_CURLY_BRACE);
	NEXT_TOKEN(t);

	while (t.tt != TT_CLOSE_CURLY_BRACE) {
		L_unread_token(&t);

		ret = parse_statement();
		CHECK_RESULT(ret);

		NEXT_TOKEN(t);
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_while() {
	int ret;
	int i;
	int enter_pos, exit_pos;
	parse_loop_s* loop = &P.loops[P.loop_size++];
	CHECK_CONDITION(P.loop_size < MAX_LOOP_NEST);

	memset(loop, 0, sizeof(parse_loop_s));

	enter_pos = CUR_CODE_POS;
	ret = parse_expression();
	CHECK_RESULT(ret);

	ADD_INS(IT_FALSE_JMP);
	exit_pos = CUR_CODE_POS;
	ADD_INS(0);

	ret = parse_then();
	CHECK_RESULT(ret);

	ADD_INS(IT_JMP);
	ADD_INS(enter_pos);

	SET_INS(exit_pos, CUR_CODE_POS);

	for (i = 0; i < loop->enter_size; ++i)
		SET_INS(loop->enter[i], enter_pos);

	for (i = 0; i < loop->exit_size; ++i)
		SET_INS(loop->exit[i], CUR_CODE_POS);

	P.loop_size--;
	ret = 0;
exit0:
	return ret;
}

static int parse_function() {
	int ret;
	token_s t;
	string_s* func_name;
	func_s* func = NULL;
	int32_t idx;
	obj_s obj;
	EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

	func_name = t.str;
	idx = HL_get_obj_direct(&P.mod->objs, func_name, &obj);

	if (idx >= 0 && obj.type != MAT_OT_DUMMY) {
		E_rt_err(P.mat, P.file_name, t.line, "The function name \"%s\" has been used in another place.", func_name->str);
		return -1;
	}

	func = malloc(sizeof(func_s));
	V_PUSH_BACK(P.mod->funcs, func, func_s*);

	obj.type = MAT_OT_FUNC;
	obj.func = func;
	func->name = func_name;
	func->param_num = 0;

	ret = F_init(func);
	CHECK_RESULT(ret);

	idx = HL_set_obj(&P.mod->objs, func_name, &obj);
	CHECK_CONDITION(idx >= 0);

	P.func = func;

	ret = parse_func_param(func);
	CHECK_RESULT(ret);

	ret = parse_block();
	CHECK_RESULT(ret);

	ADD_OP_LINE(t.line);
	ADD_INS(IT_RET);

	ret = 0;
exit0:
	P.func = NULL;
	return ret;
}

static int parse_func_param(func_s* func) {
	token_s t;
	int ret;
	assert(func);

	EXPECT_NEXT_TOKEN(t, TT_OPEN_PAREN);
	NEXT_TOKEN(t);

	if (t.tt == TT_CLOSE_PAREN)
		return 0;

	while (1) {
		int32_t idx;
		obj_s obj;
		obj.type = MAT_OT_DUMMY;

		EXPECT_TOKEN(t, TT_IDENTIFIER);
		NEXT_TOKEN(t);

		idx = HL_set_obj(&func->objs, t.str, &obj);
		CHECK_CONDITION(idx >= 0);

		func->param_num++;

		if (t.tt == TT_CLOSE_PAREN)
			return 0;

		EXPECT_TOKEN(t, TT_COMMA);
		NEXT_TOKEN(t);
	}

	ret = 0;
exit0:
	return ret;
}

static int parse_func_call() {
	int ret;
	token_s t;
	int32_t param_num = 0;

	NEXT_TOKEN(t);

	while (t.tt != TT_CLOSE_PAREN) {
		L_unread_token(&t);

		ret = parse_expression();
		CHECK_RESULT(ret);

		param_num++;

		NEXT_TOKEN(t);

		if (t.tt == TT_COMMA)
			NEXT_TOKEN(t);
	}

	ADD_OP_LINE(t.line);
	ADD_INS(IT_CALL);
	ADD_INS(param_num);

	ret = 0;
exit0:
	return ret;
}

static int parse_import() {
	int ret;
	token_s t;
	obj_s obj;
	uint32_t idx;
	mod_s* mod;

	EXPECT_NEXT_TOKEN(t, TT_CONST_STRING);
	ADD_OP_LINE(t.line);
	ADD_INS(IT_IMPORT);

	ret = S_get_str_idx(&P.mat->strs_nogc, t.str, &idx);
	CHECK_RESULT(ret);

	ADD_INS(idx);

	ret = VM_add_mod(P.mat, t.str, &mod, &idx);
	CHECK_RESULT(ret);

	ret = BU_import(P.mat, mod);
	CHECK_RESULT(ret);

	EXPECT_NEXT_TOKEN(t, TT_REV_AS);
	EXPECT_NEXT_TOKEN(t, TT_IDENTIFIER);

	obj.type = MAT_OT_MOD;
	obj.mod = mod;
	idx = HL_set_obj(&P.mod->objs, t.str, &obj);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

/* method */
int P_parse_file(matrix_t mat, mod_s* mod) {
	int ret = 0;
	assert(mat);
	assert(mod);

	ret = init_parser(mat, mod);
	CHECK_RESULT(ret);

	ret = parse_program();
	CHECK_RESULT(ret);

	ret = OPT_optimize(mod);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	P_clear();
	return ret;
}

int P_parse_str(matrix_t mat, mod_s* mod, const char* str, uint32_t size) {
	int ret;
	assert(mat);
	assert(mod);
	assert(str);

	ret = init_parser_str(mat, mod, str, size);
	CHECK_RESULT(ret);

	ret = parse_program();
	CHECK_RESULT(ret);

	ret = OPT_optimize(mod);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	P_clear();
	return ret;
}

int P_clear() {
	P.mat = NULL;
	P.file_name = NULL;
	P.mod = NULL;
	P.func = NULL;
	memset(P.loops, 0, sizeof(P.loops));
	P.loop_size = 0;
	L_clear();
	return 0;
}

