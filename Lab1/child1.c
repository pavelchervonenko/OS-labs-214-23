#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

void reverse_string(char *str, size_t length)
{
    size_t i = 0;
    size_t j = length - 1;
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
    char buf[4096];
    ssize_t bytes;

    pid_t pid = getpid();

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (file == -1)
    {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) // NOTE: Читаем строку из stdin
    {
        if (bytes < 0)
        {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(file);
            exit(EXIT_FAILURE);
        }

        else if (buf[0] == '\n') // NOTE: Когда клавиша Enter нажата без ввода
        {
            break;
        }

        buf[bytes - 1] = '\0'; // NOTE: Меняем символ новой строки на null терминатор

        reverse_string(buf, bytes - 1);

        ssize_t written = write(file, buf, bytes - 1);
        if (written != bytes - 1)
        {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(file);
            exit(EXIT_FAILURE);
        }

        if (write(file, "\n", 1) != 1) // NOTE: Добавляем перевод строки после записи
        {
            const char msg[] = "error: failed to write newline to file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(file);
            exit(EXIT_FAILURE);
        }

        {
            char msg[64];
            int msg_len = snprintf(msg, sizeof(msg), "written %ld bytes to file\n", written + 1);
            write(STDOUT_FILENO, msg, msg_len); // NOTE: Выводим количество записанных байт
        }
    }

    if (bytes == 0)
    {
        const char msg[] = "end of input\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }

    exit(EXIT_SUCCESS);
    close(file);
}
