#ifndef __LIBRARY2_H
#define __LIBRARY2_H

#include <stddef.h>
#include <stdlib.h>

#define MAX_ORDER 20

typedef struct Block
{
    struct Block *next;
} Block;

typedef struct
{
    Block *free_lists[MAX_ORDER + 1];
    void *memory;
    size_t size;
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);
void *allocator_alloc(Allocator *const allocator, const size_t size);
void allocator_destroy(Allocator *const allocator);
void allocator_free(Allocator *const allocator, void *const memory);

#endif