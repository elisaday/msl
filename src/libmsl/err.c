/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "matrix.h"
#include "header.h"
#include "err.h"
#include "matrix.h"

#define LOG_BUFFER_SIZE 1024

void E_log(const char* txt, ...) {
#if MATRIX_DEBUG
	char buffer[LOG_BUFFER_SIZE];
	va_list list;
	assert(txt);
	va_start(list, txt);
	vsprintf(buffer, txt, list);
	printf("%s\n", buffer);
	va_end(list);
#endif
}

void E_rt_err(matrix_t mat, const char* file_name, uint32_t line, const char* fmt, ...) {
	char buffer[LOG_BUFFER_SIZE];
	va_list list;
	assert(mat);
	assert(fmt);
	va_start(list, fmt);
	vsprintf(buffer, fmt, list);
	printf("Error: %s\nFile \"%s\", line: %d\n", buffer, file_name, line);
	va_end(list);
	E_stack_trace(mat);
}

void E_stack_trace(matrix_t mat) {

}