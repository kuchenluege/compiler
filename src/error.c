#include <stdarg.h>
#include <stdio.h>
#include "compiler/error.h"

void print_error(char* file_name, error_type type, int line, ...) {
	va_list args;
    va_start(args, line);

	printf("%s:%d: ", file_name, line);
	switch (type)
	{
	case UNREC_TOKEN:
		vprintf("error: unrecognized token '%s'", args);
		break;
	case UNCLOSED_COMMENT:
		printf("error: unterminated comment");
		break;
	case UNCLOSED_STR:
		printf("error: mising terminating \" character");
		break;
	case EXTRA_DEC_PT:
		printf("error: too many decimal points in number");
		break;
	case MISSING_TOKEN:
		vprintf("error: expected %s, got %s token", args);
		break;
	}
	printf("\n");
}