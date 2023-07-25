#ifndef _LIST_H
#define _LIST_H


typedef void dispose_func(void *);

typedef struct list_node_ {
        struct list_node_ *next;
        struct list_node_ *prev;
        void *data;
} list_node;

typedef struct list_ {
        list_node *head;
        list_node *tail;
        int size;
        dispose_func *dispose;
} list;

#define list_foreach(list, type, temp, curs) \
	if (list && list->size) \
{ \
	list_node *curs = list->head; \
	type temp = (type)curs->data;	\
	for(; curs; curs = curs->next, \
	    temp = curs ? (type)curs->data : NULL) {

#define list_foreach_end } }

/* Append an element to the end of a list */
list_node *list_append(list * l, void *data);
/* Delete an element from the list, freeing the data */
void list_erase(list * l, list_node * n);
/* Erase every element in a list, destroy the list */
void list_destroy(list * l);
/* Remove an item from a list without freeing it */
void *list_remove(list * l, list_node * n);
/* Create a new list */
list *list_create(dispose_func * dispose);


#endif                          /* _LIST_H */
