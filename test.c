#include <dirent.h>
#include <stdio.h>
#include <time.h>

int main() {
    // DIR *dir;
    // struct dirent *ent;
    //
    // dir = opendir(".");
    // if (dir != NULL) {
    //     while ((ent = readdir(dir)) != NULL) {
    //         printf("%s\n", ent->d_name);
    //     }
    //     closedir(dir);
    // } else {
    //     perror("Could not open directory");
    //     return 1;
    // }

    time_t t;
    time(&t);
    const struct tm *tm = localtime(&t);

    char buf[80];
    strftime(buf, sizeof(buf), "%d:%m:%Y", tm);
    printf("%s\n", buf);

    return 0;
}
