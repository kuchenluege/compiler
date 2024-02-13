#ifndef TOKEN_H
#define TOKEN_H

// Token Types

typedef enum token_type {
	T_PERIOD = '.',
	T_SEMICOLON = ';',
	T_COLON = ':',
	T_LPAREN = '(',
	T_RPAREN = ')',
	T_COMMA = ',',
	T_LBRACK = '[',
	T_RBRACK = ']',
	T_PROGRAM,
	T_IS,
	T_BEGIN,
	T_END,
	T_GLOBAL,
	T_PROCEDURE,
	T_VARIABLE,
	T_IF,
	T_THEN,
	T_ELSE,
	T_RETURN,
	T_NOT,
	T_EXPR_OP,
	T_ARITH_OP,
	T_REL_OP,
	T_TERM_OP,
	T_ASSMT,
	T_IDENT,
	T_TYPE,
	T_NUM_LIT,
	T_STR_LIT,
	T_BOOL_LIT,
	T_EOF,
	T_UNKNOWN
} token_type;

typedef enum token_subtype {
	T_ST_NONE = 0,
	T_ST_AND = '&',
	T_ST_OR = '|',
	T_ST_PLUS = '+',
	T_ST_MINUS = '-',
	T_ST_MULT = '*',
	T_ST_DIVIDE = '/',
	T_ST_LTHAN,
	T_ST_GTHAN,
	T_ST_LTEQL,
	T_ST_GTEQL,
	T_ST_EQLTO,
	T_ST_NOTEQ,
	T_ST_INTEGER,
	T_ST_FLOAT,
	T_ST_STRING,
	T_ST_BOOL,
	T_ST_INT_LIT,
	T_ST_FLOAT_LIT,
	T_ST_TRUE,
	T_ST_FALSE
} token_subtype;


const char *RES_WORDS[18] =
	{"PROGRAM", "IS", "BEGIN", "END",
	 "GLOBAL", "PROCEDURE", "VARIABLE", "IF",
 	 "THEN", "ELSE", "RETURN", "TRUE",
 	 "FALSE", "NOT", "INTEGER", "FLOAT",
 	 "STRING", "BOOL"};

const token_type RW_TOKEN_TYPES[18] =
	{T_PROGRAM, T_IS, T_BEGIN, T_END,
	 T_GLOBAL, T_PROCEDURE, T_VARIABLE, T_IF,
 	 T_THEN, T_ELSE, T_RETURN, T_BOOL_LIT,
 	 T_BOOL_LIT, T_NOT, T_TYPE, T_TYPE,
 	 T_TYPE, T_TYPE};

const token_subtype RW_TOKEN_SUBTYPES[18] =
	{T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_NONE,
	 T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_NONE,
 	 T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_TRUE,
 	 T_ST_FALSE, T_ST_NONE, T_ST_INTEGER, T_ST_FLOAT,
 	 T_ST_STRING, T_ST_BOOL};

typedef enum val_tag {
	T_TAG_NONE = 0, T_TAG_INT, T_TAG_BOOL, T_TAG_FLT, T_TAG_STR
} val_tag;

typedef struct {
	token_type type;
	token_subtype subtype;
	val_tag tag;
	union { 
		int int_val;
		int bool_val;
		float flt_val;
		char str_val[256];
	} lex_val;
} token;

#endif