#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

enum {
    NF,
    LA,
    L,
    SUCCESS = 0,
    ERROR_READ_FILE_PATH,
    ERROR_OPENING_FILE,
    ERROR_GETTING_FILE_STAT,
    ERROR_OPENING_DIR,
    ERROR_UNKNOWN_FLAG,
};

void getFileType(const mode_t mode, char* accR) {
    switch (mode & S_IFMT) {
        case S_IFREG: {
            accR[0] = '-';
            break;
        }
        case S_IFDIR: {
            accR[0] = 'd';
            break;
        }
        case S_IFBLK: {
            accR[0] = 'b';
            break;
        }
        case S_IFCHR: {
            accR[0] = 'c';
            break;
        }
        case S_IFIFO: {
            accR[0] = 'p';
            break;
        }
        case S_IFSOCK: {
            accR[0] = 's';
            break;
        }
        case S_IFLNK: {
            accR[0] = 'l';
            break;
        }
        default: {
            perror("getFileType");
            break;
        }
    }
    accR[1] = (mode & S_IRUSR) ? 'r' : '-';
    accR[2] = (mode & S_IWUSR) ? 'w' : '-';
    accR[3] = (mode & S_IXUSR) ? 'x' : '-';
    accR[4] = (mode & S_IRGRP) ? 'r' : '-';
    accR[5] = (mode & S_IWGRP) ? 'w' : '-';
    accR[6] = (mode & S_IXGRP) ? 'x' : '-';
    accR[7] = (mode & S_IROTH) ? 'r' : '-';
    accR[8] = (mode & S_IWOTH) ? 'w' : '-';
    accR[9] = (mode & S_IXOTH) ? 'x' : '-';
}

int LsDir(DIR* dir, const char* dir_name, const int flag) {
    struct dirent *ent;
    struct stat file_stat;

    int ret = chdir(dir_name);
    if (ret != 0) {
        return ERROR_OPENING_DIR;
    }

    while ((ent = readdir(dir)) != NULL) {

        const int fd = open(ent->d_name, "r");
        if (fd < 0) {
            return ERROR_OPENING_FILE;
        }

        ret = fstat(fd, &file_stat);
        if (ret < 0) {
            close(fd);
            return ERROR_GETTING_FILE_STAT;
        }

        printf("%s:\n", ent->d_name);

        if (flag == NF) {
            printf("%lu   \033[32m%s", file_stat.st_ino, ent->d_name);
            break;
        }

        if (flag == LA || flag == L) {
            if (flag == L && ent->d_name[0] == '.') {
                continue;
            }
            char accessR[10];
            getFileType(file_stat.st_mode, accessR);

            char buf[20];
            const struct tm *tm = localtime(&file_stat.st_mtime);
            strftime(buf, sizeof(buf), "%x", tm);

            // if (ent->d_name[0] == '.') {
            //     printf("%s   %2ld   %2hd   %2hd   %5ld   %s   \033[41;37m%s", accessR, file_stat.st_nlink,
            //     file_stat.st_uid, file_stat.st_gid, file_stat.st_size, buf, ent->d_name);
            // } else {
            printf("%s   %2ld   %2hd   %2hd   %5ld   %s   \033[32m%s\n", accessR, file_stat.st_nlink,
                file_stat.st_uid, file_stat.st_gid, file_stat.st_size, buf, ent->d_name);
            // }
        }
        close(fd);
    }
    printf("\n");
    return SUCCESS;
}

int ls(const int cnt, const char **str_dirs) {
    int err;
    int flag = NF;
    if (strcmp(str_dirs[cnt - 1], "-l") == 0) {
        flag = L;
    }
    if (strcmp(str_dirs[cnt - 1], "-la") == 0) {
        flag = L;
    }

    if (cnt == 1 || (cnt == 2 && flag != NF)) {
        printf("HUY\n");
        char progpath[1024];
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
        if (len == -1) {
            return ERROR_READ_FILE_PATH;
        }
        while (progpath[len] != '/')
            --len;
        progpath[len] = '\0';

        DIR *dir = opendir(progpath);
        if (dir != NULL) {
            if((err = LsDir(dir, progpath, flag))) {
                closedir(dir);
                return err;
            }
            closedir(dir);
        } else {
            printf("Could not open current directory\n");
        }
    }
    else {
        printf("WWW\n");
        for (int i = 1; ((flag == NF)? (i < cnt) : (i < cnt - 1)); i++) {
            DIR *dir = opendir(str_dirs[i]);
            if (dir != NULL) {
                if ((err = LsDir(dir, str_dirs[i], flag))) {
                    closedir(dir);
                    return err;
                }
                closedir(dir);
            } else {
                printf("Could not open directory '%s'\n", str_dirs[i]);
            }
        }
    }
    return SUCCESS;
}

int main(const int argc, char const *argv[]) {
    return ls(argc, argv);
}