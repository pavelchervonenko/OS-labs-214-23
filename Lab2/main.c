#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct timeval time1, time2, delta_time;
struct timezone time_zone;

typedef struct
{
    long long *array;
    long long *begin;
    long long *end;
    int *count_threads_running;
    int count_threads;
} QuickSort;

void time_start()
{
    gettimeofday(&time1, &time_zone);
}
long time_stop()
{
    gettimeofday(&time2, &time_zone);

    delta_time.tv_sec = time2.tv_sec - time1.tv_sec;
    delta_time.tv_usec = time2.tv_usec - time1.tv_usec;

    if (delta_time.tv_usec < 0)
    {
        delta_time.tv_sec--;
        delta_time.tv_usec += 1000000;
    }

    return delta_time.tv_sec * 1000 + delta_time.tv_usec / 1000;
}

void swap(long long *x, long long *y)
{
    long long temp = *x;
    *x = *y;
    *y = temp;
}

void long_to_string(long long number, char **ptr)
{
    if (number == 0)
    {
        **ptr = '0';
        (*ptr)[1] = 0;
        return;
    }
    long_to_string(number / 10, ptr);
    **ptr = '0' + (number % 10);
    ++(*ptr);
    **ptr = 0;
    return;
}

void checking_sorted(long long *array, long long n)
{
    long long *ptr = array;
    for (long long i = 1; i < n; ++i)
    {
        if (*ptr > ptr[1])
        {
            char msg[] = "array not sorted\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return;
        }
        ++ptr;
    }
    char msg[] = "array sorted\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    return;
}

void splitting_array(long long **left, long long **right, long long **seed, long long *begin, long long *end)
{
    while (*left < *right)
    {
        if ((**left >= **seed) && (**right < **seed))
        {
            swap(*left, *right);
        }
        while (**left < **seed)
        {
            ++(*left);
        }
        while (*right >= begin && **right >= **seed)
        {
            --(*right);
        }
    }
    if (**left > **seed)
    {
        swap(*left, *seed);
    }
    *seed = *left;

    while (**left == **seed)
    {
        ++(*left);
    }

    ++(*right);

    return;
}

int quick_sorting(long long *array, long long *begin, long long *end, int *running_threads_count, int count_threads);

void *quick_sorting_for_thread(void *args)
{
    QuickSort *qsort_args = (QuickSort *)args;
    int result = quick_sorting(qsort_args->array, qsort_args->begin, qsort_args->end, qsort_args->count_threads_running, qsort_args->count_threads);
    free(args);
    return (void *)(long)result;
}

int quick_sorting(long long *array, long long *begin, long long *end, int *running_threads_count, int count_threads)
{
    if (end - begin < 2)
    {
        return EXIT_SUCCESS;
    }

    long long *pivot = end - 1;
    long long *left = begin;
    long long *right = end - 2;

    splitting_array(&left, &right, &pivot, begin, end);

    if (((end - left > 100000)) && (*(running_threads_count) < count_threads))
    {
        if (pthread_mutex_lock(&mutex) != 0)
        {
            char msg[] = "error: failed locking mutex\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return EXIT_FAILURE;
        }

        (*(running_threads_count))++;

        if (pthread_mutex_unlock(&mutex) != 0)
        {
            char msg[] = "error: failed unlocking mutex\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return EXIT_FAILURE;
        }

        QuickSort *args = malloc(sizeof(QuickSort));

        if (args == NULL)
        {
            char msg[] = "error: failed allocating\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return EXIT_FAILURE;
        }
        args->array = array;
        args->begin = left;
        args->end = end;
        args->count_threads_running = running_threads_count;
        args->count_threads = count_threads;

        pthread_t thread;

        if (pthread_create(&thread, NULL, quick_sorting_for_thread, args) != 0)
        {
            char msg[] = "error: failed creating thread\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            free(args);
            return EXIT_FAILURE;
        }

        quick_sorting(array, begin, right, running_threads_count, count_threads);

        pthread_join(thread, NULL);

        if (pthread_mutex_lock(&mutex) != 0)
        {
            char msg[] = "error: failed locking mutex\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return EXIT_FAILURE;
        }

        (*(running_threads_count))--;

        if (pthread_mutex_unlock(&mutex) != 0)
        {
            char msg[] = "error: failed unlocking mutex\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            return EXIT_FAILURE;
        }
    }
    else
    {
        quick_sorting(array, left, end, running_threads_count, count_threads);
        quick_sorting(array, begin, right, running_threads_count, count_threads);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        char msg[] = "error: unknown arguments\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int count_threads = atoi(argv[1]);

    int count_threads_running = 1;

    long long n = 100000000;

    int file;
    if ((file = open("input.bin", O_RDWR)) < 0)
    {
        char msg[] = "error: failed open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    long long *array;
    if ((array = mmap(NULL, n * sizeof(long long), PROT_READ | PROT_WRITE, MAP_PRIVATE, file, 0)) == (void *)(-1))
    {
        char msg[] = "error: failed mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(file);
        exit(EXIT_FAILURE);
    }

    char msg_start_sorting[] = "starting sorting...\n";
    write(STDOUT_FILENO, msg_start_sorting, sizeof(msg_start_sorting) - 1);

    time_start();

    QuickSort *data = malloc(sizeof(QuickSort));

    data->array = array;
    data->begin = array;
    data->end = array + n;
    data->count_threads = count_threads;
    data->count_threads_running = &count_threads_running;

    pthread_t thread_first;

    if (pthread_create(&thread_first, NULL, quick_sorting_for_thread, data) != 0)
    {
        char msg[] = "error: failed pthread_create\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        free(data);

        munmap(array, n * sizeof(long long));
        close(file);

        return EXIT_FAILURE;
    }

    pthread_join(thread_first, NULL);

    long long time = (long long)time_stop();

    char msg_finish_sorting[] = "sorting is completed in ";
    write(STDOUT_FILENO, msg_finish_sorting, sizeof(msg_finish_sorting) - 1);

    char *time_string = (char *)malloc(25 * sizeof(char));
    if (time_string == NULL)
    {
        char msg[] = "error: failed allocating memory\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);

        munmap(array, n * sizeof(long long));
        close(file);

        return EXIT_FAILURE;
    }
    char *ptr = time_string;
    long_to_string(time, &ptr);

    write(STDOUT_FILENO, time_string, sizeof(time_string) - 1);
    write(STDOUT_FILENO, "ms\n", sizeof("ms\n") - 1);

    free(time_string);

    checking_sorted(array, n);

    munmap(array, n * sizeof(long long));
    close(file);

    return EXIT_SUCCESS;
}