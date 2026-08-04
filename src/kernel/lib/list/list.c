// 参考: 《操作系统真相还原》(于渊) 第9章 线程
#include "./list.h"

void list_init(struct list* list) {
    list->head.prev = 0;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = 0;
}

void list_append(struct list* list, struct list_elem* elem) {
    elem->next = &list->tail;
    elem->prev = list->tail.prev;
    list->tail.prev->next = elem;
    list->tail.prev = elem;
}

void list_push(struct list* list, struct list_elem* elem) {
    elem->prev = &list->head;
    elem->next = list->head.next;
    list->head.next->prev = elem;
    list->head.next = elem;
}

void list_remove(struct list_elem* elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
}

int list_empty(struct list* list) {
    return list->head.next == &list->tail;
}

struct list_elem* list_pop_front(struct list* list) {
    struct list_elem* first = list->head.next;
    if (first == &list->tail) {
        return 0;
    }
    list_remove(first);
    return first;
}

struct list_elem* elem_find(struct list* list, struct list_elem* elem) {
    struct list_elem* e = list->head.next;
    while (e != &list->tail) {
        if (e == elem) {
            return e;
        }
        e = e->next;
    }
    return 0;
}
