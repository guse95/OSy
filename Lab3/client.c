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
    // Проверка количества аргументов
    if (argc != 2) {
        fprintf(stderr, "Usage: %s output_file\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Открытие общей памяти
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    // Отображение общей памяти в адресное пространство процесса
    char *ptr = (char *)mmap(NULL, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    // Создание семафоров
    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT, 0666, 0);
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT, 0666, 0);

    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        munmap(ptr, BUFSIZ);
        close(fd);
        return EXIT_FAILURE;
    }

    pid_t pid = getpid();
    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (file == -1) {
        perror("open");
        sem_close(sem_parent);
        sem_close(sem_child);
        munmap(ptr, BUFSIZ);
        close(fd);
        return EXIT_FAILURE;
    }

    {
        char msg[128];
        int32_t len = snprintf(msg, sizeof(msg) - 1,
                               "%d: Start typing lines of text. Press 'Ctrl-D' with no input to exit\n", pid);
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

        // Обработка строки чисел
        char *token = strtok(buf, " ");
        while (token != NULL) {
            char *endptr;
            double num = strtod(token, &endptr);

            if (*endptr != '\0') {
                fprintf(stderr, "error: value is not a number\n");
                sem_post(sem_parent);
                continue;
            }

            sum += num;
            token = strtok(NULL, " ");
        }

        // Запись суммы в файл
        size_t ansLen = snprintf(ans, sizeof(ans), "%.5f\n", sum);
        int32_t written = write(file, ans, ansLen);
        if (written != ansLen) {
            perror("write");
            sem_post(sem_parent);
            continue;
        }

        sem_post(sem_parent);
    }

    // Завершение записи в файл
    const char term = '\0';
    write(file, &term, sizeof(term));

    // Освобождение ресурсов
    munmap(ptr, BUFSIZ);
    close(fd);
    sem_close(sem_parent);
    sem_close(sem_child);
    close(file);
}