#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

#define NUM_ALLOCS 10000
#define MAX_ALLOC_SIZE 1024

typedef void *(*alloc_func)(void *allocator, size_t size);
typedef void (*free_func)(void *allocator, void *memory);
typedef void *(*create_func)(void *memory, size_t size);
typedef void (*destroy_func)(void *allocator);

static alloc_func allocator_alloc = NULL;
static free_func allocator_free = NULL;
static create_func allocator_create = NULL;
static destroy_func allocator_destroy = NULL;

static double get_time_diff(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

static void run_tests(void *allocator, const char *allocator_name)
{
    struct timespec start, end;
    void *ptrs[NUM_ALLOCS];
    size_t total_allocated = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        size_t size = rand() % MAX_ALLOC_SIZE + 1;
        ptrs[i] = allocator_alloc(allocator, size);
        if (ptrs[i])
        {
            total_allocated += size;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("[%s] Allocation time: %.6f sec\n", allocator_name, get_time_diff(start, end));

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        allocator_free(allocator, ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("[%s] Deallocation time: %.6f sec\n", allocator_name, get_time_diff(start, end));

    size_t total_memory = NUM_ALLOCS * MAX_ALLOC_SIZE;
    double utilization_factor = (double)total_allocated / total_memory;
    printf("[%s] Memory utilization factor: %.6f\n\n", allocator_name, utilization_factor);
}

static void test_malloc_free()
{
    struct timespec start, end;
    void *ptrs[NUM_ALLOCS];
    size_t total_allocated = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        size_t size = rand() % MAX_ALLOC_SIZE + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i])
        {
            total_allocated += size;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("[malloc] Allocation time: %.6f sec\n", get_time_diff(start, end));

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        free(ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("[malloc] Deallocation time: %.6f sec\n", get_time_diff(start, end));

    size_t total_memory = NUM_ALLOCS * MAX_ALLOC_SIZE;
    double utilization_factor = (double)total_allocated / total_memory;
    printf("[malloc] Memory utilization factor: %.6f\n\n", utilization_factor);
}

static void *alloc_reserv(void *allocator, size_t size)
{
    (void)allocator;

    size_t total_size = size + sizeof(size_t);
    void *ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED)
    {
        const char msg[] = "error: failed mmap in alloc_reserv\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return NULL;
    }

    *((size_t *)ptr) = size;

    return (void *)((char *)ptr + sizeof(size_t));
}

static void free_reserv(void *allocator, void *memory)
{
    (void)allocator;
    if (memory)
    {
        void *original_ptr = (void *)((char *)memory - sizeof(size_t));

        size_t size = *((size_t *)original_ptr);

        if (munmap(original_ptr, size + sizeof(size_t)) == -1)
        {
            const char msg[] = "error: failed munmap in free_reserv\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return;
        }
    }
}

int main(int argc, char **argv)
{
    void *library = NULL;

    if (argc > 1)
    {
        library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);

        if (library != NULL)
        {
            allocator_create = (create_func)dlsym(library, "allocator_create");
            allocator_destroy = (destroy_func)dlsym(library, "allocator_destroy");
            allocator_alloc = (alloc_func)dlsym(library, "allocator_alloc");
            allocator_free = (free_func)dlsym(library, "allocator_free");

            if (allocator_create == NULL || allocator_destroy == NULL ||
                allocator_alloc == NULL || allocator_free == NULL)
            {
                const char msg[] = "warning: failed to find allocator func in the library\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                allocator_alloc = alloc_reserv;
                allocator_free = free_reserv;
            }
        }
        else
        {
            const char msg[] = "warning: failed to load the library -> using reserv allocator\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            allocator_alloc = alloc_reserv;
            allocator_free = free_reserv;
        }
    }
    else
    {
        const char msg[] = "warning: no library path -> using reserv allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        allocator_alloc = alloc_reserv;
        allocator_free = free_reserv;
    }

    const char msg_start[] = "--- Running tests ---\n\n";
    write(STDERR_FILENO, msg_start, sizeof(msg_start));

    size_t memory_size = 4096 * 100;
    void *memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
    {
        const char msg[] = "error in main: failed to allocate memory for allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return -1;
    }

    void *allocator = NULL;
    allocator = allocator_create(memory, memory_size);

    if (allocator == NULL)
    {
        const char msg_create[] = "error in main: allocator_create\n\n";
        write(STDERR_FILENO, msg_create, sizeof(msg_create));
        return -1;
    }

    run_tests(allocator, "Custom Allocator");

    if (allocator_destroy)
    {
        allocator_destroy(allocator);
    }

    if (munmap(memory, memory_size) == -1)
    {
        const char msg[] = "error in main: failed to free memory for allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    if (library)
    {
        if (dlclose(library) == 0)
        {
            const char msg[] = "Library close!\n\n";
            write(STDERR_FILENO, msg, sizeof(msg));
        }
    }

    test_malloc_free();

    const char msg_finish[] = "--- Tests completed ---\n";
    write(STDERR_FILENO, msg_finish, sizeof(msg_finish));

    return 0;
}