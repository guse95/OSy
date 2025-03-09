#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>


typedef struct {
    int num_threads;
    clock_t begin;
    int *array;
    int ar_len;
} thread_data;

void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void* BetcherSort(void* arg) {
    thread_data *data = (thread_data *)arg;
    const int num_threads = data->num_threads;
    int *array = data->array;
    const int len = data->ar_len;
    const clock_t begin = data->begin;

    for (size_t i = 0; i < (len / num_threads + 1); i++) {
        for (size_t j = (i % 2) ? 0 : 1; j + 1 < len; j += 2) {
            if (array[j] > array[j + 1]) {
                swap(array + j, array + j + 1);
            }
        }
        // if ((float)(clock() - begin)/CLOCKS_PER_SEC > 1) {
        //     const char msg[] = "error: too long\n";
        //     write(STDERR_FILENO, msg, sizeof(msg));
        //     exit(EXIT_FAILURE);
        // }
    }


    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char **argv) {
    // char buf[1];
    //
    // char *int_in_str = (char*) malloc(sizeof (char));
    // if (int_in_str == NULL) {
    //     const char msg[] = "error: failed to allocate memory\n";
    //     write(STDERR_FILENO, msg, sizeof(msg));
    //     exit(EXIT_FAILURE);
    // }
    // int int_in_str_size = 1;
    // int ind_str = 0;


    ssize_t bytes;

    int ind_array = atoi(argv[2]);
    int *array = (int*) malloc(ind_array * sizeof (int));
    if (array == NULL) {
        const char msg[] = "error: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }
    // int array_size = 1;
    const clock_t begin = clock();
    clock_t end;

    srand(time(NULL));
    for (int i = 0; i < ind_array; i++) {
        array[i] = rand() % 2000 - 1000;
        end = clock();
        // if (((float)(end - begin))/CLOCKS_PER_SEC > 1) {
        //     const char msg[] = "error: too long\n";
        //     write(STDERR_FILENO, msg, sizeof(msg));
        //     exit(EXIT_FAILURE);
        // }
    }

    // pid_t pid = getpid();

    // {
    //     char msg[128];
    //     int32_t len = snprintf(msg, sizeof(msg) - 1,
    //                            "%d: Start typing integers. Press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
    //     write(STDOUT_FILENO, msg, len);
    // }
    // while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) != 0) {
    //     if (bytes < 0) {
    //         free(int_in_str);
    //         free(array);
    //         const char msg[] = "error: failed to read from stdin\n";
    //         write(STDERR_FILENO, msg, sizeof(msg));
    //         exit(EXIT_FAILURE);
    //     }
    //
    //     if (isdigit(buf[0]) || (buf[0] == '-' && ind_str == 0)) {
    //         int_in_str[ind_str] = buf[0];
    //         ind_str++;
    //
    //         if (ind_str >= int_in_str_size) {
    //             char *ptr;
    //             int_in_str_size *= 2;
    //             ptr = (char*) realloc(int_in_str, int_in_str_size * sizeof(char));
    //             if (ptr == NULL) {
    //                 free(int_in_str);
    //                 free(array);
    //                 const char msg[] = "error: failed to reallocate memory\n";
    //                 write(STDERR_FILENO, msg, sizeof(msg));
    //                 exit(EXIT_FAILURE);
    //             }
    //             int_in_str = ptr;
    //         }
    //     } else {
    //         if (ind_str == 0) {
    //             continue;
    //         }
    //         int_in_str[ind_str] = '\0';
    //
    //         array[ind_array] = atoi(int_in_str);
    //         ind_array++;
    //
    //         if (ind_array >= array_size) {
    //             array_size *= 2;
    //             int *ptr;
    //             ptr = (int*) realloc(array, array_size * sizeof(int));
    //             if (ptr == NULL) {
    //                 free(array);
    //                 free(int_in_str);
    //                 const char msg[] = "error: failed to reallocate memory\n";
    //                 write(STDERR_FILENO, msg, sizeof(msg));
    //                 exit(EXIT_FAILURE);
    //             }
    //             array = ptr;
    //         }
    //
    //         ind_str = 0;
    //     }
    //
    //     if (buf[0] == '\n' || buf[0] == EOF) {
    //         break;
    //     }
    // }
    // free(int_in_str);

    int num_of_threads = atoi(argv[1]);

    pthread_t threads[num_of_threads];
    thread_data thread_data_array[num_of_threads];

    for (int i = 0; i < num_of_threads; i++) {
        thread_data_array[i].num_threads = num_of_threads;
        thread_data_array[i].array = array;
        thread_data_array[i].ar_len = ind_array;
        thread_data_array[i].begin = begin;
        if (pthread_create(&threads[i], NULL, BetcherSort, (void*)&thread_data_array[i]) != 0) {
            const char msg[] = "error: failed to create thread\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        end = clock();

        // if (((float)(end - begin))/CLOCKS_PER_SEC > 1) {
        //     const char msg[] = "error: too long\n";
        //     write(STDERR_FILENO, msg, sizeof(msg));
        //     exit(EXIT_FAILURE);
        // }
    }

    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // const char msg[] = "\nSorted array:\n";
    // write(STDOUT_FILENO, msg, sizeof(msg));
    // char ans[20];
    // for (int i = 0; i < ind_array; ++i) {
    //     size_t ansLen = snprintf(ans, sizeof(ans), ((i == (ind_array - 1))? "%d\n" : "%d "), array[i]);
    //     int32_t written = write(STDOUT_FILENO, ans, ansLen);
    //
    //     if (written != ansLen) {
    //         const char msg[] = "error: failed to write to file\n";
    //         write(STDERR_FILENO, msg, sizeof(msg));
    //         exit(EXIT_FAILURE);
    //     }
    // }

    printf("You can use 'ps -T -p %d' to see the threads of this process.\n", getpid());
    free(array);
    return 0;
}