#ifndef ERROR_H
#define ERROR_H

typedef enum {
    UNRECOGNIZED_TOKEN,
    UNCLOSED_COMMENT,
    TOKEN_TOO_LONG,
    UNCLOSED_STRING,
    EXTRA_DECIMAL_POINT,
    MISSING_TOKEN_FOUND_TOKEN,
    MISSING_TOKEN_FOUND_OTHER,
    MISSING_OTHER_FOUND_TOKEN,
    MISSING_OTHER_FOUND_OTHER,
    DUPLICATE_DECLARATION,
    ILLEGAL_ARRAY_LEN,
    ILLEGAL_ARRAY_INDEX,
    UNDECLARED_SYMBOL,
    NONVAR_ASSMT_DEST,
    INCOMPATIBLE_TYPE_ASSMT,
    NOT_AN_ARRAY,
    NOT_A_PROC,
    UNEXPECTED_TOKEN_IN_PROC_CALL,
    MISSING_ARG,
    INVALID_ARG_TYPE,
    INVALID_OPERAND_TYPE,
    INVALID_OPERAND_TYPES,
    NONBOOL_CONDITION,
    OUT_OF_MEMORY
} error_type;

void log_error(char* file_name, error_type type, int line, ...);

#endif