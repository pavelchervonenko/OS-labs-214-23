#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#include "library1.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT size_t round_up_to_power_of_two(size_t size)
{
    if (size == 0)
    {
        return 1;
    }

    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;

    return size;
}

EXPORT Allocator *allocator_create(void *const memory, const size_t size)
{
    Allocator *allocator = (Allocator *)malloc(sizeof(Allocator));
    if (!allocator)
        return NULL;

    allocator->memory = memory;
    allocator->size = size;
    allocator->free_pages = NULL;

    for (int i = 0; i <= MAX_ORDER; ++i)
    {
        allocator->free_lists[i] = NULL;
    }

    // Инициализация свободных страниц
    size_t num_pages = size / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; ++i)
    {
        Page *page = (Page *)((char *)memory + i * PAGE_SIZE);
        page->next_free = allocator->free_pages;
        page->block_size = 0;
        page->large_block = NULL;
        allocator->free_pages = page;
    }

    return allocator;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size)
{
    if (size == 0)
        return NULL;

    // Округление до ближайшей степени двойки
    size_t block_size = 1;
    while (block_size < size)
    {
        block_size <<= 1;
    }

    int order = (int)log2(block_size);

    if (order > MAX_ORDER)
    {
        // Выделение большого блока
        Page *page = allocator->free_pages;
        if (!page)
            return NULL;

        allocator->free_pages = page->next_free;
        page->large_block = page;
        return (void *)((char *)page + sizeof(Page));
    }

    // Поиск свободного блока нужного размера
    if (allocator->free_lists[order])
    {
        Page *page = allocator->free_lists[order];
        allocator->free_lists[order] = page->next_free;
        return (void *)((char *)page + sizeof(Page));
    }

    // Если нет свободного блока, разбиваем страницу на блоки
    Page *page = allocator->free_pages;
    if (!page)
        return NULL;

    allocator->free_pages = page->next_free;
    page->block_size = block_size;

    size_t num_blocks = PAGE_SIZE / block_size;
    for (size_t i = 1; i < num_blocks; ++i)
    {
        Page *new_block = (Page *)((char *)page + i * block_size);
        new_block->next_free = allocator->free_lists[order];
        allocator->free_lists[order] = new_block;
    }

    return (void *)((char *)page + sizeof(Page));
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory)
{
    if (!memory)
        return;

    Page *page = (Page *)((char *)memory - sizeof(Page));

    if (page->large_block)
    {
        // Освобождение большого блока
        page->next_free = allocator->free_pages;
        allocator->free_pages = page;
    }
    else
    {
        // Освобождение блока
        size_t block_size = page->block_size;
        int order = (int)log2(block_size);
        page->next_free = allocator->free_lists[order];
        allocator->free_lists[order] = page;
    }
}

void allocator_destroy(Allocator *const allocator)
{
    if (allocator)
    {
        free(allocator);
    }
}