/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_ERROR__
#define __H_ERROR__

#include "header.h"

#define CHECK_MALLOC(p)\
	do {\
		if (!(p)) {\
			E_log("Out of memory.");\
			exit(0);\
		}\
	} while (0);

#define CHECK_RESULT(ret)\
	do {\
		if ((ret) < 0) {\
			E_log("File: [%s] line: [%d] ecode: [%d]", __FILE__, __LINE__, (ret));\
			goto exit0;\
		}\
	} while (0);

#define CHECK_CONDITION(cond)\
	do {\
		if (!(cond)) {\
			ret = -1;\
			E_log("File: [%s] line: [%d] esscode: [%d]", __FILE__, __LINE__, (cond));\
			goto exit0;\
		}\
	} while (0);

void E_log(const char* txt, ...);
void E_rt_err(matrix_t mat, const char* file_name, uint32_t line, const char* fmt, ...);
void E_stack_trace(matrix_t mat);

#endif


