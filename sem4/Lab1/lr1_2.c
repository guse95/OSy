#include <stdio.h>
#include <string.h>

enum {
    XOR,
    MASK,
    COPY,
    FIND,
    SUCCESS = 0,
    ERROR,
};

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

int main(int argc, char* argv[]) {

    switch (CommandValid(argv[argc - 1])) {
        case XOR: {}
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