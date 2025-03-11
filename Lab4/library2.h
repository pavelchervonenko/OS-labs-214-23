#include <stddef.h>
#include <stdint.h>

#ifndef __LIBRARY2_H
#define __LIBRARY2_H

#define MIN_BLOCK_SIZE 8
#define MAX_BLOCK_SIZE 1024
#define NUM_LISTS 11

typedef struct Block
{
    struct Block *next;
} Block;

typedef struct
{
    void *memory;
    size_t size;
    Block *free_lists[NUM_LISTS];
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);
void *allocator_alloc(Allocator *const allocator, const size_t size);
void allocator_destroy(Allocator *const allocator);
void allocator_free(Allocator *const allocator, void *const memory);

#endif