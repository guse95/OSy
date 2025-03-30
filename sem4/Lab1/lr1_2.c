#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

enum {
    XOR,
    MASK,
    COPY,
    FIND,
    SUCCESS = 0,
    ERROR,
    ERROR_UNKNOWN_FLAG,
    ERROR_NOT_ENOUGH_ARGUMENTS,
    ERROR_FILE_OPENING,
    ERROR_FILE_READING,
};

void HandlingError(const int code) {
    switch (code) {
        case SUCCESS: {
            printf("SUCCESS.\n");
            break;
        }
        case ERROR_FILE_OPENING: {
            printf("File opening error.\n");
            break;
        }
        case ERROR_FILE_READING: {
            printf("File reading error.\n");
            break;
        }
        case ERROR_UNKNOWN_FLAG: {
            printf("Unknown flag.\n");
            break;
        }
        case ERROR_NOT_ENOUGH_ARGUMENTS: {
            printf("Not enough arguments.\n");
            break;
        }
        default: {
            printf("ERROR:%d\n", code);
            break;
        }
    }
}

int contain(const char* string, const char* word) {
    for (int i = 0; word[i] != '\0'; i++) {
        if (string[i] != word[i]) {
            return -1;
        }
    }
    return 0;
}

int CommandValid(const char* command) {
    if (contain(command, "xor") == 0) {return XOR;}
    if (contain(command, "mask ") == 0) {return MASK;}
    if (contain(command, "copy") == 0) {return COPY;}
    if (contain(command, "find ") == 0) {return FIND;}
    return -1;
}

void xorStr(char* res, const char* str1) {
    for (int i = 0; str1[i] != '\0'; i++) {
        res[i] = (char)(str1[i] != res[i] + '0');
    }
}

int xor(const int argc, char** argv) {

    if ((strlen(argv[argc - 1]) != 4) || !isdigit(argv[argc - 1][3])
        || (argv[argc - 1][3] > '6') || (argv[argc - 1][3] < '2')) {
        return ERROR_UNKNOWN_FLAG;
    }
    const int N = (int)(argv[argc - 1] - '0');
    char block[1<<N + 1], result[1<<N + 1];
    block[1<<N] = '\0';
    result[1<<N] = '\0';


    for (int i = 1; i < (argc - 1); i++) {
        for (int j = 0; j < 1<<N; j++) {
            result[j] = '0';
        }
        const int fd = open(argv[i], O_RDONLY); // Бинарное чтение
        if (fd == -1) {
            return ERROR_FILE_OPENING;
        }
        int cnt;
        while ((cnt = read(fd, block, 1<<N)) > 0) { // Бинарное чтение
            block[cnt] = '\0';
            xorStr(result, block);
        }
        if (cnt == -1) {
            return ERROR_FILE_READING;
        }
        printf("For file %s result of <xor%d>: %s\n", argv[i], N, result);
        close(fd);
    }
    return SUCCESS;
}

int main(const int argc, char* argv[]) {
    if (argc < 3) {
        HandlingError(ERROR_NOT_ENOUGH_ARGUMENTS);
        return ERROR_NOT_ENOUGH_ARGUMENTS;
    }
    int err;
    switch (CommandValid(argv[argc - 1])) {
        case XOR: {
            if ((err = xor(argc, argv))) {
                HandlingError(err);
                return err;
            }
            break;
        }
        case MASK: {}
        case COPY: {}
        case FIND: {}
        case -1: {
            printf("Unknown flag.\n");
            return SUCCESS;
        }
        default: {
            printf("Somthing wrong.\n");
            return ERROR;
        }
    }
}