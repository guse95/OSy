#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <ctype.h>


typedef enum{
    EMPTY,
    WOMEN,
    MEN
}CurState;

struct Bathroom{
    CurState state;
    int maxPeopleNumber;
    int curPeopleNumber;
};

static struct Bathroom bRoom;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


int isInt(const char* str){
    size_t i = str[0] == '-'? 1 : 0;
    while(str[i] != '\0'){
        if(!isdigit(str[i])){
            return 0;
        }
        ++i;
    }
    return 1;
}



int woman_wants_to_enter(){
    pthread_mutex_lock(&mutex);

    int canEnter = 0;
    if((bRoom.curPeopleNumber < bRoom.maxPeopleNumber && bRoom.state == WOMEN) || bRoom.state == EMPTY) {
        canEnter = 1;
    }

    pthread_mutex_unlock(&mutex);

    return canEnter;
}
int man_wants_to_enter(){
    pthread_mutex_lock(&mutex);

    int canEnter = 0;
    if((bRoom.curPeopleNumber < bRoom.maxPeopleNumber && bRoom.state == MEN) || bRoom.state == EMPTY) {
        canEnter = 1;
    }

    pthread_mutex_unlock(&mutex);

    return canEnter;
}
void woman_leaves(){
    pthread_mutex_lock(&mutex);

    bRoom.curPeopleNumber--;
    if(bRoom.curPeopleNumber == 0){
        bRoom.state = EMPTY;
    }

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}
void man_leaves(){
    pthread_mutex_lock(&mutex);

    bRoom.curPeopleNumber--;
    if(bRoom.curPeopleNumber == 0){
        bRoom.state = EMPTY;
    }

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

void* woman_thread(void* arg){
    int id = *((int*)arg);

    while(1){
        pthread_mutex_lock(&mutex);
        while (bRoom.state != EMPTY && !(bRoom.state == WOMEN || bRoom.curPeopleNumber < bRoom.maxPeopleNumber))
        {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        
        int canEnter = woman_wants_to_enter();
        int rndTime;
        if(canEnter){
            bRoom.state = WOMEN;
            bRoom.curPeopleNumber++;
            printf("Woman %d entered the room and began washing.\n", id);
            rndTime = rand() % 5;
            sleep(rndTime);
            woman_leaves();
            printf("Woman %d ended washing and escaped the room.\n", id);
        }
        rndTime = 2 + rand() % 6;
        sleep(rndTime);
    }
}
void* man_thread(void* arg){
    int id = *((int*)arg);


    while(1){
        pthread_mutex_lock(&mutex);
        while (bRoom.state != EMPTY && !(bRoom.state == MEN || bRoom.curPeopleNumber < bRoom.maxPeopleNumber))
        {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        
        int canEnter = man_wants_to_enter();
        int rndTime;
        if(canEnter){
            bRoom.state = MEN;
            bRoom.curPeopleNumber++;
            printf("Man %d entered the room and began washing.\n", id);
            rndTime = rand() % 3;
            sleep(rndTime);
            man_leaves();
            printf("Man %d ended washing and escaped the room.\n", id);
        }
        rndTime = 2 + rand() % 6;
        sleep(rndTime);
    }
}


int main(int argc, char const *argv[])
{
    srand(time(NULL)); 
    if(argc != 3){
        printf("There must be 3 arguments ./<command> <places in bathroom> <amount of threads>\n");
        return -1;
    }
    if(!(isInt(argv[1]) && isInt(argv[2]))){
        printf("Arguments must be integers!\n");
        return -1;
    }

    
    bRoom.state = EMPTY;
    bRoom.maxPeopleNumber = atoi(argv[1]);
    bRoom.curPeopleNumber = 0;
    int thAmount = atoi(argv[2]);
    if(bRoom.maxPeopleNumber <= 0 || thAmount <= 0){
        printf("Arguments must be positive!\n");
        return -1;
    }

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * thAmount);
    if(!threads){
        printf("Memory error!\n");
        return -1;
    }

    int *IDs = (int*)malloc(thAmount * sizeof(int));
    if(!IDs){
        free(threads);
        printf("Memory error!\n");
        return -1;
    }

    for (size_t i = 0, w = 0, m = 0; i < (size_t)thAmount; i++)
    {
        if(i % 2 == 0){
            IDs[i] = m++;
        }else{
            IDs[i] = w++;
        }
    }
    

    for (size_t i = 0; i < (size_t)thAmount; i++)
    {
        if(i % 2 == 0){
            if( pthread_create(&threads[i], NULL, man_thread, &IDs[i]) == -1){
                printf("Pthread error!\n");
                for (size_t j = 0; j < i; j++)
                {
                    pthread_cancel(threads[j]);
                }
                free(threads);
            }
        } else {
            if( pthread_create(&threads[i], NULL, woman_thread, &IDs[i]) == -1){
                printf("Pthread error!\n");
                for (size_t j = 0; j < i; j++)
                {
                    pthread_cancel(threads[j]);
                }
                free(threads);
            }
        }
    }
    

    for (size_t i = 0; i < (size_t)thAmount; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}