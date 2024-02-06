#ifndef TOKEN_H
#define TOKEN_H

// Token Types

// Single character only (never part of a multi-character token)
#define T_PERIOD '.'
#define T_SEMICOLON ';'
#define T_LPAREN '('
#define T_RPAREN ')'
#define T_COMMA ','
#define T_LBRACK '['
#define T_RBRACK ']'
#define T_AND '&'
#define T_OR '|'
#define T_PLUS '+'
#define T_MINUS '-'
#define T_MULT '*'
#define T_DIVIDE '/' // part of comment indicators, but comments aren't tokens

// Multiple* characters (* -> some are single character but are the same as the starting character of another token)
#define T_PROGRAM 256
#define T_IS 257
#define T_BEGIN 258
#define T_END 259
#define T_GLOBAL 260
#define T_PROCEDURE 261
#define T_VARIABLE 262
#define T_IF 263
#define T_THEN 264
#define T_ELSE 265
#define T_RETURN 266
#define T_TRUE 267
#define T_FALSE 268
#define T_NOT 269

#define T_LTHAN 270
#define T_GTHAN 271
#define T_LTEQL 272
#define T_GTEQL 273
#define T_EQLTO 274
#define T_NOTEQ 275

#define T_COLON 276
#define T_ASSMT 277

#define T_IDENT 278
#define T_INTEGER 279
#define T_FLOAT 280
#define T_STRING 281
#define T_BOOL 282

#define T_INT_LIT 283
#define T_FLOAT_LIT 284
#define T_STR_LIT 285

#define T_EOF 286
#define T_UNKNOWN 287

const char *RSVD_WORDS[18] =
	{"PROGRAM", "IS", "BEGIN", "END",
	 "GLOBAL", "PROCEDURE", "VARIABLE", "IF",
 	 "THEN", "ELSE", "RETURN", "TRUE",
 	 "FALSE", "NOT", "INTEGER", "FLOAT",
 	 "STRING", "BOOL"};

const int RW_TOKEN_SUBTYPES[18] =
	{T_PROGRAM, T_IS, T_BEGIN, T_END,
	 T_GLOBAL, T_PROCEDURE, T_VARIABLE, T_IF,
 	 T_THEN, T_ELSE, T_RETURN, T_TRUE,
 	 T_FALSE, T_NOT, T_INTEGER, T_FLOAT,
 	 T_STRING, T_BOOL};

typedef struct {
	int type;
	int subtype; // might use later
	union {
		int int_val;
		int bool_val;
		float flt_val;
		char str_val[256];
	} lex_val;
} token_t;

#endif