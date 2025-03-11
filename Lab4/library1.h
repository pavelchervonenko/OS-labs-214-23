#include <stddef.h>
#include <stdint.h>

#ifndef __LIBRARY1_H
#define __LIBRARY1_H

#define PAGE_SIZE 4096
#define MAX_ORDER 10

typedef struct
{
    size_t size;
    int is_free;
    void *next;
} Block;

typedef struct
{
    void *memory;
    size_t size;
    void *free_lists[MAX_ORDER + 1];
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);
void *allocator_alloc(Allocator *const allocator, const size_t size);
void allocator_destroy(Allocator *const allocator);
void allocator_free(Allocator *const allocator, void *const memory);

#endif