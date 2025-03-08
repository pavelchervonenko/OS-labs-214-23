#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

typedef void *(*alloc_func)(void *allocator, size_t size);
typedef void (*free_func)(void *allocator, void *memory);

static alloc_func allocator_alloc = NULL;
static free_func allocator_free = NULL;

static void *alloc_stub(void *allocator, size_t size)
{
    (void)allocator;
    return malloc(size);
}

static void free_stub(void *allocator, void *memory)
{
    (void)allocator;
    free(memory);
}

int main(int argc, char **argv)
{
    void *library = NULL;

    if (argc > 1)
    {
        library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
        if (library != NULL)
        {
            allocator_alloc = (alloc_func)dlsym(library, "allocator_alloc");
            allocator_free = (free_func)dlsym(library, "allocator_free");

            if (allocator_alloc == NULL || allocator_free == NULL)
            {
                const char msg[] = "warning: failed to find allocator functions in the library\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                allocator_alloc = alloc_stub;
                allocator_free = free_stub;
            }
        }
        else
        {
            const char msg[] = "warning: failed to load the library, using fallback allocator\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            allocator_alloc = alloc_stub;
            allocator_free = free_stub;
        }
    }
    else
    {
        const char msg[] = "warning: no library path provided, using fallback allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        allocator_alloc = alloc_stub;
        allocator_free = free_stub;
    }

    void *allocator = NULL;
    void *ptr1 = allocator_alloc(allocator, 128);
    void *ptr2 = allocator_alloc(allocator, 256);

    printf("Allocated pointers: %p, %p\n", ptr1, ptr2);

    allocator_free(allocator, ptr1);
    allocator_free(allocator, ptr2);

    if (library)
    {
        dlclose(library);
    }

    return 0;
}