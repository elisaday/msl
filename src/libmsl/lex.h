/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/

#ifndef __H_LEX__
#define __H_LEX__

#include "str.h"

typedef enum {
    TT_STREAM_END = 0,

    TT_REV_IF,
    TT_REV_WHILE,
    TT_REV_ELIF,
    TT_REV_ELSE,
    TT_REV_DEF,
    TT_REV_RETURN,
    TT_REV_BREAK,
    TT_REV_CONTINUE,
    TT_REV_IMPORT,
    TT_REV_AS,
    TT_REV_NONE,

    TT_COMMA,               // ,
    TT_POINT,               // .
    TT_COLON,               // :
    TT_SEMICOLON,           // ;

    TT_OPEN_PAREN,          // (
    TT_CLOSE_PAREN,         // )
    TT_OPEN_BRACE,          // [
    TT_CLOSE_BRACE,         // ]
    TT_OPEN_CURLY_BRACE,    // {
    TT_CLOSE_CURLY_BRACE,   // }

    TT_IDENTIFIER,

    TT_CONST_INT,
    TT_CONST_REAL,
    TT_CONST_STRING,

    TT_OPERATOR,
} token_type_e;

typedef enum {
    OT_NONE = 0,

    // unary operator
    OT_ADD,                     // +
    OT_SUB,                     // -
    OT_BITWISE_NOT,             // ~
    OT_LOGICAL_NOT,             // !
    OT_INC,                     // ++
    OT_DEC,                     // --

    // binary operator OT_ADD, OT_SUB
    OT_MUL,                     // *
    OT_DIV,                     // /
    OT_MOD,                     // %
    OT_EXP,                     // ^
    OT_BITWISE_AND,             // &
    OT_BITWISE_OR,              // |
    OT_BITWISE_XOR,             // #
    OT_BITWISE_SHIFT_LEFT,      // <<
    OT_BITWISE_SHIFT_RIGHT,     // >>

    // relational operator
    OT_EQUAL,                   // ==
    OT_NOT_EQUAL,               // !=
    OT_LESS,                    // <
    OT_LESS_EQUAL,              // <=
    OT_GREATER,                 // >
    OT_GREATER_EQUAL,           // >=
    OT_LOGICAL_AND,             // &&
    OT_LOGICAL_OR,              // ||

    // assign operator */
    OT_ASSIGN,                  // =
    OT_ASSIGN_ADD,              // +=
    OT_ASSIGN_SUB,              // -=
    OT_ASSIGN_MUL,              // *=
    OT_ASSIGN_DIV,              // /=
    OT_ASSIGN_MOD,              // %=
    OT_ASSIGN_EXP,              // ^=
    OT_ASSIGN_AND,              // &=
    OT_ASSIGN_OR,               // |=
    OT_ASSIGN_XOR,              // #=
    OT_ASSIGN_SHIFT_LEFT,       // <<=
    OT_ASSIGN_SHIFT_RIGHT,      // >>=
} operator_e;

typedef struct token_s {
	token_type_e tt;
	operator_e ot;
	uint32_t line;
	union {
		real_t real;
		int32_t int32;
		string_s* str;
	};
} token_s;

int L_init();
void L_clear();
int L_set_env(matrix_t mat, const char* filename, mat_str_table_s* mat_str_table);
int L_set_env_str(matrix_t mat, const char* file_name, const char* str, uint32_t size, mat_str_table_s* mat_str_table);
int L_read_token(token_s* t);
int L_unread_token(token_s* t);
const char* L_token_to_string(token_s* t);

#endif

