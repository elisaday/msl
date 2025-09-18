/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#ifndef __H_INS__
#define __H_INS__

#include "matrix.h"
#include "mod.h"

typedef enum {
    // misc
    IT_NOP,
    IT_EXIT,

    // module
    IT_IMPORT,
    IT_IMPORT_DLL,

    // stack
    IT_PUSH_NONE,
    IT_PUSH_STRING,
    IT_PUSH_INT,
    IT_PUSH_REAL,
    IT_POP,
    IT_REF_OBJ,
    IT_PUSH_OBJ,

    // assign
    IT_ASSIGN,
    IT_ASSIGN_ADD,
    IT_ASSIGN_SUB,
    IT_ASSIGN_MUL,
    IT_ASSIGN_DIV,
    IT_ASSIGN_MOD,
    IT_ASSIGN_EXP,
    IT_ASSIGN_AND,
    IT_ASSIGN_OR,
    IT_ASSIGN_XOR,
    IT_ASSIGN_SHIFT_LEFT,
    IT_ASSIGN_SHIFT_RIGHT,

    // jmp
    IT_FALSE_JMP,
    IT_TRUE_JMP,
    IT_JMP,

    // unary operator
    IT_MINUS,
    IT_INC,
    IT_DEC,
    IT_LNOT,
    IT_BNOT,

    // binary operator
    IT_ADD,
    IT_SUB,
    IT_MUL,
    IT_DIV,
    IT_MOD,
    IT_EXP,
    IT_BXOR,
    IT_BOR,
    IT_BAND,
    IT_SHL,
    IT_SHR,

    // logical operator
    IT_LOR,
    IT_LAND,

    // relational operator
    IT_NEQ,
    IT_EQ,
    IT_GE,
    IT_LE,
    IT_GT,
    IT_LT,

    // function call
    IT_CALL,
    IT_RET,
    IT_RET_RESULT,

    // list
    IT_MAKE_LIST,

    // dict
    IT_MAKE_DICT,
} ins_e;

int INS_disasm(mod_s* mod, mat_str_table_s* strs, MAT_disasm_callback cb);

#endif

