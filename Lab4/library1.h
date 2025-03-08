#ifndef __LIBRARY1_H
#define __LIBRARY1_H

#include <stddef.h>
#include <stdlib.h>

#define PAGE_SIZE 4096 
#define MAX_ORDER 10   

typedef struct Page
{
    struct Page *next_free;
    size_t block_size;
    void *large_block;
} Page;

typedef struct Allocator
{
    void *memory;
    size_t size;
    Page *free_pages;
    Page *free_lists[MAX_ORDER + 1];
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);
void *allocator_alloc(Allocator *const allocator, const size_t size);
void allocator_destroy(Allocator *const allocator);
void allocator_free(Allocator *const allocator, void *const memory);

#endif