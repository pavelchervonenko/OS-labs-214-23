#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static char CHILD1_PROGRAM_NAME[] = "child1";
static char CHILD2_PROGRAM_NAME[] = "child2";

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        const char msg[] = "error: unknown args\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    srand(time(NULL)); // NOTE: Инициалиализация генератора случайных чисел

    char progpath[1024]; // NOTE: Получаем полный путь к каталогу, в котором находится программа
    {
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1); // NOTE: Читаем полный путь к программе, включая ее название

        if (len == -1)
        {
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        while (progpath[len] != '/')
            --len;

        progpath[len] = '\0';
    }

    char buf[4096];
    ssize_t bytes;

    // NOTE: Открываем два канала
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1)
    {
        const char msg[] = "error: failed to create pipe1\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2) == -1)
    {
        const char msg[] = "error: failed to create pipe2\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // NOTE: Создаем новый процесс
    const pid_t child1 = fork();

    switch (child1)
    {
    case -1: // NOTE: Ядру не удается создать другой процесс
    {
        const char msg[] = "error: failed to spawn new process\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    break;

    case 0: // NOTE: Мы - потомок1, потомок1 не знает своего pid после развилки
    {
        pid_t pid = getpid();         // NOTE: Получаем PID потомка1
        dup2(pipe1[0], STDIN_FILENO); // NOTE: Соединяем входные потоки
        close(pipe1[1]);
        {
            char msg[64];
            const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child1\n", pid);
            write(STDOUT_FILENO, msg, length);
        }
        {
            char path[1024];
            snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CHILD1_PROGRAM_NAME);

            char *const args[] = {CHILD1_PROGRAM_NAME, argv[1], NULL};

            int32_t status = execv(path, args);

            if (status == -1)
            {
                const char msg[] = "error: failed to exec into new exectuable image\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }
    break;

    default:
    {
        const pid_t child2 = fork();

        switch (child2)
        {
        case -1: // NOTE: Ядру не удается создать другой процесс
        {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        break;

        case 0: // NOTE: Мы - потомок2, потомок2 не знает своего pid после развилки
        {
            pid_t pid = getpid();         // NOTE: Получаю PID потомка2
            dup2(pipe2[0], STDIN_FILENO); // NOTE: Соединяем входные потоки
            close(pipe2[1]);
            {
                char msg[64];
                const int32_t length = snprintf(msg, sizeof(msg) - 1, "%d: I'm a child2\n", pid);
                write(STDOUT_FILENO, msg, length);
            }
            {
                char path[1024];
                snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CHILD2_PROGRAM_NAME);

                char *const args[] = {CHILD2_PROGRAM_NAME, argv[2], NULL};

                int32_t status = execv(path, args);

                if (status == -1)
                {
                    const char msg[] = "error: failed to exec into new exectuable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        }
        break;

        default: // NOTE: Мы - родитель, родитель знает свой pid после развилки
        {
            pid_t pid = getpid(); // NOTE: Получаем PID родителя
            {
                char msg[64];
                const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a parent, my children has PID %d and PID %d\n", pid, child1, child2);
                write(STDOUT_FILENO, msg, length);
            }

            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0)
            {
                int probability = rand() % 100; // NOTE: Генерериуем вероятность

                if (probability < 80)
                {
                    write(pipe1[1], buf, bytes);
                }
                else
                {
                    write(pipe2[1], buf, bytes);
                }
            }

            close(pipe1[1]);
            close(pipe2[1]);

            // NOTE: ожидаем завершение дочерних процессов
            int child_status1, child_status2;
            waitpid(child1, &child_status1, 0);
            waitpid(child2, &child_status2, 0);

            if (child_status1 != EXIT_SUCCESS || child_status2 != EXIT_SUCCESS)
            {
                const char msg[] = "error: one of the children exited with error\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
        break;
        }
    }
    break;
    }
}