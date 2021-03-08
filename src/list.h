#pragma once

typedef struct list_node
{
	struct list *source;
	struct list_node *before, *next;
	void *val;
} list_node;

typedef struct list
{
	list_node *start, *end;
	int count;
} list;

void list_print_all_strs(list *l);
void list_test();
void list_test_push_to_front();
void list_test_push_to_back();
void list_print_all_ints(list *l);
void list_test_sort();

list *list_init();
list_node *list_get_node_at(list *ptr, int index);
void *list_get_val_at(list *ptr, int index);
void list_insert(list *ptr, list_node *n, void *v, char before);
void list_insert_at(list *ptr, int index, void *v);
void list_push_front(list *ptr, void *v);
void list_push_back(list *ptr, void *v);
void list_pop_front(list *ptr);
void list_pop_back(list *ptr);
void list_swap(list_node *n1, list_node *n2);
list_node *list_move_to_front(list *ptr, list_node *n);
list_node *list_move_to_back(list *ptr, list_node *n);
list_node *list_erase(list *ptr, list_node *n);
void list_erase_at(list *ptr, int index);
void list_dummy(void *ptr);

#define list_clear(ptr, cleaner) \
{ \
	while(ptr->start) \
	{ \
		if(cleaner != NULL) cleaner(ptr->start->val); \
		list_erase(ptr, ptr->start); \
	} \
}

#define list_free(ptr, cleaner) \
{ \
	list_clear(ptr, cleaner); \
	free(ptr); \
}