#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <crypt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

enum {
    SUCCESS,
    ERROR_WRONG_LOGIN,
    ERROR_USER_EXISTS,
    ERROR_WRONG_LOGIN_FORMAT,
    ERROR_WRONG_LOGIN_LENGTH,
    ERROR_WRONG_PASSWORD,
    ERROR_WRONG_PASSWORD_FORMAT,
    ERROR_HOWMUCH_FORMAT,
    TIME_UNDER_1900,
    TIME_IS_FUTURE,
    ERROR_SANCTIONS_FORMAT,
    MEMORY_ALLOCATION_ERROR,
    MEMORY_MAPPING_ERROR,
    ERROR_READ,
    FILE_OPEN_ERROR,
    STAT_ERROR,
    FTRUNCATE_ERROR,
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
        case ERROR_HOWMUCH_FORMAT:
            printf("Format should be: Howmuch hh:mm:ss "
                   "DD:MM:YYYY <flag>(possible: -s, -m, -h, -y)"); break;
        case TIME_UNDER_1900:
            printf("Time under 1900 seconds is incorrect.\n"); break;
        case TIME_IS_FUTURE:
            printf("Time is future.\n"); break;
        case ERROR_SANCTIONS_FORMAT:
            printf("Format should be: Sanctions username "
                   "<number>(from 1 to 5)"); break;
        case MEMORY_ALLOCATION_ERROR:
            printf("Allocation failure.\n"); break;
        case MEMORY_MAPPING_ERROR:
            printf("Mapping failure.\n"); break;
        case ERROR_READ:
            printf("Reading failure.\n"); break;
        case FILE_OPEN_ERROR:
            printf("File open failure.\n"); break;
        case STAT_ERROR:
            printf("Stat failure.\n"); break;
        case FTRUNCATE_ERROR:
            printf("Ftruncate failure.\n"); break;
        default:
            printf("Unknown error.\n"); break;
    }
}

struct string {
    char *str;
    size_t len;
};

struct User {
    struct string login;
    struct string pin;
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

// int encrypt_password(const char *password, char **encrypted_password)
// {
//
//     char *salt = crypt_gensalt_ra(NULL, 0, NULL, 0);
//     if (!salt)
//     {
//         return -1;
//     }
//
//     void *enc_ctx = NULL;
//     int enc_cxt_sz = 0;
//     char *tmp_encrypted_password = crypt_ra(password, salt, &enc_ctx, &enc_cxt_sz);
//
//     if (tmp_encrypted_password == NULL)
//     {
//         free(salt);
//         return -1;
//     }
//
//     *encrypted_password = (char *)calloc((strlen(tmp_encrypted_password) + 1), sizeof(char));
//
//     strcpy(*encrypted_password, tmp_encrypted_password);
//
//     free(enc_ctx);
//     free(salt);
//     return 0;
// }
//
// int compare_passwords(const char *password, const char *hashed_password, int *compare_res)
// {
//
//     int cmp_res = 0;
//
//     void *enc_ctx = NULL;
//     int enc_cxt_sz = 0;
//
//     char *hashed_entered_password = crypt_ra(password, hashed_password, &enc_ctx, &enc_cxt_sz);
//     if (!hashed_entered_password)
//     {
//         return -1;
//     }
//
//     *compare_res = strcmp(hashed_password, hashed_entered_password);
//
//     free(enc_ctx);
//     return 0;
// }

int CreateAcc(const char* login, const int pin, struct Data* base, int fd) {
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login.str, login) == 0) {
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

    base->users[base->size - 1].login.str = strdup(login);
    if (base->users[base->size - 1].login.str == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    base->users[base->size - 1].login.len = strlen(login);
    base->users[base->size - 1].pin = pin;
    base->users[base->size - 1].sanctions = -1;
    return SUCCESS;
}

int SignIn(const char* login, const int pin, struct Data* base) {
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login.str, login) == 0) {
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

    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    printf("%s\n", buf);
}

void Date() {
    time_t t;
    time(&t);
    const struct tm *tm = localtime(&t);

    char buf[20];
    strftime(buf, sizeof(buf), "%d:%m:%Y", tm);
    printf("%s\n", buf);
}

int TimeValid(const char* ptr) {
    if (strlen(ptr) != 10 || strlen(ptr) != 8 || ptr[2] != ':' || ptr[5] != ':') {
        return ERROR_HOWMUCH_FORMAT;
    }
    while (*ptr != '\0') {
        if (!isdigit(*ptr) && *ptr != ':') {
            return ERROR_HOWMUCH_FORMAT;
        }
    }
    return SUCCESS;
}

int AtoiLR(const char *str, int l, int r) {
    int res = 0;
    while (l <= r) {
        res = res * 10 + (str[l++] - '0');
    }
    return res;
}

int ToTimeT(const char* ptr, time_t* timeptr) {
    struct tm t;
    t.tm_year = AtoiLR(ptr, 15,18) - 1900;
    t.tm_mon = AtoiLR(ptr, 12,13) - 1;
    t.tm_mday = AtoiLR(ptr, 9,10);
    t.tm_hour = AtoiLR(ptr, 0,1);
    t.tm_min = AtoiLR(ptr, 3,4);
    t.tm_sec = AtoiLR(ptr, 6,7);

    const time_t now = time(NULL);
    const struct tm *tn = localtime(&now);

    if (t.tm_year < 0) {
        return TIME_UNDER_1900;
    }

    if (tn->tm_year - t.tm_year < 0 || tn->tm_mon - t.tm_mon < 0 || tn->tm_mday - t.tm_mday < 0 ||
        tn->tm_hour - t.tm_hour < 0 || tn->tm_min - t.tm_min < 0 || tn->tm_sec - t.tm_sec < 0) {
        return TIME_IS_FUTURE;
        }

    *timeptr = mktime(&t);

    return 0;
}

int Howmuch(char* input) {
    if (strlen(input) != strlen("Howmuch hh:mm:ss DD:MM:YYYY -f")) {
        return ERROR_HOWMUCH_FORMAT;
    }

    char* ptr = input;
    while (*ptr != ' ') {
        ptr++;
    }
    char *time_str = ptr + 1;
    while (*ptr != ' ' || *ptr == '\0') {
        ptr++;
    }
    ++ptr;
    while (*ptr != ' ' || *ptr == '\0') {
        ptr++;
    }
    if (*ptr == '\0' || *(ptr + 1) != '-') {
        return ERROR_HOWMUCH_FORMAT;
    }
    *ptr = '\0';
    ptr += 2;
    int err;
    if ((err = TimeValid(time_str))) {
        return err;
    }

    time_t* past = (time_t*)malloc(sizeof(time_t));
    if (past == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    if ((err = ToTimeT(ptr, past))) {
        free(past);
        return err;
    }

    time_t now;
    time(&now);
    const double dist = difftime(now, *past);
    free(past);
    switch (*ptr) {
        case 's': {
            printf("Seconds passed: %d\n", (int)dist);
            break;
        }
        case 'm': {
            printf("Minutes passed: %.2f\n", (dist/60));
            break;
        }
        case 'h': {
            printf("Hours passed: %.2f\n", (dist/3600));
            break;
        }
        case 'y': {
            printf("Years passed: %.2f\n", (dist/3600/24/365));
            break;
        }
        default: {
            printf("Unknown flag.\n");
            return ERROR_HOWMUCH_FORMAT;
        }
    }
    return SUCCESS;
}

int Sanctions(char* command) {
    char* lexem = strtok(command, " ");
    lexem = strtok(NULL, " ");
    if (lexem == NULL) {
        return ERROR_SANCTIONS_FORMAT;
    }
    //записать санкции в файл
    return SUCCESS;
}


int InitDataFromFile(struct Data* base, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return FILE_OPEN_ERROR;
    }

    fread(&base->size, sizeof(int), 1, file);
    fread(&base->capacity, sizeof(int), 1, file);

    base->users = (struct User*)malloc(base->capacity * sizeof(struct User));
    if (!base->users) {
        fclose(file);
        return MEMORY_ALLOCATION_ERROR;
    }

    for (int i = 0; i < base->size; ++i) {
        struct User user;
        fread(&user.login.len, sizeof(int), 1, file);
        // user.login.str = (char*)malloc(user.login.len + 1);
        // if (user.login.str == NULL) {
        //     fclose(file);
        //     return MEMORY_ALLOCATION_ERROR;
        // }
        fread(&user.login.str, sizeof(char) * user.login.len, 1, file);
        fread(&user.pin, sizeof(int), 1, file);
        fread(&user.sanctions, sizeof(int), 1, file);

        base->users[i].login.str = strdup(user.login.str);
        base->users[i].login.len = user.login.len;
        base->users[i].pin = user.pin;
        base->users[i].sanctions = user.sanctions;
    }

    fclose(file);
    return SUCCESS;
}

int SaveDataToFile(const struct Data* base, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return FILE_OPEN_ERROR;
    }

    fwrite(&base->size, sizeof(int), 1, file);
    fwrite(&base->capacity, sizeof(int), 1, file);

    for (int i = 0; i < base->size; ++i) {
        fwrite(&(base->users[i].login.len), sizeof(int), 1, file);
        fwrite(&(base->users[i].login.str), sizeof(char) * base->users[i].login.len, 1, file);
        fwrite(&(base->users[i].pin), sizeof(int), 1, file);
        fwrite(&(base->users[i].sanctions), sizeof(int), 1, file);
    }

    fclose(file);
    return SUCCESS;
}

void FreeUsers(struct Data* base) {
    for (int i = 0; i < base->size; ++i) {
        free(base->users[i].login);
    }
    free(base->users);
}

int main() {
    int in_account = 0;

    struct Data* base = (struct Data*)malloc(sizeof(struct Data));
    if (base == NULL) {
        HandlingError(MEMORY_ALLOCATION_ERROR);
        return MEMORY_ALLOCATION_ERROR;
    }

    base->capacity = 2;
    base->size = 0;
    base->users = (struct User*)malloc(sizeof(struct User) * base->capacity);

    while (1) {
        char code[2];
        printf("Yo\nYou have 3 ways:\n1 - Sign in\n2 - Create an account\n3 - Quit\n");
        printf("Enter your choice:");
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

        printf("Enter login:");
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

        printf("Enter pincode:");
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

        if (code[0] == '1') {
            if ((err = SignIn(login, pincode, base))) {
                HandlingError(err);
                break;
            }
            in_account = 1;
        }
        else if (code[0] == '2') {
            if ((err = CreateAcc(login, pincode, base, fd))) {
                HandlingError(err);
                break;
            }
            in_account = 1;
        }
        else {
            printf("||%s||\n", code);
            printf("Something wrong.\n");
            break;
        }

        while (in_account) {
            printf("Available commands:\nTime\nDate\nHowmuch hh:mm:ss "
                   "DD:MM:YYYY <flag>(possible: -s, -m, -h, -y)\nLogout\n"
                   "Sanctions username <number>(from 1 to 5)\nEnter command: ");
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
                case HOWMUCH: {
                    Howmuch(command);
                    break;
                }
                case LOGOUT: {
                    in_account = 0;
                    break;
                }
                case SANCTIONS: {
                    Sanctions(command);
                    break;
                }
                default: {
                    printf("Unknown command.\n");
                    break;
                }
            }
        }
    }
    close(fd);
    munmap(file_data, file_size);
}