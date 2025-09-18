/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "ins.h"
#include "vec.h"
#include "str.h"
#include "mod.h"
#include "hash_list.h"

static void show_splitter(MAT_disasm_callback cb) {
	cb("---------------------------------");
}

static void show_obj_list(hash_list_s* hl, MAT_disasm_callback cb) {
	hash_node_s* hash_node = HL_first(hl);

	while (hash_node) {
		int ret;
		char buf[256];
		obj_s obj;
		obj_s* k = &hash_node->key;
		obj_s* v = &hash_node->val;
		assert(k->type == MAT_OT_STR);
		assert(v->type == MAT_OT_INT32);

		ret = HL_get_obj(hl, v->int32, &obj);
		assert(ret == 0);

		sprintf(buf, "%d: %s %s", v->int32, k->str->str, O_type_to_str(obj.type));
		cb(buf);

		hash_node = HL_next(hl, hash_node);
	}
}

static void general_info(mod_s* mod, MAT_disasm_callback cb) {
	uint8_t buf[256];

	show_splitter(cb);
	sprintf(buf, "module: %s", mod->name->str);
	cb(buf);
	sprintf(buf, "obj count: %d", HL_SIZE(mod->objs));
	cb(buf);
	show_splitter(cb);

	show_obj_list(&mod->objs, cb);
	show_splitter(cb);
}

static int disasm(vec_s* code, mat_str_table_s* strs, MAT_disasm_callback cb) {
	uint32_t* ip = &V_AT(*code, 0, uint32_t);
	uint32_t* ipbegin = ip;
	uint32_t* ipend = &V_AT(*code, V_SIZE(*code), uint32_t);
	uint8_t buf[256];

	while (ip < ipend) {
		switch (*ip) {
			case IT_NOP:
				sprintf(buf, "%ld NOP", ip++ - ipbegin);
				break;

			case IT_EXIT:
				sprintf(buf, "%ld EXIT", ip++ - ipbegin);
				break;

			case IT_IMPORT: {
				string_s* str;
				uint32_t idx = *(ip + 1);

				if (S_get_str_by_idx(strs, idx, &str) != 0) {
					sprintf(buf, "ERROR: cannot find string[idx: %d]", idx);
					return -1;
				}

				sprintf(buf, "%ld IMPORT \"%s\"(%d)", ip - ipbegin, str->str, idx);
				ip += 2;
				break;
			}

			case IT_PUSH_NONE:
				sprintf(buf, "%ld IT_PUSH_NONE", ip++ - ipbegin);
				break;

			case IT_PUSH_STRING: {
				uint32_t idx = *(ip + 1);
				string_s* str;

				if (S_get_str_by_idx(strs, idx, &str) != 0) {
					sprintf(buf, "ERROR: cannot find string[idx: %d]", idx);
					return -1;
				}

				sprintf(buf, "%ld PUSH_STRING %s", ip - ipbegin, S_CSTR(str));
				ip += 2;
				break;
			}

			case IT_PUSH_INT:
				sprintf(buf, "%ld PUSH_INT %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_PUSH_REAL:
				sprintf(buf, "%ld PUSH_REAL %f", ip - ipbegin, *(float*)(ip + 1));
				ip += 2;
				break;

			case IT_POP:
				sprintf(buf, "%ld POP", ip++ - ipbegin);
				break;

			case IT_ASSIGN:
				sprintf(buf, "%ld ASSIGN %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_ADD:
				sprintf(buf, "%ld ASSIGN_ADD %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_SUB:
				sprintf(buf, "%ld ASSIGN_SUB %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_MUL:
				sprintf(buf, "%ld ASSIGN_MUL %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_DIV:
				sprintf(buf, "%ld ASSIGN_DIV %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_MOD:
				sprintf(buf, "%ld ASSIGN_MOD %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_EXP:
				sprintf(buf, "%ld ASSIGN_EXP %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_AND:
				sprintf(buf, "%ld ASSIGN_AND %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_OR:
				sprintf(buf, "%ld ASSIGN_OR %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_XOR:
				sprintf(buf, "%ld ASSIGN_XOR %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_SHIFT_LEFT:
				sprintf(buf, "%ld ASSIGN_SHIFT_LEFT %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_ASSIGN_SHIFT_RIGHT:
				sprintf(buf, "%ld ASSIGN_SHIFT_RIGHT %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(ip + 2), *(ip + 3));
				ip += 4;
				break;

			case IT_FALSE_JMP:
				sprintf(buf, "%ld FALSE_JMP %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_TRUE_JMP:
				sprintf(buf, "%ld TRUE_JMP %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_JMP:
				sprintf(buf, "%ld JMP %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_MINUS:
				sprintf(buf, "%ld MINUS", ip - ipbegin);
				ip++;
				break;

			case IT_INC:
				sprintf(buf, "%ld INC", ip - ipbegin);
				ip++;
				break;

			case IT_DEC:
				sprintf(buf, "%ld DEC", ip - ipbegin);
				ip++;
				break;

			case IT_LNOT:
				sprintf(buf, "%ld LNOT", ip - ipbegin);
				ip++;
				break;

			case IT_BNOT:
				sprintf(buf, "%ld BNOT", ip - ipbegin);
				ip++;
				break;

			case IT_ADD:
				sprintf(buf, "%ld ADD", ip - ipbegin);
				ip++;
				break;

			case IT_SUB:
				sprintf(buf, "%ld SUB", ip - ipbegin);
				ip++;
				break;

			case IT_MUL:
				sprintf(buf, "%ld MUL", ip - ipbegin);
				ip++;
				break;

			case IT_DIV:
				sprintf(buf, "%ld DIV", ip - ipbegin);
				ip++;
				break;

			case IT_MOD:
				sprintf(buf, "%ld MOD", ip - ipbegin);
				ip++;
				break;

			case IT_EXP:
				sprintf(buf, "%ld EXP", ip - ipbegin);
				ip++;
				break;

			case IT_BXOR:
				sprintf(buf, "%ld BXOR", ip - ipbegin);
				ip++;
				break;

			case IT_BOR:
				sprintf(buf, "%ld BOR", ip - ipbegin);
				ip++;
				break;

			case IT_BAND:
				sprintf(buf, "%ld BAND", ip - ipbegin);
				ip++;
				break;

			case IT_SHL:
				sprintf(buf, "%ld SHL", ip - ipbegin);
				ip++;
				break;

			case IT_SHR:
				sprintf(buf, "%ld SHR", ip - ipbegin);
				ip++;
				break;

			case IT_LOR:
				sprintf(buf, "%ld LOR", ip - ipbegin);
				ip++;
				break;

			case IT_LAND:
				sprintf(buf, "%ld LAND", ip - ipbegin);
				ip++;
				break;

			case IT_NEQ:
				sprintf(buf, "%ld NEQ", ip - ipbegin);
				ip++;
				break;

			case IT_EQ:
				sprintf(buf, "%ld EQ", ip - ipbegin);
				ip++;
				break;

			case IT_GE:
				sprintf(buf, "%ld GE", ip - ipbegin);
				ip++;
				break;

			case IT_LE:
				sprintf(buf, "%ld LE", ip - ipbegin);
				ip++;
				break;

			case IT_GT:
				sprintf(buf, "%ld GT", ip - ipbegin);
				ip++;
				break;

			case IT_LT:
				sprintf(buf, "%ld LT", ip - ipbegin);
				ip++;
				break;

			case IT_CALL:
				sprintf(buf, "%ld CALL %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_RET:
				sprintf(buf, "%ld RET", ip++ - ipbegin);
				break;

			case IT_RET_RESULT:
				sprintf(buf, "%ld RET_RESULT", ip++ - ipbegin);
				break;

			case IT_MAKE_LIST:
				sprintf(buf, "%ld MAKE_LIST %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_MAKE_DICT:
				sprintf(buf, "%ld MAKE_DICT %d", ip - ipbegin, *(ip + 1));
				ip += 2;
				break;

			case IT_REF_OBJ:
				sprintf(buf, "%ld REF_OBJ %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(int32_t*)(ip + 2), *(ip + 3));
				ip += 3;
				break;

			case IT_PUSH_OBJ:
				sprintf(buf, "%ld PUSH_OBJ %d %d %d", ip - ipbegin, *(int32_t*)(ip + 1), *(int32_t*)(ip + 2), *(ip + 3));
				ip += 3;
				break;

			default:
				sprintf(buf, "ERROR: unknown code[%d]", *ip);
				cb(buf);
				return -1;
		}

		cb(buf);
	}

	return 0;
}

static void show_func(func_s* func, mat_str_table_s* strs, MAT_disasm_callback cb) {
	char txt[256];
	show_splitter(cb);
	sprintf(txt, "function: %s", func->name->str);
	cb(txt);
	show_splitter(cb);
	sprintf(txt, "param num: %d", func->param_num);
	cb(txt);
	show_splitter(cb);
	show_obj_list(&func->objs, cb);
	show_splitter(cb);
	disasm(&func->code, strs, cb);
}

int INS_disasm(mod_s* mod, mat_str_table_s* strs, MAT_disasm_callback cb) {
	uint32_t i;

	general_info(mod, cb);
	disasm(&mod->code, strs, cb);

	for (i = 0; i < HL_SIZE(mod->objs); ++i) {
		obj_s obj;

		if (HL_get_obj(&mod->objs, i, &obj) != 0) {
			assert(0);
			continue;
		}

		if (obj.type == MAT_OT_FUNC) {
			show_func(obj.func, strs, cb);
		}
	}

	return 0;
}

