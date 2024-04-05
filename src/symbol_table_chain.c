#include <stdio.h>
#include <stdlib.h>
#include "compiler/symbol_table_chain.h"

struct stc {
	ht *table;
	stc *prev;
	stc *next;
};

stc *stc_create() {
    // Reserved Words
	stc *head = malloc(sizeof(stc));
	head->table = ht_create();
	head->prev = NULL;
    // Global symbols
	head->next = malloc(sizeof(stc));
    head->next->table = ht_create();
    head->next->prev = head;
    head->next->next = NULL;
	return head;
}

void stc_destroy(stc *head) {
	stc *link = head;
	while (link != NULL) {
		stc *tmp = link;
        hti iter = ht_iterator(tmp->table);
        while (ht_next(&iter)) {
            token *t = iter.value;
            free_symbol_token(t);
        }
		ht_destroy(tmp->table);
		link = tmp->next;
		free(tmp);
	}
}

void stc_add_local(stc *head) {
	stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
	link->next = malloc(sizeof(stc));
	link->next->table = ht_create();
	link->next->prev = link;
	link->next->next = NULL;
}

void stc_del_local(stc *head) {
	if (!head->next || !head->next->next) {
		printf("warning: cannot delete reserved word or global symbol table");
		return;
	}
	stc *link = head->next;
	while (link->next != NULL) {
		link = link->next;
	}
    hti iter = ht_iterator(link->table);
    while (ht_next(&iter)) {
        token *t = iter.value;
        if (t != NULL) {
            free_symbol_token(t);
        }
    }
	ht_destroy(link->table);
	link->prev->next = NULL;
	free(link);
}

void stc_put_res_word(stc *head, const char *name, token *symbol) {
	ht_set(head->table, name, symbol);
}

void stc_put_global(stc *head, const char *name, token *symbol) {
	ht_set(head->next->table, name, symbol);
}

void stc_put_local(stc *head, const char *name, token *symbol) {
	stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
	ht_set(link->table, name, symbol);
}

token *stc_search_res_word(stc *head, const char *name) {
	return ht_get(head->table, name);
}

token *stc_search_global(stc *head, const char *name) {
	return ht_get(head->next->table, name);
}

token *stc_search_local(stc *head, const char *name) {
    stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
    return ht_get(link->table, name);
}

token *stc_search_local_first(stc *head, const char *name) {
    stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
    while(link->prev != NULL) {
        void *local_result = ht_get(link->table, name);
        if (local_result) return local_result;
        link = link->prev;
    }
    return NULL;
}