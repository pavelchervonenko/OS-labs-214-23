#include "library2.h"
#include "stdint.h"
#include "unistd.h"
#include <stdio.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT Allocator *allocator_create(void *const memory, const size_t size)
{
    if (!memory || size < 1)
    {
        return NULL;
    }

    Allocator *allocator = (Allocator *)malloc(sizeof(Allocator));
    if (!allocator)
    {
        return NULL;
    }

    for (int i = 0; i <= MAX_ORDER; i++)
    {
        allocator->free_lists[i] = NULL;
    }

    size_t total_size = size;
    size_t block_size = (size_t)1 << MAX_ORDER;

    while (total_size >= block_size)
    {
        Block *block = (Block *)((uint8_t *)memory + (total_size - block_size));
        block->next = allocator->free_lists[MAX_ORDER];
        allocator->free_lists[MAX_ORDER] = block;
        total_size -= block_size;
    }

    return allocator;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    if (!allocator || size <= 0)
    {
        return NULL;
    }

    int order = 0;
    while ((size_t)(1 << order) < size && order < MAX_ORDER)
    {
        order++;
    }

    for (int i = order; i <= MAX_ORDER; i++)
    {
        if (allocator->free_lists[i])
        {
            Block *block = allocator->free_lists[i];
            allocator->free_lists[i] = block->next;

            while (i > order)
            {
                i--;
                Block *buddy = (Block *)((uint8_t *)block + ((size_t)1 << i));
                buddy->next = allocator->free_lists[i];
                allocator->free_lists[i] = buddy;
            }

            block->next = (Block *)((uintptr_t)allocator->free_lists[order] | 1);
            return (void *)block;
        }
    }

    return NULL;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory)
{
    if (!allocator || !memory)
    {
        return;
    }

    Block *block = (Block *)memory;
    int order = 0;
    while (order <= MAX_ORDER)
    {
        if ((uintptr_t)block->next & 1)
        {
            break;
        }
        order++;
    }

    block->next = allocator->free_lists[order];
    allocator->free_lists[order] = block;

    while (order < MAX_ORDER)
    {
        Block *buddy = (Block *)((uintptr_t)block ^ ((size_t)1 << order));
        Block *prev = NULL;
        Block *curr = allocator->free_lists[order];

        while (curr && curr != buddy)
        {
            prev = curr;
            curr = curr->next;
        }

        if (curr != buddy)
            break;

        if (prev)
        {
            prev->next = curr->next;
        }
        else
        {
            allocator->free_lists[order] = curr->next;
        }

        if (block > buddy)
        {
            block = buddy;
        }

        order++;
        block->next = allocator->free_lists[order];
        allocator->free_lists[order] = block;
    }
}

EXPORT void allocator_destroy(Allocator *const allocator)
{
    if (allocator)
    {
        free(allocator);
    }
}