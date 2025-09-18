/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#include "obj.h"
#include "mod.h"
#include "vec.h"
#include "hash_list.h"
#include "str.h"
#include "err.h"
#include "func.h"

int MOD_init(mod_s* mod, string_s* name) {
	int ret;
	mod->handle = 0;

	ret = V_init(&mod->code, sizeof(uint32_t), DEFAULT_MOD_CODE_SIZE);
	CHECK_RESULT(ret);

	ret = V_init(&mod->op_line, sizeof(op_line_s), DEFAULT_MOD_CODE_SIZE);
	CHECK_RESULT(ret);

	ret = V_init(&mod->funcs, sizeof(func_s*), DEFAULT_MOD_FUNC_NUM);
	CHECK_RESULT(ret);

	ret = HL_init(&mod->objs, DEFAULT_MOD_OBJ_SIZE);
	CHECK_RESULT(ret);

	mod->name = name;
	mod->init = 0;

	ret = 0;
exit0:
	return ret;
}

int MOD_init_dll(matrix_t mat, mod_s* mod, string_s* name) {
	int ret;
	char path[MAX_PATH];
	typedef int (*PFN_IMPORT)(matrix_t mat, mat_mod_t mod);
	PFN_IMPORT pfn;

	ret = V_init(&mod->code, sizeof(uint32_t), 0);
	CHECK_RESULT(ret);

	ret = V_init(&mod->op_line, sizeof(op_line_s), 0);
	CHECK_RESULT(ret);

	ret = V_init(&mod->funcs, sizeof(func_s*), 0);
	CHECK_RESULT(ret);

	ret = HL_init(&mod->objs, DEFAULT_MOD_OBJ_SIZE);
	CHECK_RESULT(ret);

	mod->name = name;

#if defined(PLATFORM_WINDOWS)
	sprintf(path, "%s.dll", name->str);
	mod->handle = LoadLibrary(path);

	if (!mod->handle) {
		E_log("Load dll \"%s\" failed", name->str);
		return -1;
	}

	pfn = (PFN_IMPORT)GetProcAddress(mod->handle, "import");

	if (!pfn) {
		E_log("Cannot find \"import\" in dll \"%s\"", name->str);
		return -1;
	}

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	sprintf(path, "%s.so", name->str);
	mod->handle = dlopen(path, RTLD_NOW);

	if (!mod->handle) {
		E_log("Load dll \"%s\" failed", name->str);
		return -1;
	}

	pfn = (PFN_IMPORT)dlsym(mod->handle, "import");

	if (!pfn) {
		char* err = dlerror();
		E_log("Cannot find \"import\" in dll \"%s\". Reason: %s", name->str, err);
		return -1;
	}

#endif

	ret = pfn(mat, mod);
	CHECK_RESULT(ret);

	mod->init = 1;
	ret = 0;
exit0:
	return ret;
}

void MOD_free(mod_s* mod) {
	uint32_t i;
	typedef int (*PFN_CLOSE)(mat_mod_t mod);
	PFN_CLOSE pfn = NULL;
	assert(mod);

#if defined(PLATFORM_WINDOWS)
	pfn = (PFN_CLOSE)GetProcAddress(mod->handle, "close");

	if (pfn)
		pfn(mod);

	FreeLibrary(mod->handle);
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
	pfn = (PFN_CLOSE)dlsym(mod->handle, "close");

	if (pfn)
		pfn(mod);

	dlclose(mod->handle);
#endif

	for (i = 0; i < V_SIZE(mod->funcs); ++i) {
		func_s* func = V_AT(mod->funcs, i, func_s*);
		F_free(func);
		free(func);
	}

	V_free(&mod->funcs);
	V_free(&mod->op_line);
	V_free(&mod->code);
	HL_free(&mod->objs);
}

int MOD_get_obj_by_idx(mod_s* mod, uint32_t idx, obj_s* obj) {
	assert(mod);
	assert(obj);

	return HL_get_obj(&mod->objs, idx, obj);
}

int MOD_get_obj_idx(mod_s* mod, string_s* name, uint32_t* idx) {
	int32_t n;
	assert(mod);
	assert(name);
	assert(idx);

	n = HL_get_obj_idx(&mod->objs, name);

	if (n < 0)
		return -1;

	*idx = n;
	return 0;
}

int MOD_get_obj(mod_s* mod, string_s* name, obj_s* obj) {
	int ret;
	ret = HL_get_obj_direct(&mod->objs, name, obj);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}