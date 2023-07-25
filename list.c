#include "list.h"
#include <stdlib.h>

list *list_create(dispose_func * dispose)
{
        list *l = (list *) calloc(1, sizeof(list));
        l->dispose = dispose;
        return l;
}

list_node *list_append(list * l, void *data)
{
        list_node *ln = (list_node *) calloc(1, sizeof(list_node));
        ln->data = data;
        if (l->size == 0) {
                l->tail = ln;
                l->head = ln;
                l->size = 1;
                return ln;
        }

        l->tail->next = ln;
        ln->prev = l->tail;
        l->tail = ln;
        l->size++;
        return ln;
}

void list_destroy(list * l)
{
        list_foreach(l, void *, i, c) 
                list_erase(l, c);
        list_foreach_end;
}

void *list_remove(list * l, list_node * n)
{
        void *d = n->data;
        if (--l->size == 0) {
                l->head = NULL;
                l->tail = NULL;
                free(n);
                return d;
        }
        if (n->next)
                n->next->prev = n->prev;
        if (n->prev)
                n->prev->next = n->next;
        if (n == l->head)
                l->head = n->next;
        if (n == l->tail)
                l->tail = n->prev;
        free(n);
        return d;
}

void list_erase(list * l, list_node * n)
{
        l->dispose(list_remove(l, n));
}
