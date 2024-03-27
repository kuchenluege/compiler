#ifndef SYMBOL_TABLE_CHAIN_H
#define SYMBOL_TABLE_CHAIN__H

#include "compiler/hash_table.h"
#include "compiler/token.h"

typedef struct stc stc;

stc *stc_create();
void stc_destroy(stc* head);
void stc_add_local(stc* head);
void stc_del_local(stc* head);
void stc_put_res_word(stc *head, const char *name, token *symbol);
void stc_put_global(stc *head, const char *name, token *symbol);
void stc_put_local(stc *head, const char *name, token *symbol);
token *stc_search_res_word(stc *head, const char *name);
token *stc_search_global(stc *head, const char *name);
token *stc_search_local(stc *head, const char *name);
token *stc_search_local_first(stc *head, const char *name);

#endif