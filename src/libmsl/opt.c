/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "opt.h"
#include "err.h"
#include "ins.h"
#include "vec.h"

static int try_assign(uint32_t** ip, uint32_t* ip_end) {
	// uint32_t* p = *ip;
	// uint32_t push_op, sth, assign, id_pos, index_num;

	// if (ip_end - p < 5)
	//  return 0;

	// push_op = *p;
	// sth = *(p + 1);
	// assign = *(p + 2);
	// id_pos = *(p + 3);
	// index_num = *(p + 4);

	// if (assign == IT_ASSIGN)
	// switch (push_op) {
	//  case IT_PUSH_NONE:
	//      *p++ = IT_ASSIGN_NONE;
	//      break;

	//  case IT_PUSH_INT:
	//      *p++ = IT_ASSIGN_INT;
	//      break;

	//  case IT_PUSH_STRING:
	//      *p++ = IT_ASSIGN_STRING;
	//      break;

	//  case IT_PUSH_REAL:
	//      *p++ = IT_ASSIGN_REAL;
	//      break;

	//  default:
	//      return 0;
	// }

	// *p++ = sth;
	// *p++ =

	// {
	//  *p++ = ref_local_idx;
	//  *p++ = op_idx;
	//  *p++ = IT_NOP;
	//  *p++ = IT_NOP;
	//  *ip += 5;
	//  return 1;
	// }

	return 0;
}

static int optimize(vec_s* code) {
	// uint32_t* ip = &V_AT(*code, 0, uint32_t);
	// uint32_t* ip_begin = ip;
	// uint32_t* ip_end = &V_AT(*code, V_SIZE(*code), uint32_t);

	// while (ip < ip_end) {
	//  switch (*ip) {
	//      case IT_PUSH_NONE:
	//      case IT_PUSH_INT:
	//      case IT_PUSH_REAL:
	//      case IT_PUSH_STRING:
	//          if (try_assign(&ip, ip_end))
	//              continue;

	//          break;

	//      case IT_PUSH_OBJ:
	//          if (try_assign_obj(&ip, ip_end))
	//              continue;

	//          break;

	//      default:
	//          break;
	//  }

	//  ip++;
	// }

	return 0;
}

int OPT_optimize(mod_s* mod) {
	int ret;
	uint32_t i;
	ret = optimize(&mod->code);
	CHECK_RESULT(ret);

	for (i = 0; i < V_SIZE(mod->funcs); i++) {
		func_s* func = V_AT(mod->funcs, i, func_s*);
		ret = optimize(&func->code);
		CHECK_RESULT(ret);
	}

	ret = 0;
exit0:
	return ret;
}
