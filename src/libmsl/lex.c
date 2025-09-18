/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#include "obj.h"
#include "lex.h"
#include "vec.h"
#include "str.h"
#include "err.h"

typedef int (*pfn_getc)();
typedef int (*pfn_ungetc)(int);

struct lex_state_s {
	matrix_t mat;
	const char* file_name;
	FILE* file;
	const char* str;
	uint32_t str_len;
	uint32_t str_pos;
	uint32_t line;
	vec_s unread; // token_s
	int name_len;
	char name[MAX_IDENTIFIER_LEN];
	pfn_getc getchar;
	pfn_ungetc ungetchar;
	mat_str_table_s* strs;
};

static struct lex_state_s L;

static int file_getc() {
	return getc(L.file);
}

static int file_ungetc(int c) {
	return ungetc(c, L.file);
}

static int str_getc() {
	if (L.str_pos >= L.str_len)
		return EOF;

	return L.str[L.str_pos++];
}

static int str_ungetc(int c) {
	if (L.str_pos > 0) {
		L.str_pos--;
		return c;
	}

	return EOF;
}

#define ADD_CHAR(c) \
	do {\
		if (L.name_len >= MAX_IDENTIFIER_LEN) {\
			E_rt_err(L.mat, L.file_name, L.line, "Identifier is too long. The max length is %d.", MAX_IDENTIFIER_LEN);\
			return -1;\
		}\
		L.name[L.name_len++] = c;\
	} while (0);

static int take_identifier(int alpha, token_s* t) {
	int ret;
	int c = alpha;

	while (c != EOF) {
		ADD_CHAR(c);
		c = L.getchar();

		if (!isalpha(c) && c != '_' && !isdigit(c)) {
			L.ungetchar(c);
			break;
		}
	}

	ADD_CHAR('\0');

	if (strcmp("while", L.name) == 0)
		t->tt = TT_REV_WHILE;
	else if (strcmp("if", L.name) == 0)
		t->tt = TT_REV_IF;
	else if (strcmp("elif", L.name) == 0)
		t->tt = TT_REV_ELIF;
	else if (strcmp("else", L.name) == 0)
		t->tt = TT_REV_ELSE;
	else if (strcmp("def", L.name) == 0)
		t->tt = TT_REV_DEF;
	else if (strcmp("return", L.name) == 0)
		t->tt = TT_REV_RETURN;
	else if (strcmp("break", L.name) == 0)
		t->tt = TT_REV_BREAK;
	else if (strcmp("continue", L.name) == 0)
		t->tt = TT_REV_CONTINUE;
	else if (strcmp("import", L.name) == 0)
		t->tt = TT_REV_IMPORT;
	else if (strcmp("as", L.name) == 0)
		t->tt = TT_REV_AS;
	else if (strcmp("none", L.name) == 0)
		t->tt = TT_REV_NONE;
	else {
		string_s* str;

		ret = S_get_str(L.strs, L.name, &str);
		CHECK_RESULT(ret);

		assert(str);
		t->str = str;
		t->tt = TT_IDENTIFIER;
	}

	ret = 0;
exit0:
	return ret;
}

static int hex2int(int32_t* out) {
	int i;
	unsigned int n = 0;

	for (i = 0; i < L.name_len - 1; ++i) {
		char c = L.name[i];

		if (isdigit(c))
			n = n * 16 + (c - '0');
		else if (c >= 'a' && c <= 'f')
			n = n * 16 + (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			n = n * 16 + (c - 'A' + 10);
		else
			return -1;
	}

	*out = (int32_t)n;
	return 0;
}

static int take_hex(token_s* t) {
	int ret;
	int c = L.getchar();

	for (;;) {
		if (c == EOF)
			break;
		else if (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
			ADD_CHAR(c);
			c = L.getchar();
		}
		else {
			L.ungetchar(c);
			break;
		}
	}

	ADD_CHAR('\0');

	t->tt = TT_CONST_INT;
	ret = hex2int(&t->int32);
	CHECK_RESULT(ret);

	ret = 0;
exit0:
	return ret;
}

static int take_number(int digit, token_s* t) {
	int dot = 0;
	int c = digit;

	assert(t);

	// 处理十六进制
	if (digit == '0') {
		c = L.getchar();

		if (c == 'x' || c == 'X')
			return take_hex(t);
		else {
			L.ungetchar(c);
			c = digit;
		}
	}

	for (;;) {
		ADD_CHAR(c);
		c = L.getchar();

		if (c == EOF)
			break;
		else if (!isdigit(c)) {
			if (!dot && c == '.') {
				dot = 1;
				continue;
			}

			L.ungetchar(c);
			break;
		}
	};

	ADD_CHAR('\0');

	if (dot) {
		t->tt = TT_CONST_REAL;
		t->real = (real_t)atof(L.name);
	}
	else {
		t->tt = TT_CONST_INT;
		t->int32 = (int32_t)atoi(L.name);
	}

	return 0;
}

static int take_const_string(token_s* t) {
	int ret;
	int32_t escape = 0;
	int c;
	assert(t);

	while (1) {
		c = L.getchar();

		if (c == EOF) {
			E_rt_err(L.mat, L.file_name, L.line, "Unfinished string.");
			return -1;
		}

		if (escape) {
			switch (c) {
				case 'n':
					c = '\n';
					escape = 0;
					break;

				case 't':
					c = '\t';
					escape = 0;
					break;

				case '\r':
					c = L.getchar();

					if (c != '\n') {
						E_rt_err(L.mat, L.file_name, L.line, "Error escape.");
						return -1;
					}

				case '\n':
					L.line++;
					escape = 0;
					continue;

				case '\\':
					c = '\\';
					escape = 0;
					break;

				default:
					E_rt_err(L.mat, L.file_name, L.line, "Error escape.");
					return -1;
			}
		}
		else {
			switch (c) {
				case '\r':
				case '\n':
					E_rt_err(L.mat, L.file_name, L.line, "Unfinished string.");
					return -1;

				case '\\':
					escape = 1;
					continue;

				case '"': {
					string_s* str;
					ADD_CHAR('\0');

					ret = S_get_str(L.strs, L.name, &str);
					CHECK_RESULT(ret);

					t->str = str;
					t->tt = TT_CONST_STRING;

					return 0;
				}

				default:
					break;
			}
		}

		ADD_CHAR(c);
	}

exit0:
	return ret;
}

static int take_left_slash(token_s* t) {
	int c = L.getchar();

	if (c == '=') {
		t->tt = TT_OPERATOR;
		t->ot = OT_ASSIGN_DIV;
		return 0;
	}

	if (c == '/') {
		while (1) {
			c = L.getchar();

			if (c == EOF)
				break;
			else if (c == '\n') {
				L.line++;
				t->line++;
				break;
			}
		}

		return 1;
	}
	else if (c == '*') {
		while (1) {
			c = L.getchar();

			if (c == '\n') {
				t->line++;
				L.line++;
			}
			else if (c == EOF) {
				E_rt_err(L.mat, L.file_name, L.line, "Unfinished comment block.");
				return -1;
			}
			else if (c == '*') {
				c = L.getchar();

				if (c == EOF) {
					E_rt_err(L.mat, L.file_name, L.line, "Unfinished comment block.");
					return -1;
				}

				if (c == '/')
					break;
				else
					L.ungetchar(c);
			}
		}

		return 1;
	}
	else {
		t->tt = TT_OPERATOR;
		t->ot = OT_DIV;

		if (c != EOF)
			L.ungetchar(c);
	}

	return 0;
}

/* method */
int L_init() {
	int ret;
	L.file = NULL;
	L.line = 1;

	ret = V_init(&L.unread, sizeof(token_s), 5);
	CHECK_RESULT(ret);

	L.name_len = 0;
	memset(L.name, 0, sizeof(L.name));
	L.getchar = NULL;
	L.ungetchar = NULL;
	L.strs = NULL;

	ret = 0;
exit0:
	return ret;
}

void L_clear() {
	if (L.file) {
		fclose(L.file);
		L.file = NULL;
	}

	V_free(&L.unread);
	L.line = 1;
	L.name[0] = '\0';
	L.name_len = 0;
	L.getchar = NULL;
	L.ungetchar = NULL;
	L.strs = NULL;
}

int L_set_env(matrix_t mat, const char* filename, mat_str_table_s* mat_str_table) {
	int ret;
	char path[MAX_PATH];
	assert(filename);
	assert(mat_str_table);

	sprintf(path, "%s.mat", filename);
	L.file = fopen(path, "rb");
	CHECK_CONDITION(L.file);

	ret = V_init(&L.unread, sizeof(token_s), 5);
	CHECK_RESULT(ret);

	L.mat = mat;
	L.file_name = filename;
	L.str = NULL;
	L.str_pos = 0;
	L.strs = mat_str_table;
	L.line = 1;
	L.getchar = file_getc;
	L.ungetchar = file_ungetc;

	ret = 0;
exit0:
	return ret;
}

int L_set_env_str(matrix_t mat, const char* file_name, const char* str, uint32_t size, mat_str_table_s* mat_str_table) {
	int ret;
	assert(file_name);
	assert(mat_str_table);

	ret = V_init(&L.unread, sizeof(token_s), 5);
	CHECK_RESULT(ret);

	L.mat = mat;
	L.file = NULL;
	L.str = str;
	L.str_len = size;
	L.str_pos = 0;
	L.file_name = file_name;
	L.strs = mat_str_table;
	L.line = 1;
	L.getchar = str_getc;
	L.ungetchar = str_ungetc;

	ret = 0;
exit0:
	return ret;
}

int L_read_token(token_s* t) {
	int ret;
	assert(t);
	assert(L.getchar);
	assert(L.ungetchar);

	if (V_SIZE(L.unread)) {
		*t = V_AT(L.unread, --V_SIZE(L.unread), token_s);
		return 0;
	}

	L.name_len = 0;
	L.name[0] = '\0';

	while (1) {
		int c = L.getchar();
		int alpha = c;
		int digit = c;

		t->line = L.line;

		if (isalpha(c))
			c = 'a';
		else if (isdigit(c))
			c = '1';

		switch (c) {
			case EOF:
				t->tt = TT_STREAM_END;
				return 0;

			case '\n':
				++L.line;
				continue;

			case '\r': {
				int n = L.getchar();
				++L.line;

				if (n != '\n')
					L.ungetchar(n);

				continue;
			}


			case ' ':
			case '\t':
				continue;

			case 'a':
			case '_': {
				ret = take_identifier(alpha, t);
				CHECK_RESULT(ret);

				return 0;
			}

			case '1': {
				ret = take_number(digit, t);
				CHECK_RESULT(ret);

				return 0;
			}

			case '"': {
				ret = take_const_string(t);
				CHECK_RESULT(ret);

				return 0;
			}

			case ':':
				t->tt = TT_COLON;
				return 0;

			case '(':
				t->tt = TT_OPEN_PAREN;
				return 0;

			case ')':
				t->tt = TT_CLOSE_PAREN;
				return 0;

			case '[':
				t->tt = TT_OPEN_BRACE;
				return 0;

			case ']':
				t->tt = TT_CLOSE_BRACE;
				return 0;

			case '{':
				t->tt = TT_OPEN_CURLY_BRACE;
				return 0;

			case '}':
				t->tt = TT_CLOSE_CURLY_BRACE;
				return 0;

			case ',':
				t->tt = TT_COMMA;
				return 0;

			case '.':
				t->tt = TT_POINT;
				return 0;

			case ';':
				t->tt = TT_SEMICOLON;
				return 0;

			case '+':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_ADD;
				else if (c == '+')
					t->ot = OT_INC;
				else {
					t->ot = OT_ADD;

					if (c != EOF)
						L.ungetchar(c);
				}

				return 0;

			case '-':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_SUB;
				else if (c == '-')
					t->ot = OT_DEC;
				else {
					t->ot = OT_SUB;

					if (c != EOF)
						L.ungetchar(c);
				}

				return 0;

			case '*':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_MUL;
				else {
					t->ot = OT_MUL;

					if (c != EOF)
						L.ungetchar(c);
				}

				return 0;

			case '/': {
				ret = take_left_slash(t);

				if (ret == 0)
					return 0;
				else if (ret == 1)
					continue;

				CHECK_RESULT(-1);
			}

			case '&':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_ADD;
				else if (c == '&')
					t->ot = OT_LOGICAL_AND;
				else {
					t->ot = OT_BITWISE_AND;

					if (c != EOF)
						L.ungetchar(c);
				}

				return 0;

			case '!':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_NOT_EQUAL;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_LOGICAL_NOT;
				}

				return 0;

			case '|':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_OR;
				else if (c == '|')
					t->ot = OT_LOGICAL_OR;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_BITWISE_OR;
				}

				return 0;

			case '~':
				t->tt = TT_OPERATOR;
				t->ot = OT_BITWISE_NOT;

				return 0;

			case '#':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_XOR;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_BITWISE_XOR;
				}

				return 0;

			case '^':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_EXP;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_EXP;
				}

				return 0;

			case '%':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_ASSIGN_MOD;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_MOD;
				}

				return 0;

			case '<':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '<') {
					c = L.getchar();

					if (c == '=')
						t->ot = OT_ASSIGN_SHIFT_LEFT;
					else {
						if (c != EOF)
							L.ungetchar(c);

						t->ot = OT_BITWISE_SHIFT_LEFT;
					}
				}
				else if (c == '=')
					t->ot = OT_LESS_EQUAL;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_LESS;
				}

				return 0;

			case '>':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '>') {
					c = L.getchar();

					if (c == '=')
						t->ot = OT_ASSIGN_SHIFT_RIGHT;
					else {
						if (c != EOF)
							L.ungetchar(c);

						t->ot = OT_BITWISE_SHIFT_RIGHT;
					}
				}
				else if (c == '=')
					t->ot = OT_GREATER_EQUAL;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_GREATER;
				}

				return 0;

			case '=':
				t->tt = TT_OPERATOR;
				c = L.getchar();

				if (c == '=')
					t->ot = OT_EQUAL;
				else {
					if (c != EOF)
						L.ungetchar(c);

					t->ot = OT_ASSIGN;
				}

				return 0;

			default:
				E_rt_err(L.mat, L.file_name, L.line, "Unrecognize symbol: %c(0x%x).", c, c);
				return -1;
		}
	}

	ret = 0;
exit0:
	return ret;
}

int L_unread_token(token_s* t) {
	assert(t);
	V_PUSH_BACK(L.unread, *t, token_s);
	return 0;
}

const char* L_token_to_string(token_s* t) {
	static char buf[128];

	switch (t->tt) {
		case TT_STREAM_END:
			return "end of stream";

		case TT_REV_IF:
			return "[if]";

		case TT_REV_WHILE:
			return "[while]";

		case TT_REV_ELIF:
			return "[elif]";

		case TT_REV_ELSE:
			return "[else]";

		case TT_REV_DEF:
			return "[def]";

		case TT_REV_RETURN:
			return "[return]";

		case TT_REV_BREAK:
			return "[break]";

		case TT_REV_CONTINUE:
			return "[continue]";

		case TT_REV_IMPORT:
			return "[import]";

		case TT_REV_NONE:
			return "[none]";

		case TT_COMMA:               // ,
			return "[,]";

		case TT_POINT:               // .
			return "[.]";

		case TT_SEMICOLON:           // ;
			return "[;]";

		case TT_OPEN_PAREN:          // (
			return "[(]";

		case TT_CLOSE_PAREN:         // )
			return "[)]";

		case TT_OPEN_BRACE:          // [
			return "[[]";

		case TT_CLOSE_BRACE:         // ]
			return "[]]";

		case TT_OPEN_CURLY_BRACE:    // {
			return "[{]";

		case TT_CLOSE_CURLY_BRACE:   // }
			return "[}]";

		case TT_IDENTIFIER:
			return S_CSTR(t->str);

		case TT_CONST_INT: {
			sprintf(buf, "%d", t->int32);
			return buf;
		}

		case TT_CONST_REAL: {
			sprintf(buf, "%f", t->real);
			return buf;
		}

		case TT_CONST_STRING:
			return S_CSTR(t->str);

		case TT_OPERATOR: {
			switch (t->ot) {
				case OT_ADD:
					return "[operator +]";

				case OT_SUB:
					return "[operator -]";

				case OT_BITWISE_NOT:
					return "[operator ~]";

				case OT_LOGICAL_NOT:
					return "[operator !]";

				case OT_INC:
					return "[operator ++]";

				case OT_DEC:
					return "[operator --]";

				case OT_MUL:
					return "[operator *]";

				case OT_DIV:
					return "[operator /]";

				case OT_MOD:
					return "[operator %]";

				case OT_EXP:
					return "[operator ^]";

				case OT_BITWISE_AND:
					return "[operator &]";

				case OT_BITWISE_OR:
					return "[operator |]";

				case OT_BITWISE_XOR:
					return "[operator #]";

				case OT_BITWISE_SHIFT_LEFT:
					return "[operator <<]";

				case OT_BITWISE_SHIFT_RIGHT:
					return "[operator >>]";

				case OT_EQUAL:
					return "[operator ==]";

				case OT_NOT_EQUAL:
					return "[operator !=]";

				case OT_LESS:
					return "[operator <]";

				case OT_LESS_EQUAL:
					return "[operator <=]";

				case OT_GREATER:
					return "[operator >]";

				case OT_GREATER_EQUAL:
					return "[operator >=]";

				case OT_LOGICAL_AND:
					return "[operator &&]";

				case OT_LOGICAL_OR:
					return "[operator ||]";

				case OT_ASSIGN:
					return "[operator =]";

				case OT_ASSIGN_ADD:
					return "[operator +=]";

				case OT_ASSIGN_SUB:
					return "[operator -=]";

				case OT_ASSIGN_MUL:
					return "[operator *=]";

				case OT_ASSIGN_DIV:
					return "[operator /=]";

				case OT_ASSIGN_MOD:
					return "[operator %=]";

				case OT_ASSIGN_EXP:
					return "[operator ^=]";

				case OT_ASSIGN_AND:
					return "[operator &=]";

				case OT_ASSIGN_OR:
					return "[operator |=]";

				case OT_ASSIGN_XOR:
					return "[operator #=]";

				case OT_ASSIGN_SHIFT_LEFT:
					return "[operator <<=]";

				case OT_ASSIGN_SHIFT_RIGHT:
					return "[operator >>=]";

				default:
					return "unknown token";
			}
		}
		break;

		default:
			return "unknown token";
	}

	return "";
}
