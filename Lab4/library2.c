#include "library2.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

static size_t get_index(size_t size)
{
    size_t index = 0;
    size_t block_size = MIN_BLOCK_SIZE;
    while (block_size < size)
    {
        block_size <<= 1;
        index++;
    }
    return index;
}

static size_t get_block_size(size_t index)
{
    return MIN_BLOCK_SIZE << index;
}

EXPORT Allocator *allocator_create(void *const memory, const size_t size)
{
    Allocator *allocator = (Allocator *)memory;
    allocator->memory = memory;
    allocator->size = size;

    for (int i = 0; i < NUM_LISTS; i++)
    {
        allocator->free_lists[i] = NULL;
    }

    size_t remaining_size = size - sizeof(Allocator);
    void *current = (void *)((char *)memory + sizeof(Allocator));

    while (remaining_size >= MIN_BLOCK_SIZE)
    {
        size_t block_size = MIN_BLOCK_SIZE;
        size_t index = 0;

        while (block_size * 2 <= remaining_size)
        {
            block_size <<= 1;
            index++;
        }

        Block *block = (Block *)current;
        block->next = allocator->free_lists[index];
        allocator->free_lists[index] = block;

        current = (void *)((char *)current + block_size);
        remaining_size -= block_size;
    }

    return allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator)
{
    (void)allocator;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    if (size == 0 || size > MAX_BLOCK_SIZE)
    {
        return NULL;
    }

    size_t index = get_index(size);
    if (index >= NUM_LISTS)
    {
        return NULL;
    }

    if (allocator->free_lists[index] != NULL)
    {
        Block *block = allocator->free_lists[index];
        allocator->free_lists[index] = block->next;
        return (void *)block;
    }

    for (size_t i = index + 1; i < NUM_LISTS; i++)
    {
        if (allocator->free_lists[i] != NULL)
        {
            Block *block = allocator->free_lists[i];
            allocator->free_lists[i] = block->next;

            size_t block_size = get_block_size(i);
            size_t new_block_size = block_size / 2;

            Block *new_block = (Block *)((char *)block + new_block_size);
            new_block->next = allocator->free_lists[i - 1];
            allocator->free_lists[i - 1] = new_block;

            return (void *)block;
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

    size_t block_size = MIN_BLOCK_SIZE;
    size_t index = 0;
    uintptr_t memory_offset = (uintptr_t)memory - (uintptr_t)allocator->memory;

    while (block_size < MAX_BLOCK_SIZE)
    {
        if (memory_offset % block_size == 0)
        {
            break;
        }
        block_size <<= 1;
        index++;
    }

    Block *block = (Block *)memory;
    block->next = allocator->free_lists[index];
    allocator->free_lists[index] = block;

    while (index < NUM_LISTS - 1)
    {
        uintptr_t buddy_offset = memory_offset ^ block_size;
        Block *buddy = (Block *)((uintptr_t)allocator->memory + buddy_offset);
        Block *prev = NULL;
        Block *curr = allocator->free_lists[index];

        while (curr != NULL)
        {
            if (curr == buddy)
            {
                if (prev == NULL)
                {
                    allocator->free_lists[index] = curr->next;
                }
                else
                {
                    prev->next = curr->next;
                }

                block_size <<= 1;
                index++;
                memory_offset = memory_offset & ~(block_size - 1);
                block = (Block *)((uintptr_t)allocator->memory + memory_offset);
                block->next = allocator->free_lists[index];
                allocator->free_lists[index] = block;
                break;
            }
            prev = curr;
            curr = curr->next;
        }

        if (curr == NULL)
        {
            break;
        }
    }
}