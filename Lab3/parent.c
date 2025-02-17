#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_NAME "/shared_memory_example"
#define SEM_PRODUSER "/parent"
#define SEM_CONSUMER1 "/child1"
#define SEM_CONSUMER2 "/child2"
#define BUFFER_SIZE 4096

static char CHILD1_PROGRAM_NAME[] = "./child1";
static char CHILD2_PROGRAM_NAME[] = "./child2";

struct shared_memory
{
    char buffer[BUFFER_SIZE];
    size_t length;
};

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        const char msg[] = "usage: ./parent <filename1> <filename2>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1)
    {
        const char msg[] = "error: failed to create shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct shared_memory)) == -1)
    {
        const char msg[] = "error: failed to set size of shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    struct shared_memory *shm_ptr = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        const char msg[] = "error: failed to map shared memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_producer = sem_open(SEM_PRODUSER, O_CREAT, 0600, 1);
    sem_t *sem_consumer1 = sem_open(SEM_CONSUMER1, O_CREAT, 0600, 0);
    sem_t *sem_consumer2 = sem_open(SEM_CONSUMER2, O_CREAT, 0600, 0);

    if (sem_producer == SEM_FAILED || sem_consumer1 == SEM_FAILED || sem_consumer2 == SEM_FAILED)
    {
        const char msg[] = "error: failed to create semaphores\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t child1 = fork();

    if (child1 == 0)
    {
        execlp(CHILD1_PROGRAM_NAME, CHILD1_PROGRAM_NAME, argv[1], NULL);
        const char msg[] = "error: failed to exec child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();

    if (child2 == 0)
    {
        execlp(CHILD2_PROGRAM_NAME, CHILD2_PROGRAM_NAME, argv[2], NULL);
        const char msg[] = "error: failed to exec child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char buf[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0)
    {
        sem_wait(sem_producer);

        shm_ptr->length = bytes_read;
        for (size_t i = 0; i < bytes_read; ++i)
        {
            shm_ptr->buffer[i] = buf[i];
        }

        int probability = rand() % 100;
        if (probability < 80)
        {
            sem_post(sem_consumer1);
        }
        else
        {
            sem_post(sem_consumer2);
        }
    }

    sem_wait(sem_producer);
    shm_ptr->length = 0;
    sem_post(sem_consumer1);
    sem_post(sem_consumer2);

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    sem_close(sem_producer);
    sem_close(sem_consumer1);
    sem_close(sem_consumer2);

    sem_unlink(SEM_PRODUSER);
    sem_unlink(SEM_CONSUMER1);
    sem_unlink(SEM_CONSUMER2);
    shm_unlink(SHM_NAME);

    return 0;
}