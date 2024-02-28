#ifndef SYMBOL_TABLE_CHAIN_H
#define SYMBOL_TABLE_CHAIN__H

#include "compiler/hash_table.h"

typedef struct stc stc;

stc *stc_create();
void stc_destroy(stc* head);
void stc_add_local_table(stc* head);
void stc_del_local_table(stc* head);
void stc_put_global(stc *head, const char *name, void *symbol);
void stc_put_current_scope(stc *head, const char *name, void *symbol);
void *stc_search(stc *head, const char *name);

#endif