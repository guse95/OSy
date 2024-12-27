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
    if (argc == 1) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    char progpath[1024];
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }

    while (progpath[len] != '/') {
        --len;
    }

    progpath[len] = '\0';

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, BUFSIZ) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char *ptr = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        close(fd);
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    switch (child) {
        case 0: {
            pid_t pid = getpid();
            printf("%d: I'm a child\n", pid);

            char path[1024];
            snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME);

            execl(path, CLIENT_PROGRAM_NAME, argv[1], NULL);
            perror("execl");
            exit(EXIT_FAILURE);
        }

        default: {
            pid_t pid = getpid();
            printf("%d: I'm a parent, my child has PID %d\n", pid, child);

            char buf[BUFSIZ];
            ssize_t bytes;
            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
                buf[strcspn(buf, "\n")] = '\0';
                strncpy(ptr, buf, BUFSIZ);
                sem_post(sem_child);
                sem_wait(sem_parent);
            }

            strncpy(ptr, "", BUFSIZ);
            sem_post(sem_child);

            int status;
            wait(&status);

            munmap(ptr, BUFSIZ);
            shm_unlink(SHM_NAME);
            sem_close(sem_parent);
            sem_close(sem_child);
            sem_unlink(SEM_PARENT);
            sem_unlink(SEM_CHILD);
            close(fd);
        }
    }

    return 0;
}