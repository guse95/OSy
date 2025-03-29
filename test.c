#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

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
        return -1;
    }

    if (tn->tm_year - t.tm_year < 0 || tn->tm_mon - t.tm_mon < 0 || tn->tm_mday - t.tm_mday < 0 ||
        tn->tm_hour - t.tm_hour < 0 || tn->tm_min - t.tm_min < 0 || tn->tm_sec - t.tm_sec < 0) {
        return -1;
        }

    *timeptr = mktime(&t);

    const double ans = difftime(now, *timeptr);
    printf("Seconds passed: %.2f\n\n", ans);
    return 0;
}



int main() {
    DIR *dir = opendir("..");
    struct dirent *ent = readdir(dir);
    ent = readdir(dir);
    ent = readdir(dir);

    printf("||%s||\n", ent->d_name);
    printf("HUY\n");
    const int fd = open(ent->d_name, "r");
    printf("HUY\n");

    struct stat file_stat;
    int ret = fstat(fd, &file_stat);
    printf("HUY\n");
    printf("%lu\n%hu\n%lu\n", file_stat.st_dev, file_stat.st_mode, file_stat.st_rdev);

    closedir(dir);
    close(fd);
}
