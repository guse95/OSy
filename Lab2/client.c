#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>


// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int num_threads;
    int *array;
    int ar_len;
} thread_data;

void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// Функция, выполняемая потоками
void* BetcherSort(void* arg) {
    thread_data *data = (thread_data *)arg;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    int *array = data->array;
    int len = data->ar_len;

    for (size_t i = 0; i < (len / num_threads + 1); i++) {
        for (size_t j = (i % 2) ? 0 : 1; j + 1 < len; j += 2) {
            if (array[j] > array[j + 1]) {
                swap(array + j, array + j + 1);
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {
    char buf[1];

    char *int_in_str = (char*) malloc(sizeof (char));
    if (int_in_str == NULL) {
        const char msg[] = "error: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    int int_in_str_size = 1;
    int ind_str = 0;


    ssize_t bytes;

    int *array = (int*) malloc(sizeof (int));
    if (array == NULL) {
        const char msg[] = "error: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    int array_size = 1;
    int ind_array = 0;

    pid_t pid = getpid();

    {
        char msg[128];
        int32_t len = snprintf(msg, sizeof(msg) - 1,
                               "%d: Start typing integers. Press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
        write(STDOUT_FILENO, msg, len);
    }
    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) != 0) {
        if (bytes < 0) {
            free(int_in_str);
            free(array);
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        if (isdigit(buf[0]) || (buf[0] == '-' && ind_str == 0)) {
            int_in_str[ind_str] = buf[0];
            ind_str++;

            if (ind_str >= int_in_str_size) {
                char *ptr;
                int_in_str_size *= 2;
                ptr = (char*) realloc(int_in_str, int_in_str_size * sizeof(char));
                if (ptr == NULL) {
                    free(int_in_str);
                    free(array);
                    const char msg[] = "error: failed to reallocate memory\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
                int_in_str = ptr;
            }
        } else {
            if (ind_str == 0) {
                continue;
            }
            int_in_str[ind_str] = '\0';

            array[ind_array] = atoi(int_in_str);
            ind_array++;

            if (ind_array >= array_size) {
                array_size *= 2;
                int *ptr;
                ptr = (int*) realloc(array, array_size * sizeof(int));
                if (ptr == NULL) {
                    free(array);
                    free(int_in_str);
                    const char msg[] = "error: failed to reallocate memory\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
                array = ptr;
            }

            ind_str = 0;
        }

        if (buf[0] == '\n' || buf[0] == EOF) {
            break;
        }
    }
    free(int_in_str);

    int num_of_threads = atoi(argv[1]);

    pthread_t threads[num_of_threads];
    thread_data thread_data_array[num_of_threads];

    for (int i = 0; i < num_of_threads; i++) {
        thread_data_array[i].thread_id = i;
        thread_data_array[i].num_threads = num_of_threads;
        thread_data_array[i].array = array;
        thread_data_array[i].ar_len = ind_array;
        if (pthread_create(&threads[i], NULL, BetcherSort, (void*)&thread_data_array[i]) != 0) {
            const char msg[] = "error: failed to create thread\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Вывод результата
    const char msg[] = "\nSorted array:\n";
    write(STDOUT_FILENO, msg, sizeof(msg));
    char ans[20];
    for (int i = 0; i < ind_array; ++i) {
        size_t ansLen = snprintf(ans, sizeof(ans), ((i == (ind_array - 1))? "%d\n" : "%d "), array[i]);
        int32_t written = write(STDOUT_FILENO, ans, ansLen);

        if (written != ansLen) {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
    }
    free(array);
    return 0;
}