
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>



#define EATING_TIME 2
#define THINKING_TIME 2
#define SIMULATION_DURATION 30

struct PhData{
    int id;
    int phAmount;
    int semId;
};

union semun{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};

int isInt(const char* str){
    int i = 0;
    while (str[i])
    {
        if(!isdigit(str[i])){
            if(i == 0 && str[i] == '-'){
                continue;
            }
            return 0;
        }
        ++i;
    }
    return 1;
}

void waitSemaphore(int semId, int semNum) {
    struct sembuf op = {semNum, -1, 0};
    semop(semId, &op, 1);
}

void signalSemaphore(int semId, int semNum) {
    struct sembuf op = {semNum, 1, 0};
    semop(semId, &op, 1);
}

void* problem(void* arg){
    struct PhData pdata = *(struct PhData*)arg;

    int id = pdata.id;
    int leftFork = id;
    int rightFork = (id + 1) % pdata.phAmount;
    int semId = pdata.semId;

    int firstFork = leftFork < rightFork ? leftFork : rightFork;
    int secondFork = leftFork < rightFork ? rightFork : leftFork;

    // int firstFork = leftFork;
    // int secondFork = rightFork;

    time_t start_time = time(NULL);

    while (time(NULL) - start_time < SIMULATION_DURATION) {
        printf("Philosopher %d is thinking...\n", id);
        sleep(rand() % 5);
        printf("Philosopher %d is hungry...\n", id);

        waitSemaphore(semId, firstFork);
        printf("Philosopher %d picked up first fork %d\n", id, firstFork);

        usleep(10000);

        waitSemaphore(semId, secondFork);
        printf("Philosopher %d picked up second fork %d\n", id, secondFork);

        printf("Philosopher %d is eating...\n", id);
        sleep(EATING_TIME);

        signalSemaphore(semId, secondFork);
        printf("Philosopher %d put down second fork %d\n", id, secondFork);

        signalSemaphore(semId, firstFork);
        printf("Philosopher %d put down first fork %d\n", id, firstFork);
    }

    printf("Philosopher %d has finished dining.\n", id);
    return NULL;

}

int main(int argc, char const *argv[])
{
    int phAmount = 5;
    if(argc == 1){
        printf("The number of philosophers is 5.\n");
    }
    else{
        if(isInt(argv[1])){
            phAmount = atoi(argv[1]);
            if(phAmount < 0){
                printf("The number of philosophers must be positive!\n");
                return -1;
            }
        }
        else{
            printf("The number of philosophers must be integer!\n");
            return -1;
        }
    }

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * phAmount);
    if(!threads){
        printf("Memory error!\n");
        return -1;
    }


    int semFd = semget(0, phAmount, IPC_CREAT | 0666);
    if(semFd == -1){
        printf("Sem error!\n");
        free(threads);
        return -1;
    }

    union semun arg;
    unsigned short* vals = (unsigned short*)malloc(sizeof(unsigned short) * phAmount);
    for (int i = 0; i < phAmount; i++) {
        vals[i] = 1;
    }
    arg.array = vals;
    if (semctl(semFd, 0, SETALL, arg) == -1) {
        perror("Sem init error:");
        free(threads);
        free(vals);
        semctl(semFd, 0, IPC_RMID);
        return 1;
    }
    free(vals);


    struct PhData* data = (struct PhData*)malloc(sizeof(struct PhData) * phAmount);
    if(!data){
        printf("Memory error!\n");
        free(threads);
        if (semctl(semFd, 0, IPC_RMID) == -1) {
            printf("Sem error!\n");
        }
        return -1;
    }



    for (size_t i = 0; i < (size_t)phAmount; i++)
    {
        data[i].id = i;
        data[i].phAmount = phAmount;
        data[i].semId = semFd;

        if(pthread_create(&threads[i], NULL, problem, &data[i]) == -1){
            printf("Pthread_create error!\n");
            free(threads);
            free(data);
            semctl(semFd, 0, IPC_RMID);
            return -1;
        }
    }

    for (int i = 0; i < phAmount; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(data);
    if (semctl(semFd, 0, IPC_RMID) == -1) {
        printf("Sem error!\n");
        return -1;
    }
    return 0;
}