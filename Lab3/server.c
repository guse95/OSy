#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/Data"
#define SEM_PARENT "/parent_semaphore"
#define SEM_CHILD "/child_semaphore"
static char CLIENT_PROGRAM_NAME[] = "client";

int main(int argc, char **argv) {
    printf("HUI");
    if (argc == 1) {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    char progpath[1024];
    {
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
        if (len == -1) {
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        while (progpath[len] != '/')
            --len;

        progpath[len] = '\0';
    }
    printf("HUI-1");

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        const char msg[] = "error: failed to make shm_open\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd, BUFSIZ) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    printf("HUI-2");
    char *ptr = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        const char msg[] = "error: failed to make mmap\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("HUI-3");
    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("HUI");
    char buf[BUFSIZ];
    ssize_t bytes;

    const pid_t child = fork();

    switch (child) {
        case -1: {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } break;

        case 0: {
            printf("HUI2");
            pid_t pid = getpid();


            {
                char msg[64];
                const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
                write(STDOUT_FILENO, msg, length);
            }

            {
                char path[1024];
                snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME);

                char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};

                int32_t status = execv(path, args);

                if (status == -1) {
                    const char msg[] = "error: failed to exec into new executable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        } break;

        default: {
            printf("HUI3");
            pid_t pid = getpid();
            {
                char msg[64];
                const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a parent, my child has PID %d\n", pid, child);
                write(STDOUT_FILENO, msg, length);
            }

            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1))) {
                buf[strcspn(buf, "\n")] = '\0';

                strncpy(ptr, buf, BUFSIZ);

                sem_post(sem_child);

                sem_wait(sem_parent);
            }

            strncpy(ptr, "", BUFSIZ);
            sem_post(sem_child);

            wait(NULL);

            munmap(ptr, BUFSIZ);
            shm_unlink(SHM_NAME);
            sem_close(sem_parent);
            sem_close(sem_child);
            sem_unlink(SEM_PARENT);
            sem_unlink(SEM_CHILD);
            close(fd);
        }
        break;
    }
}