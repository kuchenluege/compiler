#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler/error.h"

void print_error(char* file_name, error_type type, int line, ...) {
	va_list args;
    va_start(args, line);

	printf("%s:%d: error: ", file_name, line);
	switch (type)
	{
	case UNRECOGNIZED_TOKEN:
		vprintf("unrecognized token '%s'", args);
		break;
	case UNCLOSED_COMMENT:
		printf("unterminated comment");
		break;
    case TOKEN_TOO_LONG:
        vprintf("%s longer than 255 characters", args);
        break;
	case UNCLOSED_STRING:
		printf("mising terminating \" character");
		break;
	case EXTRA_DECIMAL_POINT:
		printf("too many decimal points in number");
		break;
	case MISSING_TOKEN_FOUND_TOKEN:
		vprintf("expected '%s' before '%s'", args);
		break;
    case MISSING_TOKEN_FOUND_OTHER:
		vprintf("expected '%s' before %s", args);
		break;
    case MISSING_OTHER_FOUND_TOKEN:
        vprintf("expected %s before '%s'", args);
        break;
    case MISSING_OTHER_FOUND_OTHER:
        vprintf("expected %s before %s", args);
        break;
    case DUPLICATE_DECLARATION:
        vprintf("duplicate declaration of symbol '%s'", args);
        break;
    case ILLEGAL_ARRAY_LEN:
        printf("array length must be a positive integer");
        break;
    case ILLEGAL_ARRAY_INDEX:
        printf("array index must be a positive integer");
        break;
    case UNDECLARED_SYMBOL:
        vprintf("undeclared symbol '%s'", args);
        break;
    case NONVAR_ASSMT_DEST:
        vprintf("assignment destination '%s' is not a variable", args);
        break;
    case INCOMPATIBLE_TYPE_ASSMT:
        vprintf("value of type %s cannot be assigned to location of type %s", args);
        break;
    case NOT_AN_ARRAY:
        vprintf("subscripted symbol '%s' is not an array", args);
        break;
    case NOT_A_PROC:
        vprintf("called symbol '%s' is not a procedure", args);
        break;
    case MISSING_ARG:
        vprintf("missing argument of type %s in call to procedure '%s'", args);
        break;
    case UNEXPECTED_TOKEN_IN_PROC_CALL:
        vprintf("unexpected token '%s' in call to procedure '%s' (%d arguments expected)", args);
        break;
    case INVALID_ARG_TYPE:
        vprintf("procedure '%s' expects argument of type %s, but argument %d has type %s", args);
        break;
    case INVALID_OPERAND_TYPE:
        vprintf("operator '%s' does not support operand of type %s", args);
        break;
    case INVALID_OPERAND_TYPES:
        vprintf("operator '%s' does not support operands of type %s and %s", args);
        break;
    case NONBOOL_CONDITION:
        printf("conditional expression must have type BOOL");
        break;
    case OUT_OF_MEMORY:
        printf("ran out of memory during compilation");
        break;
    }
	printf("\n");
}