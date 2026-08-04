// 参考: 《操作系统真相还原》(于渊) 第9章 线程
#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>

struct list_elem {
    struct list_elem* prev;
    struct list_elem* next;
};

struct list {
    struct list_elem head;
    struct list_elem tail;
};

void list_init(struct list* list);
void list_append(struct list* list, struct list_elem* elem);
void list_push(struct list* list, struct list_elem* elem);
void list_remove(struct list_elem* elem);
int list_empty(struct list* list);
struct list_elem* list_pop_front(struct list* list);
struct list_elem* elem_find(struct list* list, struct list_elem* elem);

#define list_entry(ptr, type, member) \
    ((type*)((uint32_t)(ptr) - offsetof(type, member)))

#endif
