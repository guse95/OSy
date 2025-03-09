#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes;
    char ans[4096];

    pid_t pid = getpid();

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    {
        char msg[128];
        int32_t len = snprintf(msg, sizeof(msg) - 1,
                               "%d: Start typing lines of text. Press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
        write(STDOUT_FILENO, msg, len);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
        float sum = 0;
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            break;
        }

        {
            char msg[32];
            int32_t len = snprintf(msg, sizeof(msg) - 1,
                                   "Sum of your numbers: ");

            int32_t written = write(file, msg, len);
            if (written != len) {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }

        {
            buf[bytes] = '\0';
            int point_cnt = 0;
            int numb_cnt = 1;
            for (int i = 0; i < bytes - 1; ++i) {
                if (isdigit(buf[i]) || (buf[i] == '.' && !point_cnt)) {
                    if (buf[i] == '.') point_cnt++;
                    continue;
                }
                if (buf[i] == ' ') {
                    point_cnt = 0;
                    buf[i] = '\0';
                    continue;
                }
                const char msg[] = "error: value is not a number\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }

        {
            char *ptr = buf;
            float numb = 0;
            sum += atof(ptr);
            for(int i = 0; i < bytes - 1; ++i) {
                if (buf[i] == '\0' && bytes > i + 1) {
                    numb = atof(ptr + i + 1);
                    sum += numb;
                }
            }

            size_t ansLen = snprintf(ans, sizeof(ans), "%.5f\n", sum);
            int32_t written = write(file, ans, ansLen);
            if (written != ansLen) {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }
    const char term = '\0';
    write(file, &term, sizeof(term));

    close(file);
}