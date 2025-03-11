#include "library1.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT Allocator *allocator_create(void *const memory, const size_t size)
{
    Allocator *allocator = (Allocator *)memory;
    allocator->memory = memory;
    allocator->size = size;

    for (int i = 0; i <= MAX_ORDER; ++i)
    {
        allocator->free_lists[i] = NULL;
    }

    size_t num_page = size / PAGE_SIZE;
    for (size_t i = 0; i < num_page; ++i)
    {
        void *page = (char *)memory + sizeof(Allocator) + i * PAGE_SIZE;
        Block *header = (Block *)page;
        header->size = PAGE_SIZE;
        header->is_free = 1;
        header->next = allocator->free_lists[MAX_ORDER];
        allocator->free_lists[MAX_ORDER] = header;
    }

    return allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator)
{
    (void)allocator;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size_t order = 0;
    while ((1 << order) < size + sizeof(Block))
    {
        ++order;
    }

    for (size_t i = order; i <= MAX_ORDER; ++i)
    {
        if (allocator->free_lists[i] != NULL)
        {
            Block *block = (Block *)allocator->free_lists[i];
            allocator->free_lists[i] = block->next;

            while (i > order)
            {
                --i;
                Block *new_block = (Block *)((char *)block + (1 << i));
                new_block->size = (1 << i);
                new_block->is_free = 1;
                new_block->next = allocator->free_lists[i];
                allocator->free_lists[i] = new_block;
            }
            block->size = (1 << order);
            block->is_free = 0;

            return (void *)((char *)block + sizeof(Block));
        }
    }
    return NULL;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory)
{
    if (memory == NULL)
    {
        return;
    }

    Block *block = (Block *)((char *)memory - sizeof(Block));
    block->is_free = 1;

    size_t order = 0;
    while ((1 << order) < block->size) // 2^order
    {
        ++order;
    }

    while (order < MAX_ORDER)
    {
        size_t buddy_addres = (size_t)block ^ (1 << order);
        Block *buddy = (Block *)buddy_addres;

        if (buddy->is_free && buddy->size == block->size)
        {
            Block *prev = NULL;
            Block *curr = (Block *)allocator->free_lists[order];

            while (curr != NULL && curr != buddy)
            {
                prev = curr;
                curr = curr->next;
            }
            if (prev == NULL)
            {
                allocator->free_lists[order] = buddy->next;
            }
            else
            {
                prev->next = buddy->next;
            }
            if (block > buddy)
            {
                block = buddy;
            }
            block->size <<= 1;
            ++order;
        }
        else
        {
            break;
        }
    }
    block->next = allocator->free_lists[order];
    allocator->free_lists[order] = block;
}