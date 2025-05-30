#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <time.h>
#include <unistd.h>

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
    ERROR_NO_LICENSE,
    MEMORY_ALLOCATION_ERROR,
    ERROR_FILE_READING,
    ERROR_FILE_WRITING,
    ERROR_READ,
    FILE_OPEN_ERROR,
    ERROR_UNABLE_TO_ENCRYPT,
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
                   "DD:MM:YYYY <flag>(possible: -s, -m, -h, -y)\n"); break;
        case TIME_UNDER_1900:
            printf("Time under 1900 seconds is incorrect.\n"); break;
        case TIME_IS_FUTURE:
            printf("Time is future.\n"); break;
        case ERROR_SANCTIONS_FORMAT:
            printf("Format should be: Sanctions username "
                   "<number>(from 1 to 5)"); break;
        case ERROR_NO_LICENSE:
            printf("You cant change sanctions for yourself.\n"); break;
        case MEMORY_ALLOCATION_ERROR:
            printf("Allocation failure.\n"); break;
        case ERROR_FILE_READING:
            printf("File reading error.\n"); break;
        case ERROR_FILE_WRITING:
            printf("File write error.\n"); break;
        case ERROR_READ:
            printf("Reading failure.\n"); break;
        case FILE_OPEN_ERROR:
            printf("File open failure.\n"); break;
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
    struct string pin; // struct string
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

int encrypt_password(const char *password, char **encrypted_password) {

    char *salt = crypt_gensalt_ra(NULL, 0, NULL, 0);
    if (!salt)
    {
        return ERROR_UNABLE_TO_ENCRYPT;
    }

    void *enc_ctx = NULL;
    int enc_cxt_sz = 0;
    char *tmp_encrypted_password = crypt_ra(password, salt, &enc_ctx, &enc_cxt_sz);

    if (tmp_encrypted_password == NULL)
    {
        free(salt);
        return ERROR_UNABLE_TO_ENCRYPT;
    }

    *encrypted_password = (char *)calloc((strlen(tmp_encrypted_password) + 1), sizeof(char));
    if (!(*encrypted_password)) {
        free(salt);
        free(enc_ctx);
        return MEMORY_ALLOCATION_ERROR;
    }

    strcpy(*encrypted_password, tmp_encrypted_password);

    free(enc_ctx);
    free(salt);
    return SUCCESS;
}

int compare_passwords(const char *password, const char *hashed_password, int *compare_res)
{
    void *enc_ctx = NULL;
    int enc_cxt_sz = 0;

    char *hashed_entered_password = crypt_ra(password, hashed_password, &enc_ctx, &enc_cxt_sz);
    if (!hashed_entered_password)
    {
        return MEMORY_ALLOCATION_ERROR;
    }

    *compare_res = strcmp(hashed_password, hashed_entered_password);

    free(enc_ctx);
    return SUCCESS;
}

int CreateAcc(const char* login, const char* pin, struct Data* base, int* Id) {
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login.str, login) == 0) {
            return ERROR_USER_EXISTS;
        }
    }
    base->size++;
    if (base->size > base->capacity) {
        base->capacity *= 2;
        struct User* ptr = (struct User*)realloc(base->users, base->capacity * sizeof(struct User));
        if (ptr == NULL) {
            return MEMORY_ALLOCATION_ERROR;
        }
        base->users = ptr;
    }

    base->users[base->size - 1].login.str = strdup(login);
    if (base->users[base->size - 1].login.str == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    char* ePin;
    int err;
    if ((err = encrypt_password(pin, &ePin))) {
        return err;
    }
    base->users[base->size - 1].login.len = strlen(login);
    base->users[base->size - 1].pin.str = ePin;
    base->users[base->size - 1].pin.len = strlen(ePin);
    base->users[base->size - 1].sanctions = -1;
    *Id = base->size - 1;
    return SUCCESS;
}

int SignIn(const char* login, const char* pin, const struct Data* base, int* Id) {
    if (base->size == 0 || base->users == NULL) {
        return ERROR_WRONG_LOGIN;
    }
    int cmp_res = 0;
    int err;
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login.str, login) == 0) {
            if ((err = compare_passwords(pin, base->users[i].pin.str, &cmp_res))) {
                return err;
            }
            if (!cmp_res) {
                *Id = i;
                return SUCCESS;
            }
            return ERROR_WRONG_PASSWORD;
        }
    }
    return ERROR_WRONG_LOGIN;
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
    if (strcmp(command, "Time") == 0) {return TIME;}
    if (strcmp(command, "Date") == 0) {return DATE;}
    if (contain(command, "Howmuch") == 0) {return HOWMUCH;}
    if (strcmp(command, "Logout") == 0) {return LOGOUT;}
    if (contain(command, "Sanctions") == 0) {return SANCTIONS;}
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
    if (strlen(ptr) != 19 || ptr[2] != ':' || ptr[5] != ':' || ptr[11] != ':' || ptr[14] != ':' || ptr[8] != ' ') {
        return ERROR_HOWMUCH_FORMAT;
    }

    for (int i = 0; ptr[i] != '\0'; i++) {
        if (!isdigit(ptr[i]) && (i != 2 && i != 5 && i != 11 && i != 14 && i != 8)) {
            return ERROR_HOWMUCH_FORMAT;
        }
    }
    return SUCCESS;
}

int AtoiLR(const char *str, int l, const int r) {
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

    char time_str[30];
    const char* t = strtok(input, " ");
    t = strtok(NULL, " ");
    const char* date = strtok(NULL, " ");
    const char* flag = strtok(NULL, " ");

    if (flag[0] != '-' || flag[2] != '\0') {
        return ERROR_HOWMUCH_FORMAT;
    }

    strcpy(time_str, t);
    strcat(time_str, " ");
    strcat(time_str, date);

    int err;
    if ((err = TimeValid(time_str))) {
        return err;
    }

    time_t* past = (time_t*)malloc(sizeof(time_t));
    if (past == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    if ((err = ToTimeT(time_str, past))) {
        free(past);
        return err;
    }
    time_t now;
    time(&now);
    const double dist = difftime(now, *past);

    free(past);

    switch (flag[1]) {
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

int Sanctions(char* command, const char* login, const struct Data* base) {
    const char* username = strtok(command, " ");

    username = strtok(NULL, " ");
    if (username == NULL || LoginValid(username)) {
        return ERROR_SANCTIONS_FORMAT;
    }
    if (strcmp(username, login) == 0) {
        return ERROR_NO_LICENSE;
    }
    const char* sanction = strtok(NULL, " ");
    if (sanction == NULL || (strlen(sanction) != 1 || ('1' < sanction[0] && sanction[0] > '5'))) {
        return ERROR_SANCTIONS_FORMAT;
    }
    if (base->size == 0 || base->users == NULL) {
        return ERROR_WRONG_LOGIN;
    }
    for (int i = 0; i < base->size; i++) {
        if (strcmp(base->users[i].login.str, username) == 0) {
            base->users[i].sanctions = sanction[0] - '0';
            return SUCCESS;
        }
    }
    return ERROR_WRONG_LOGIN;
}

int InitDataFromFile(struct Data* base, const char* filename) {
    if (access(filename, F_OK) == 0) {
        FILE* file = fopen(filename, "r");
        if (!file) {
            return FILE_OPEN_ERROR;
        }
        fread(&base->size, sizeof(int), 1, file);
        fread(&base->capacity, sizeof(int), 1, file);
        base->users = (struct User*)malloc(base->capacity * sizeof(struct User));
        if (base->users == NULL) {
            fclose(file);
            return MEMORY_ALLOCATION_ERROR;
        }
        for (int i = 0; i < base->size; ++i) {
            struct User user;
            {
                fread(&user.login.len, sizeof(int), 1, file);
                user.login.str = (char*)malloc(sizeof(char) * (user.login.len + 1));
                if (user.login.str == NULL) {
                    fclose(file);
                    for (int j = 0; j < i; ++j) {
                        free(base->users[j].login.str);
                        free(base->users[j].pin.str);
                    }
                    free(base->users);
                    return MEMORY_ALLOCATION_ERROR;
                }
                fread(user.login.str, sizeof(char), user.login.len, file);
                user.login.str[user.login.len] = '\0';
            }
            {
                fread(&user.pin.len, sizeof(int), 1, file);
                user.pin.str = (char*)malloc(sizeof(char) * (user.pin.len + 1));
                if (user.pin.str == NULL) {
                    fclose(file);
                    for (int j = 0; j < i; ++j) {
                        free(base->users[j].login.str);
                        free(base->users[j].pin.str);
                    }
                    free(base->users);
                    free(user.login.str); /// XD
                    return MEMORY_ALLOCATION_ERROR;
                }
                fread(user.pin.str, sizeof(char), user.pin.len, file);
                user.pin.str[user.pin.len] = '\0';
            }
            
            fread(&user.sanctions, sizeof(int), 1, file);

            base->users[i].login.str = user.login.str;
            base->users[i].login.len = user.login.len;
            base->users[i].pin.str = user.pin.str;
            base->users[i].pin.len = user.pin.len;
            base->users[i].sanctions = user.sanctions;
        }
        fclose(file);
    } else {
        base->capacity = 2;
        base->size = 0;
        base->users = (struct User*)malloc(sizeof(struct User) * base->capacity);
        if (base->users == NULL) {
            return MEMORY_ALLOCATION_ERROR;
        }
    }
    return SUCCESS;
}

int SaveDataToFile(const struct Data* base, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        return FILE_OPEN_ERROR;
    }

    fwrite(&base->size, sizeof(int), 1, file);
    fwrite(&base->capacity, sizeof(int), 1, file);
    if (base->users == NULL) {
        return SUCCESS;
    }

    for (int i = 0; i < base->size; ++i) {
        fwrite(&(base->users[i].login.len), sizeof(int), 1, file);
        fwrite(base->users[i].login.str, sizeof(char), base->users[i].login.len, file);

        fwrite(&(base->users[i].pin.len), sizeof(int), 1, file);
        fwrite(base->users[i].pin.str, sizeof(char), base->users[i].pin.len, file);

        fwrite(&(base->users[i].sanctions), sizeof(int), 1, file);
    }
    fclose(file);
    return SUCCESS;
}

void FreeUsers(struct Data* base) {
    if (base == NULL) {
        return;
    }
    if (base->users == NULL) {
        return;
    }
    for (int i = 0; i < base->size; ++i) {
        free(base->users[i].login.str);
        free(base->users[i].pin.str);
    }
    free(base->users);
    free(base);
}

int main() {
    struct Data* base = (struct Data*)malloc(sizeof(struct Data));
    if (base == NULL) {
        HandlingError(MEMORY_ALLOCATION_ERROR);
        return MEMORY_ALLOCATION_ERROR;
    }

    int err;
    if ((err = InitDataFromFile(base, "data.txt"))) {
        HandlingError(err);
        return err;
    }

    while (1) {
        char code[256];
        int in_account = 0;

        printf("Yo\nYou have 3 ways:\n1 - Sign in\n2 - Create an account\n3 - Quit\n");
        printf("Enter your choice:");

        while (1) {
            if (fgets(code, sizeof(code), stdin) == NULL) {
                HandlingError(ERROR_READ);
                if ((err = SaveDataToFile(base, "data.txt"))) {
                    HandlingError(err);
                    FreeUsers(base);
                    return err;
                }
                FreeUsers(base);
                return ERROR_READ;
            }
            // while ((c = getchar()) != '\n' && c != EOF) {}
            code[strcspn(code, "\n")] = '\0';
            if (strlen(code) > 1 || (code[0] != '1' && code[0] != '2' && code[0] != '3')) {
                printf("Unavailable code. Try again:\n");
                continue;
            }
            break;
        }
        if (code[0] == '3') {
            if ((err = SaveDataToFile(base, "data.txt"))) {
                HandlingError(err);
                FreeUsers(base);
                return err;
            }
            FreeUsers(base);
            return SUCCESS;
        }

        printf("Enter login:");
        char login[256];
        if (fgets(login, sizeof(login), stdin) == NULL) {
            HandlingError(ERROR_READ);
            if ((err = SaveDataToFile(base, "data.txt"))) {
                HandlingError(err);
                FreeUsers(base);
                return err;
            }
            FreeUsers(base);
            return ERROR_READ;
        }
        // while ((c = getchar()) != '\n' && c != EOF) {}
        login[strcspn(login, "\n")] = '\0';

        int userId = -1;
        if ((err = LoginValid(login))) {
            HandlingError(err);
            continue;
        }

        printf("Enter pincode:");
        char pin[256];
        if (fgets(pin, sizeof(pin), stdin) == NULL) {
            HandlingError(ERROR_READ);
            if ((err = SaveDataToFile(base, "data.txt"))) {
                HandlingError(err);
                FreeUsers(base);
                return err;
            }
            FreeUsers(base);
            return ERROR_READ;
        }
        // while ((c = getchar()) != '\n' && c != EOF) {}
        pin[strcspn(pin, "\n")] = '\0';
        if ((err = PinValid(pin))) {
            HandlingError(err);
            continue;
        }

        if (code[0] == '1') {
            err = SignIn(login, pin, base, &userId);
            HandlingError(err);
            if (err) {
                continue;
            }
            in_account = 1;
        }
        else if (code[0] == '2') {
            err = CreateAcc(login, pin, base, &userId);
            HandlingError(err);
            continue;
        }

        int sanc = base->users[userId].sanctions;

        while (in_account) {
            printf("Available commands:\nTime\nDate\nHowmuch hh:mm:ss "
                   "DD:MM:YYYY <flag>(possible: -s, -m, -h, -y)\nLogout\n"
                   "Sanctions username <number>(from 1 to 5)\nEnter command: ");
            char command[40];
            getchar();
            if (scanf("%[^\n]", command) != 1) {
                HandlingError(ERROR_READ);
                if ((err = SaveDataToFile(base, "data.txt"))) {
                    HandlingError(err);
                    FreeUsers(base);
                    return err;
                }
                FreeUsers(base);
                return ERROR_READ;
            }

            const int com = CommandValid(command);
            if (com < 0 ) {
                printf("Invalid command. Try again:\n");
                continue;
            }
            if (sanc > 0) {
                --sanc;
            } else if (sanc == 0) {
                --sanc;
                printf("You can no longer enter commands in the current session"
                       " due to the restrictions imposed.\n");
                break;
            }

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
                    HandlingError(Howmuch(command));
                    break;
                }
                case LOGOUT: {
                    in_account = 0;
                    break;
                }
                case SANCTIONS: {
                    HandlingError(Sanctions(command, login, base));
                    break;
                }
                default: {
                    printf("Unknown command.\n");
                    break;
                }
            }
        }

    }
}