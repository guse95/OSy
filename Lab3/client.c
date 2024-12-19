#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/Data"
#define SEM_PARENT "/parent_semaphore"
#define SEM_CHILD "/child_semaphore"

int main(int argc, char **argv) {

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        const char msg[] = "error: failed to make shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    char *ptr = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        const char msg[] = "error: failed to make mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
            const char msg[] = "error: failed to create semaphore\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            close(fd);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }

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

    char buf[BUFSIZ];
    ssize_t bytes;
    char ans[BUFSIZ];

    while (1) {

        sem_wait(sem_child);

        strncpy(buf, ptr, BUFSIZ);
        if (strlen(buf) == 0) {
            break;
        }

        float sum = 0;
        if (bytes < 0) {
            abort();
//            const char msg[] = "error: failed to read from stdin\n";
//            write(STDERR_FILENO, msg, sizeof(msg));
//            exit(EXIT_FAILURE);
        } else if (buf[0] == '\0') {
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
        sem_post(sem_parent);
    }
    const char term = '\0';
    write(file, &term, sizeof(term));

    munmap(ptr, BUFSIZ);
    close(fd);
    sem_close(sem_parent);
    sem_close(sem_child);
    close(file);
}