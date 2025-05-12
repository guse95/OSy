
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/limits.h>

#define MESSAGE_LEN PATH_MAX
#define STSIZE(st) (sizeof(st.messageText) + sizeof(st.from))
#define TO_SERVER 1


struct message{
    long mtype;
    char messageText[MESSAGE_LEN];
    long from;
};

typedef enum{
    SUCCESS,
    MEMORY_ERROR,
    FILE_ERROR,
    DIRECTORY_ERROR,
    NULL_ERROR,
}returnType;

returnType absdir(const char* relativePath, char* newPath) {
    if(realpath(relativePath, newPath)){
        short len = (short)strlen(newPath);
        newPath[len++] = '/';
        newPath[len] = '\0';
        return SUCCESS;
    }else{
        return DIRECTORY_ERROR;
    }
}


void* output(void* arg){
    int msgId = *(int*)arg;
    struct message msg;
    pid_t pid = getpid();

    while (1)
    {
        if(msgrcv(msgId, &msg, STSIZE(msg), pid, 0) == -1){
            perror("msgrcv");
            return NULL;
        }
        printf("%s\n", msg.messageText);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    key_t key = ftok("/home/gaalex/Programs/SysProg/SysProg/Labs/Lab1/Task6/Server/output/main", 'A');
    if(key == -1){
        perror("ftok");
        return -1;
    }

    if(argc == 1){
        printf("There must be at least one argument! ./<command> <directory1> ... <directoryn>\n");
        return -1;
    }


    int msgId = msgget(key, 0700);
    pid_t pid = getpid();
    if(msgId == -1){
        perror("msgget");
        return -1;
    }

    pthread_t thread;
    if(pthread_create(&thread, NULL, output, &msgId) == -1){
        perror("pcreate");
        return -1;
    }

    for (size_t i = 1; i < (size_t)argc; i++)
    {
        char dirName[PATH_MAX];
        int ret = absdir(argv[i], dirName);
        (void)ret;

        if(ret != SUCCESS){
            printf("Path error!\n");
            continue;
        }

        struct message msg;

        msg.from = (int)pid;
        msg.mtype = TO_SERVER;
        strcpy(msg.messageText, dirName);

        if(msgsnd(msgId, &msg, STSIZE(msg), 0) == -1){
            perror("msgsnd");
            continue;
        }
    }


    char c = getchar();
    (void)c;
    pthread_cancel(thread);
    return 0;
}
