#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>

#define SHM_NAME "/shared_memory_example"
#define SEM_CONSUMER1 "/child1"
#define SEM_PRODUCER "/parent"

struct shared_memory
{
    char buffer[4096];
    size_t length;
};

void reverse_string(char *str, size_t length)
{
    size_t i = 0, j = length - 1;
    while (i < j)
    {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        const char msg[] = "usage: ./child1 <filename>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (file == -1)
    {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_consumer = sem_open(SEM_CONSUMER1, 0);
    sem_t *sem_producer = sem_open(SEM_PRODUCER, 0);
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
    struct shared_memory *shm_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (sem_consumer == SEM_FAILED || sem_producer == SEM_FAILED || shm_ptr == MAP_FAILED)
    {
        const char msg[] = "error: failed to open resources\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        sem_wait(sem_consumer);

        if (shm_ptr->length == 0)
        {
            break;
        }

        reverse_string(shm_ptr->buffer, shm_ptr->length);

        if (write(file, shm_ptr->buffer, shm_ptr->length) != (ssize_t)shm_ptr->length)
        {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(file);
            exit(EXIT_FAILURE);
        }

        sem_post(sem_producer);
    }

    close(file);
    munmap(shm_ptr, sizeof(struct shared_memory));
    return 0;
}