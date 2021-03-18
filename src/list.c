#include "list.h"
#include <stdlib.h>
#include <stddef.h>
#include "utils.h"

void list_print_all_strs(list *l)
{
	printf("\n");
	int i = 0;
	list_node *n;
	for(n = l->start; n != NULL; n = n->next)
	{
		char *str = (char*) n->val;
		printf("%d. %s\n", i++, str);
	}
}

void list_test()
{
	list *l = list_init();
	list_push_back(l, dupe_str("1"));
	list_print_all_strs(l);
	list_push_back(l, dupe_str("2"));
	list_print_all_strs(l);
	list_push_front(l, dupe_str("3"));
	list_print_all_strs(l);
	list_push_back(l, dupe_str("4"));
	list_print_all_strs(l);
	list_free(l, free);
}

void list_test_push_to_front()
{
	list *l = list_init();
	list_push_back(l, dupe_str("message 1"));
	list_push_back(l, dupe_str("message 2"));
	list_push_back(l, dupe_str("message 3"));
	list_push_back(l, dupe_str("message 4"));
	list_move_to_front(l, list_get_node_at(l, 3));
	list_print_all_strs(l);
	list_free(l, free);
}

void list_test_push_to_back()
{
	list *l = list_init();
	list_push_back(l, dupe_str("message 1"));
	list_push_back(l, dupe_str("message 2"));
	list_push_back(l, dupe_str("message 3"));
	list_push_back(l, dupe_str("message 4"));
	list_move_to_back(l, list_get_node_at(l, 0));
	list_print_all_strs(l);
	list_free(l, free);
}

void list_print_all_ints(list *l)
{
	printf("\n");
	int i = 0;
	list_node *n;
	for(n = l->start; n != NULL; n = n->next)
	{
		int *ii = (int*) n->val;
		printf("%d. %d\n", i++, *ii);
	}
}

void list_test_sort()
{
	list *l = list_init();
	list_push_back(l, new_int(30));
	list_push_back(l, new_int(50));
	list_push_back(l, new_int(21));
	list_push_back(l, new_int(11));
	list_push_back(l, new_int(8));
	list_node *n;
	for(n = l->start; n != NULL; n = n->next)
	{
		int *v = (int*) n->val;
		if(*v < *((int*) l->start->val))
			list_move_to_front(l, n);
	}
	list_print_all_ints(l);
	list_free(l, free);
}

list *list_init()
{
	list *ret = malloc(sizeof *ret);
	ret->start = NULL;
	ret->end = NULL;
	ret->count = 0;
	return ret;
}

list_node *list_get_node_at(list *ptr, int index)
{
	if(index < 0 || index > ptr->count - 1) return NULL;
	list_node *n;
	int cnt = -1;
	for(n = ptr->start; n != NULL; n = n->next)
		if(++cnt == index) return n;
	return NULL;
}

void *list_get_val_at(list *ptr, int index)
{
	list_node *n = list_get_node_at(ptr, index);
	if(n == NULL) return NULL;
 	return n->val;
}

void list_insert(list *ptr, list_node *n, void *v, char before)
{
	if(before && n == ptr->start)
	{
		list_push_front(ptr, v);
		return;
	}
	list_node *nn = malloc(sizeof *nn);
	nn->source = ptr;
	nn->val = v;
	if(n)
	{
		if(before)
		{
			nn->before = n->before;
			nn->next = n;
			if(n->before) n->before->next = nn;
			n->before = nn;
		}
		else
		{
			nn->before = n;
			nn->next = n->next;
			if(n->next) n->next->before = nn;
			n->next = nn;
		}
	}
	else
	{
		nn->before = NULL;
		nn->next = NULL;
		ptr->start = nn;
		ptr->end = nn;
	}
	if(n == ptr->end) ptr->end = nn;
	++ptr->count;
}

void list_insert_at(list *ptr, int index, void *v)
{
	list_insert(ptr, list_get_node_at(ptr, index), v, 0);
}

void list_push_front(list *ptr, void *v)
{
	if(ptr->count < 1)
	{
		list_insert(ptr, ptr->end, v, 0);
		return;
	}
	list_node *nn = malloc(sizeof *nn);
	nn->source = ptr;
	nn->val = v;
	nn->before = NULL;
	nn->next = ptr->start;
	ptr->start->before = nn;
	ptr->start = nn;
	++ptr->count;
}

void list_push_back(list *ptr, void *v)
{
	list_insert(ptr, ptr->end, v, 0);
}

void list_pop_front(list *ptr)
{
	list_erase(ptr, ptr->start);
}

void list_pop_back(list *ptr)
{
	list_erase(ptr, ptr->end);
}

void list_swap(list_node *n1, list_node *n2)
{
	void *n1d = n1->val;
	n1->val = n2->val;
	n2->val = n1d;
}

list_node *list_move_to_front(list *ptr, list_node *n)
{
	void *nv = n->val;
	list_node *ret = n->next;
	list_push_front(ptr, nv);
	list_erase(ptr, n);
	return ret;
}

list_node *list_move_to_back(list *ptr, list_node *n)
{
	void *nv = n->val;
	list_push_back(ptr, nv);
	return list_erase(ptr, n);
}

list_node *list_erase(list *ptr, list_node *n)
{
	list_node *ret = n->next;
	if(n->before) n->before->next = n->next;
	if(n->next) n->next->before = n->before;
	if(n == ptr->start) ptr->start = n->next;
	if(n == ptr->end) ptr->end = n->before;
	free(n);
	--ptr->count;
	return ret;
}

void list_erase_at(list *ptr, int index)
{
	list_erase(ptr, list_get_node_at(ptr, index));
}

void list_dummy(void *ptr)
{
}