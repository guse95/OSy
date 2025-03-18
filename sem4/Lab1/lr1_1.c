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

enum {
    TIME,
    DATE,
    HOWMUCH,
    LOGOUT,
    SANCTIONS
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

int CommandValid(const char* command) {
    if (strcmp(command, "Time") == 0) {return TIME;}
    if (strcmp(command, "Date") == 0) {return DATE;}
    if (strcmp(command, "Howmuch") == 0) {return HOWMUCH;}
    if (strcmp(command, "Logout") == 0) {return LOGOUT;}
    if (strcmp(command, "Sanctions") == 0) {return SANCTIONS;}
    return -1;
}

void Time() {
    time_t t;
    time(&t);
    const struct tm *tm = localtime(&t);

    char buf[80];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    printf("%s\n", buf);
}

void Date() {
    time_t t;
    time(&t);
    const struct tm *tm = localtime(&t);

    char buf[80];
    strftime(buf, sizeof(buf), "%d:%m:%Y", tm);
    printf("%s\n", buf);
}

int main() {
    printf("Yo\nYou have 3 ways:\n1 - Sign in\n2 - Create an account\n3 - Quit\n");
    printf("Enter your choice: ");
    char code[2];
    // size_t len;

    struct Data* base = (struct Data*)malloc(sizeof(struct Data));
    base->capacity = 2;
    base->size = 0;
    base->users = (struct User*)malloc(sizeof(struct User) * base->capacity);
    int in_account = 0;

    while (1) {
        if (scanf("%s", code) != 1) {
            HandlingError(ERROR_READ);
            return ERROR_READ;
        }
        if (strlen(code) > 1 || (code[0] != '1' && code[0] != '2' && code[0] != '3')) {
            printf("Unavailable code. Try again:\n");
            continue;
        }
        break;
    }

    if (code[0] == '3') {
        return SUCCESS;
    }
    printf("Enter login: ");
    char login[10];
    if (scanf("%s", login) != 1) {
        HandlingError(ERROR_READ);
        return ERROR_READ;
    }
    int err;
    if ((err = LoginValid(login))) {
        HandlingError(err);
        return err;
    }
    printf("Enter pincode: ");

    char pin[10];

    if (scanf("%s", pin) != 1) {
        HandlingError(ERROR_READ);
        return ERROR_READ;
    }
    if ((err = PinValid(pin))) {
        HandlingError(err);
        return err;
    }
    const int pincode = atoi(pin);

    switch (code[0]) {
        case '1': {
            if ((err = SignIn(login, pincode, base))) {
                HandlingError(err);
            } else {
                in_account = 1;
            }
            break;
        }
        case '2': {
            if ((err = CreateAcc(login, pincode, base))) {
                HandlingError(err);
            } else {
                in_account = 1;
            }
            break;
        }
        default: {
            printf("Something wrong.\n");
            break;
        }
    }

    if (in_account) {
        printf("Available commands: Time, Date, Howmuch, Logout, Sanctions\nEnter command: ");
        char command[10];
        if (scanf("%s", command) != 1) {
            HandlingError(ERROR_READ);
            return ERROR_READ;
        }
        const int com = CommandValid(command);
        switch (com) {
            case TIME: {
                Time();
                break;
            }
            case DATE: {
                Date();
                break;
            }
            case HOWMUCH: {}
            case LOGOUT: {}
            case SANCTIONS: {}
            default:
                printf("Something wrong.\n");
        }
    }
}