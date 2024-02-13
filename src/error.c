#include <stdio.h>
#include "compiler/error.h"

void print_error(char* file_name, error_type type, int line, char* quote) {
	printf("%s:%d: ", file_name, line);
	switch (type)
	{
	case UNREC_TOKEN:
		printf("error: unrecognized token '%s'", quote);
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
	case EXPECTED_TOKEN:
		printf("error: expected token '%s'", quote);
	}
	printf("\n");
}