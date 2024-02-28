#include <stdio.h>
#include <stdlib.h>
#include "compiler/symbol_table_chain.h"

struct stc {
	ht *table;
	stc *prev;
	stc *next;
};

stc *stc_create() {
	stc *head = malloc(sizeof(stc));
	head->table = ht_create();
	head->prev = NULL;
	head->next = NULL;
	return head;
}

void stc_destroy(stc *head) {
	stc *link = head;
	while (link != NULL) {
		stc *tmp = link;
		ht_destroy(tmp->table);
		link = tmp->next;
		free(tmp);
	}
}

void stc_add_local_table(stc *head) {
	stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
	link->next = malloc(sizeof(stc));
	link->next->table = ht_create();
	link->next->prev = link;
	link->next->next = NULL;
}

void stc_del_local_table(stc *head) {
	if (!head->next) {
		printf("warning: cannot delete global symbol table");
		return;
	}
	stc *link = head->next;
	while (link->next != NULL) {
		link = link->next;
	}
	ht_destroy(link->table);
	link->prev->next = NULL;
	free(link);
}

void stc_put_global(stc *head, const char *name, void *symbol) {
	ht_set(head->table, name, symbol);
}

void stc_put_current_scope(stc *head, const char *name, void *symbol) {
	stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
	ht_set(link->table, name, symbol);
}

void *stc_search(stc *head, const char *name) {
	stc *link = head;
	while (link->next != NULL) {
		link = link->next;
	}
	while (link != NULL) {
		void *result = ht_get(link->table, name);
		if (result) return result;
		link = link->prev;
	}
	return NULL;
}