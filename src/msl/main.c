/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifdef WIN32
#define CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <string.h>
#include "matrix.h"

#define MAX_INPUT_BUFF 512
#define PROMPT ">> "
#define PROMPT_UNDONE ".. "

static matrix_t mat;

static void show_info() {
	printf("MSL %s Copyright (c) 2004 Zeb.  All rights reserved.\n", MSL_VER);
}

static int console_loop() {
	int ret;
	char buf[MAX_INPUT_BUFF];
	uint32_t mod_idx;
	size_t size = 0;
	show_info();
	printf("type\"info();\" for more information.\n");

	if (MAT_add_mod(mat, "__main__", &mod_idx) != 0)
		return -1;

	for (;;) {
		if (size == 0)
			fputs(PROMPT, stdout);
		else
			fputs(PROMPT_UNDONE, stdout);

		fflush(stdout);

		if (fgets(buf + size, MAX_INPUT_BUFF - size, stdin) == NULL)
			break;

		size += strlen(buf + size);

		if (size >= MAX_INPUT_BUFF - 1) {
			printf("Input buffer is full.\n");
			size = 0;
			fflush(stdin);
			continue;
		}

		ret = MAT_exec_str(mat, buf, size, mod_idx);

		if (ret != -2)
			size = 0;
	}

	return 0;
}

static void print_usage() {
	show_info();
	printf("usage: msl [options]\n");
	printf("options:\n");
	printf("-h, -H, --help             : show this message.\n");
	printf("--src <source>             : run source file.\n");
	printf("--disasm <source> <output> : disassemble source file.\n");
}

static int run_file(const char* file) {
	uint32_t h_mod;
	return MAT_exec_file(mat, file, &h_mod);
}

static FILE* fp_asm;
void disassemble_output(char* txt) {
	fprintf(fp_asm, "%s\n", txt);
}

static int disasm_file(const char* src, const char* asm_file) {
	fp_asm = fopen(asm_file, "w");
	MAT_disasm(mat, src, disassemble_output);
	fclose(fp_asm);
	return 0;
}

static int parse_option(int argc, char* argv[]) {
	int i;

	if (argc <= 1) {
		console_loop();
		return 0;
	}

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-h") == 0 ||
		        strcmp(argv[i], "--help") == 0 ||
		        strcmp(argv[i], "-H") == 0) {
			print_usage();
			return 0;
		}

		if (strcmp(argv[i], "--src") == 0) {
			char* src;

			if (i + 1 >= argc)
				return -1;

			i++;
			src = argv[i];
			run_file(src);
			return 0;
		}

		if (strcmp(argv[i], "--disasm") == 0) {
			char* src;
			char* asm_file;

			if (i + 2 >= argc)
				return -1;

			i++;
			src = argv[i];
			i++;
			asm_file = argv[i];
			disasm_file(src, asm_file);
			return 0;
		}
	}

	return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(103);
#endif

	if (MAT_init(&mat) != 0) {
		printf("Error: init matrix failed.\n");
		return -1;
	}

	parse_option(argc, argv);
	MAT_free(mat);
	return 0;
}
