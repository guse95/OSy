#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
    SUCCESS,
    ERROR_WRONG_LOGIN,
    ERROR_USER_EXISTS,
    ERROR_WRONG_LOGIN_FORMAT,
    ERROR_WRONG_LOGIN_LENGTH,
    ERROR_WRONG_PASSWORD,
    ERROR_WRONG_PASSWORD_FORMAT,
    MEMORY_ALLOCATION_ERROR,
    ERROR_READ
};

void HandlingError(const int code) {
    switch (code) {
        case SUCCESS:
            printf("SUCCESS.\n"); break;
        case ERROR_WRONG_LOGIN:
            printf("No such user.\n"); break;
        case ERROR_USER_EXISTS:
            printf("User already exists.\n"); break;
        case ERROR_WRONG_LOGIN_LENGTH:
            printf("Login length should be no more than 6 simbols.\n"); break;
        case ERROR_WRONG_LOGIN_FORMAT:
            printf("Login can contain only latin alphabet symbols and numbers.\n"); break;
        case ERROR_WRONG_PASSWORD_FORMAT:
            printf("Pin-code should be decimal number from 0 to 100000.\n"); break;
        case ERROR_WRONG_PASSWORD:
            printf("Pin-code is incorrect.\n"); break;
        case MEMORY_ALLOCATION_ERROR:
            printf("Allocation failure.\n"); break;
        case ERROR_READ:
            printf("Reading failure.\n"); break;
        default:
            printf("Unknown error.\n"); break;
    }
}

struct User {
    char* login;
    int pin;
    int sanctions;
};

struct Data {
    struct User* users;
    int size;
    int capacity;
};

int LoginValid(const char* login) {
    int len = 0;
    while (*login != '\0') {
        len++;
        if (!isalnum(*login)) {
            return ERROR_WRONG_LOGIN_FORMAT;
        }
        if (len > 6) {
            return ERROR_WRONG_LOGIN_LENGTH;
        }
        login++;
    }
    return SUCCESS;
}

int PinValid(const char* word) {
    int len = 0;
    while (*word != '\0') {
        len++;
        if (!isdigit(*word) || len >= 6) {
            return ERROR_WRONG_PASSWORD_FORMAT;
        }
        word++;
    }
    return SUCCESS;
}

int CreateAcc(char* login, const int pin, struct Data* base) {
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login, login) == 0) {
            return ERROR_USER_EXISTS;
        }
    }
    base->size++;
    if (base->size > base->capacity) {
        base->capacity *= 2;
        struct User* ptr = realloc(base->users, base->capacity * sizeof(struct User));
        if (ptr == NULL) {
            return MEMORY_ALLOCATION_ERROR;
        }
        base->users = ptr;
    }
    base->users[base->size - 1].login = login; // возможно нужно сделать копирование
    base->users[base->size - 1].pin = pin;
    base->users[base->size - 1].sanctions = -1;
    return SUCCESS;
}

int SignIn(const char* login, const int pin, struct Data* base) {
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login, login) == 0) {
            if (base->users[i].pin == pin) {
                return SUCCESS;
            }
            return ERROR_WRONG_PASSWORD;
        }
    }
    return ERROR_WRONG_LOGIN;
}

int main() {
    // char test[] = "123";
    // HandlingError(LoginValid(test));
    // Валидацию в мейне потом регистрация и вход
    printf("Yo\nYou have 2 ways:\n1 - Sign in\n2 - Create an account");
    char* code = NULL;
    size_t len;

    if (getline(&code, &len, stdin) == -1) {
        HandlingError(ERROR_READ);
        return ERROR_READ;
    }

    if (len != 2 || (code[0] != '1' && code[0] != '2')) {
        printf("Unavailable code. Try again:\n");
    }
}