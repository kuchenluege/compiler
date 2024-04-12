#include <stdlib.h>
#include "compiler/token.h"

char *const RES_WORDS[] =
	{"PROGRAM", "IS", "BEGIN", "END",
	 "GLOBAL", "PROCEDURE", "VARIABLE", "IF",
 	 "THEN", "ELSE", "FOR", "RETURN", "TRUE",
 	 "FALSE", "NOT", "INTEGER", "FLOAT",
 	 "STRING", "BOOL"};

const token_type RW_TOKEN_TYPES[] =
	{T_PROGRAM, T_IS, T_BEGIN, T_END,
	 T_GLOBAL, T_PROCEDURE, T_VARIABLE, T_IF,
 	 T_THEN, T_ELSE, T_FOR, T_RETURN, T_LITERAL,
 	 T_LITERAL, T_NOT, T_TYPE, T_TYPE,
 	 T_TYPE, T_TYPE};

const token_subtype RW_TOKEN_SUBTYPES[] =
	{T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_NONE,
	 T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_NONE,
 	 T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_NONE, T_ST_TRUE,
 	 T_ST_FALSE, T_ST_NONE, T_ST_INTEGER, T_ST_FLOAT,
 	 T_ST_STRING, T_ST_BOOL};

void free_symbol_token(token *tok) {
    if (tok->sym_type == ST_VAR) {
        switch (tok->sym_val_type)
        {
        case SVT_INT:
        case SVT_INT_ARR:
            free(tok->sym_val.int_ptr);
            break;
        case SVT_BOOL:
        case SVT_BOOL_ARR:
            free(tok->sym_val.bool_ptr);
            break;
        case SVT_FLT:
        case SVT_FLT_ARR:
            free(tok->sym_val.float_ptr);
            break;
        case SVT_STR:
        case SVT_STR_ARR:
            free(tok->sym_val.str_ptr);
            break;
        }
    }
    if (tok->sym_type == ST_PROC) {
        free(tok->proc_arg_types);
    }
    free(tok);
}

int is_symbol(token *tok) {
    return tok->sym_type != ST_NONE;
}

char *type_string(symbol_value_type type) {
    switch (type) {
    case SVT_INT:
        return "INTEGER";
    case SVT_INT_ARR:
        return "INTEGER array";
    case SVT_BOOL:
        return "BOOL";
    case SVT_BOOL_ARR:
        return "BOOL array";
    case SVT_FLT:
        return "FLOAT";
    case SVT_FLT_ARR:
        return "FLOAT array";
    case SVT_STR:
        return "STRING";
    case SVT_STR_ARR:
        return "STRING array";
    default:
        return "";
    }
}

symbol_value_type svt_from_literal_value_type(token_subtype lit_val_type) {
    switch (lit_val_type) {
    case T_ST_INT_LIT:
        return SVT_INT;
    case T_ST_TRUE:
    case T_ST_FALSE:
        return SVT_BOOL;
    case T_ST_FLOAT_LIT:
        return SVT_FLT;
    case T_ST_STR_LIT:
        return SVT_STR;
    default:
        return SVT_NONE;
    }
}

symbol_value_type svt_from_type_literal(token_subtype type_lit, int is_array) {
    switch (type_lit) {
    case T_ST_INTEGER:
        if (is_array) return SVT_INT_ARR;
        else return SVT_INT;
    case T_ST_BOOL:
        if (is_array) return SVT_BOOL_ARR;
        else return SVT_BOOL;
    case T_ST_FLOAT:
        if (is_array) return SVT_FLT_ARR;
        else return SVT_FLT;
    case T_ST_STRING:
        if (is_array) return SVT_STR_ARR;
        else return SVT_STR;
    default:
        return SVT_NONE;
    }
}

int is_array_type(symbol_value_type type) {
    return type == SVT_INT_ARR || type == SVT_BOOL_ARR || type == SVT_FLT_ARR || type == SVT_STR_ARR;
}

symbol_value_type type_of_arr_elem(symbol_value_type arr_type) {
    switch (arr_type)
    {
    case SVT_INT_ARR:
        return SVT_INT;
    case SVT_BOOL_ARR:
        return SVT_BOOL;
    case SVT_FLT_ARR:
        return SVT_FLT;
    case SVT_STR_ARR:
        return SVT_STR;
    default:
        return SVT_NONE;
    }
}

int compatible_types(symbol_value_type type1, symbol_value_type type2) {
    return type1 == SVT_BOOL && type2 == SVT_INT ||
           type1 == SVT_INT && type2 == SVT_BOOL ||
           type1 == SVT_INT && type2 == SVT_FLT ||
           type1 == SVT_FLT && type2 == SVT_INT;
}