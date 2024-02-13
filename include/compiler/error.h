#ifndef ERROR_H
#define ERROR_H

typedef enum {UNREC_TOKEN, UNCLOSED_COMMENT, UNCLOSED_STR, EXTRA_DEC_PT, EXPECTED_TOKEN} error_type;

void print_error(char* file_name, error_type type, int line, char* quote);

#endif