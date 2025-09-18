/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "vec.h"
#include "str.h"
#include "err.h"
#include "hash.h"
#include "list.h"

static int mul_int(obj_s* o1, obj_s* o2, obj_s* r) {
	r->type = MAT_OT_INT32;
	r->int32 = o1->int32 * o2->int32;
	return 0;
}

static int mul_real(obj_s* o1, obj_s* o2, obj_s* r) {
	real_t f1, f2;

	if (O_to_real(o1, &f1) != 0 || O_to_real(o2, &f2) != 0)
		return -1;

	r->type = MAT_OT_REAL;
	r->real = f1 * f2;
	return 0;
}

static int mul_list(matrix_t mat, obj_s* o1, obj_s* o2, obj_s* r) {
	list_s* l;
	uint32_t n = 0 < o2->int32 ? o2->int32 : 0; // max(0, o2->int32);
	uint32_t size = V_SIZE(o1->list->v);
	uint32_t i, j;
	LI_alloc(mat, size * n, &l);

	for (i = 0; i < n; ++i) {
		for (j = 0; j < size; ++j) {
			V_AT(l->v, i * size + j, obj_s) = V_AT(o1->list->v, j, obj_s);
		}
	}

	V_SIZE(l->v) = size * n;
	r->type = MAT_OT_LIST;
	r->list = l;
	return 0;
}

int O_compare_eq(obj_s* o1, obj_s* o2) {
	if (o1->type != o2->type) {
		return 0;
	}

	switch (o1->type) {
		case MAT_OT_NONE:
			return 1;

		case MAT_OT_INT32:
			return o1->int32 == o2->int32;

		case MAT_OT_REAL:
			return o1->real == o2->real;

		case MAT_OT_STR:
			return S_compare_eq(o1->str, o2->str);

		default:
			return -1;
	}

	return 0;
}

int O_compare_gt(obj_s* o1, obj_s* o2) {
	if (o1->type != o2->type)
		return -1;

	switch (o1->type) {
		case MAT_OT_INT32:
			return o1->int32 > o2->int32;

		case MAT_OT_REAL:
			return o1->real > o2->real;

		default:
			return -1;
	}

	assert(0);
	return -1;
}

int O_compare_ge(obj_s* o1, obj_s* o2) {
	if (o1->type != o2->type)
		return -1;

	switch (o1->type) {
		case MAT_OT_INT32:
			return o1->int32 >= o2->int32;

		case MAT_OT_REAL:
			return o1->real >= o2->real;

		default:
			return -1;
	}

	assert(0);
	return -1;
}

int O_dump(obj_s* obj) {
	switch (obj->type) {
		case MAT_OT_DUMMY:
			printf("dummy object\n");
			break;

		case MAT_OT_NONE:
			printf("none\n");
			break;

		case MAT_OT_INT32:
			printf("%d\t[int]\n", obj->int32);
			break;

		case MAT_OT_REAL:
			printf("%f\t[real]\n", obj->real);
			break;

		case MAT_OT_STR:
			printf("%s\t[string]\n", obj->str->str);
			break;

		case MAT_OT_FUNC:
			printf("%s\t[function]\n", obj->func->name->str);
			break;

		case MAT_OT_C_FUNC:
			printf("%ld\t[c function]\n", obj->c_func);
			break;

		case MAT_OT_OBJ_REF:
			printf("[ref] => %d\n", obj->obj_ref->type);
			//O_dump(obj->obj_ref);
			break;

		case MAT_OT_LIST:
			printf("%ld\t[list]\n", obj->list);
			break;

		case MAT_OT_DICT:
			printf("%ld\t[dict]\n", obj->dict);
			break;

		default: {
			printf("invalid object.\n");
			return -1;
		}
	}

	return 0;
}

int O_print(obj_s* obj, int quotes) {
	switch (obj->type) {
		case MAT_OT_NONE:
			printf("none");
			break;

		case MAT_OT_INT32:
			printf("%d", obj->int32);
			break;

		case MAT_OT_REAL:
			printf(REAL_FMT, obj->real);
			break;

		case MAT_OT_STR:
			if (quotes)
				printf("\"%s\"", obj->str->str);
			else
				printf("%s", obj->str->str);

			break;

		case MAT_OT_FUNC:
			printf(" < func % s > ", obj->func->name->str);
			break;

		case MAT_OT_C_FUNC:
			printf(" < c_func % ld > ", obj->c_func);
			break;

		case MAT_OT_LIST: {
			uint32_t i = 0;
			printf("[");

			if (V_SIZE(obj->list->v) > 0) {
				if (V_SIZE(obj->list->v) > 1) {
					for (i = 0; i < V_SIZE(obj->list->v) - 1; ++i) {
						O_print(&V_AT(obj->list->v, i, obj_s), 1);
						printf(", ");
					}
				}

				O_print(&V_AT(obj->list->v, i, obj_s), 1);
			}

			printf("]");
			break;
		}

		case MAT_OT_DICT: {
			hash_node_s* node = H_first(&obj->dict->h);
			printf("{");

			while (node) {
				O_print(&node->key, 1);
				printf(": ");
				O_print(&node->val, 1);
				node = H_next(&obj->dict->h, node);

				if (node)
					printf(", ");
			}

			printf("}");
			break;
		}

		case MAT_OT_DUMMY:
			printf("Cannot print <dummy object>");
			break;

		default: {
			printf("invalid object.\n");
			return -1;
		}
	}

	return 0;
}

const char* O_type_to_str(mat_obj_type_e type) {
	switch (type) {
		case MAT_OT_NONE:
			return " < type 'none' > ";

		case MAT_OT_INT32:
			return " < type 'int32' > ";

		case MAT_OT_REAL:
			return " < type 'real' > ";

		case MAT_OT_STR:
			return " < type 'string' > ";

		case MAT_OT_FUNC:
			return " < type 'function' > ";

		case MAT_OT_C_FUNC:
			return " < type 'c function' > ";

		case MAT_OT_MOD:
			return " < type 'mod' > ";

		case MAT_OT_OBJ_REF:
			return " < type 'obj ref' > ";

		case MAT_OT_DUMMY:
			return " < dummy object > ";

		default:
			return "<type unknown>";
	}
}

int O_to_str(matrix_t mat, obj_s* o, string_s** s) {
	int ret;

	if (o->type == MAT_OT_STR) {
		*s = o->str;
		return 0;
	}
	else if (o->type == MAT_OT_INT32) {
		ret = S_create_int(&mat->strs_gc, o->int32, s);
		CHECK_RESULT(ret);
		return 0;
	}
	else if (o->type == MAT_OT_REAL) {
		ret = S_create_real(&mat->strs_gc, o->real, s);
		CHECK_RESULT(ret);
		return 0;
	}

	return -1;
exit0:
	return -1;
}

int O_to_real(obj_s* o, real_t* r) {

	if (o->type == MAT_OT_INT32) {
		*r = (real_t)o->int32;
		return 0;
	}
	else if (o->type == MAT_OT_REAL) {
		*r = o->real;
		return 0;
	}
	else if (o->type == MAT_OT_STR) {
		*r = (real_t)atof(o->str->str);
		return 0;
	}

	return -1;
}

int O_to_int32(obj_s* o, int32_t* r) {
	if (o->type == MAT_OT_INT32) {
		*r = o->int32;
		return 0;
	}
	else if (o->type == MAT_OT_REAL) {
		*r = (int32_t)o->real;
		return 0;
	}
	else if (o->type == MAT_OT_STR) {
		*r = atoi(o->str->str);
		return 0;
	}

	return -1;
}

int O_add(matrix_t mat, obj_s* o1, obj_s* o2, obj_s* r) {
	int ret;

	if (o1->type == MAT_OT_INT32 && o2->type == MAT_OT_INT32) {
		r->type = MAT_OT_INT32;
		r->int32 = o1->int32 + o2->int32;
		return 0;
	}
	else if (o1->type == MAT_OT_REAL || o2->type == MAT_OT_REAL) {
		real_t f1, f2;

		if (O_to_real(o1, &f1) != 0 || O_to_real(o2, &f2) != 0)
			return -1;

		r->type = MAT_OT_REAL;
		r->real = f1 + f2;
		return 0;
	}
	else if (o1->type == MAT_OT_STR && o2->type == MAT_OT_STR) {
		ret = S_conc_str(&mat->strs_gc, S_CSTR(o1->str), S_CSTR(o2->str), &r->str);
		CHECK_RESULT(ret);

		r->type = MAT_OT_STR;
		return 0;
	}

exit0:
	return -1;
}

int O_sub(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type == MAT_OT_INT32 || o2->type == MAT_OT_INT32) {
		r->type = MAT_OT_INT32;
		r->int32 = o1->int32 - o2->int32;
		return 0;
	}
	else if (o1->type == MAT_OT_REAL || o2->type == MAT_OT_REAL) {
		real_t f1, f2;

		if (O_to_real(o1, &f1) != 0 || O_to_real(o2, &f2) != 0)
			return -1;

		r->type = MAT_OT_REAL;
		r->real = f1 - f2;
		return 0;
	}

	return -1;
}

int O_mul(matrix_t mat, obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type == MAT_OT_INT32) {
		if (o2->type == MAT_OT_INT32)
			return mul_int(o1, o2, r);
		else if (o2->type == MAT_OT_REAL)
			return mul_real(o1, o2, r);
		else if (o2->type == MAT_OT_LIST)
			return mul_list(mat, o2, o1, r);
	}
	else if (o1->type == MAT_OT_REAL) {
		if (o2->type == MAT_OT_INT32)
			return mul_real(o1, o2, r);
		else if (o2->type == MAT_OT_REAL)
			return mul_real(o1, o2, r);
	}
	else if (o1->type == MAT_OT_LIST) {
		if (o2->type == MAT_OT_INT32)
			return mul_list(mat, o1, o2, r);
	}

	return -1;
}

int O_div(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type == MAT_OT_INT32 || o2->type == MAT_OT_INT32) {
		r->type = MAT_OT_INT32;
		r->int32 = o1->int32 / o2->int32;
		return 0;
	}
	else if (o1->type == MAT_OT_REAL || o2->type == MAT_OT_REAL) {
		real_t f1, f2;

		if (O_to_real(o1, &f1) != 0 || O_to_real(o2, &f2) != 0)
			return -1;

		r->type = MAT_OT_REAL;
		r->real = f1 / f2;
		return 0;
	}

	return -1;
}

int O_exp(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type == MAT_OT_INT32) {
		int32_t x = o1->int32;

		if (o2->type == MAT_OT_INT32) {
			int32_t y = o2->int32;
			r->type = MAT_OT_INT32;
			r->int32 = (int32_t)pow((real_t)x, (real_t)y);
			return 0;
		}
		else if (o2->type == MAT_OT_REAL) {
			real_t y = o2->real;
			r->type = MAT_OT_REAL;
			r->real = (real_t)pow((real_t)x, y);
			return 0;
		}
	}
	else if (o1->type = MAT_OT_REAL) {
		real_t x = (real_t)o1->real;

		if (o2->type == MAT_OT_INT32) {
			int32_t y = o2->int32;
			r->type = MAT_OT_REAL;
			r->real = (real_t)pow(x, (real_t)y);
			return 0;
		}
		else if (o2->type == MAT_OT_REAL) {
			real_t y = o2->real;
			r->type = MAT_OT_REAL;
			r->real = (real_t)pow(x, y);
			return 0;
		}
	}

	return -1;
}

int O_shift_left(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type = MAT_OT_INT32 && o2->type == MAT_OT_INT32) {
		int32_t n1 = o1->int32;
		int32_t n2 = o2->int32;
		r->type = MAT_OT_INT32;
		r->int32 = n1 << n2;
		return 0;
	}

	return -1;
}

int O_shift_right(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type = MAT_OT_INT32 && o2->type == MAT_OT_INT32) {
		int32_t n1 = o1->int32;
		int32_t n2 = o2->int32;
		r->type = MAT_OT_INT32;
		r->int32 = n1 >> n2;
		return 0;
	}

	return -1;
}

int O_bitwise_or(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type = MAT_OT_INT32 && o2->type == MAT_OT_INT32) {
		uint32_t n1 = (uint32_t)o1->int32;
		uint32_t n2 = (uint32_t)o2->int32;
		r->type = MAT_OT_INT32;
		r->int32 = (int32_t)(n1 | n2);
		return 0;
	}

	return -1;
}

int O_bitwise_and(obj_s* o1, obj_s* o2, obj_s* r) {
	if (o1->type = MAT_OT_INT32 && o2->type == MAT_OT_INT32) {
		uint32_t n1 = (uint32_t)o1->int32;
		uint32_t n2 = (uint32_t)o2->int32;
		r->type = MAT_OT_INT32;
		r->int32 = (int32_t)(n1 & n2);
		return 0;
	}

	return -1;
}
