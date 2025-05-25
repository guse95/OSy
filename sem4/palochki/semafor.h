#pragma once
#include <ostream>
#include <sys/ipc.h>
#include <sys/sem.h>

enum {
    CREATE,
    CONNECT
};

class Semafor {
    int num;
    int semid;
public:
    Semafor(const int id_, const int num_, int op) : num(num_) {
        const key_t key_ = ftok(("sem_" + std::to_string(id_)).c_str(), id_);
        if (op == CREATE) {
            semid = semget(key_, num, IPC_CREAT | IPC_EXCL | 0666);
        } else {
            semid = semget(key_, num, 0);
        }
        if (semid == -1) {
            throw std::runtime_error("Error creating semaphore");
        }
    }


    void assign(const std::initializer_list<unsigned short> init) const {
        if (init.size() != num) {
            throw std::runtime_error("Incorrect size");
        }
        semctl(semid, 0, SETALL, init);
    }

    void assign(const int poz, const unsigned short val) const {
        if (poz >= num) {
            throw std::runtime_error("Incorrect size");
        }
        semctl(semid, poz, SETVAL, val);
    }

    void sem_wait(const unsigned short int ind) const {
        if (ind >= num) {
            throw std::runtime_error("Incorrect size");
        }
        sembuf opp = {ind, -1, 0};
        semop(semid, &opp, 1);
    }

    void sem_post(const unsigned short int ind) const {
        if (ind >= num) {
            throw std::runtime_error("Incorrect size");
        }
        sembuf opp = {ind, 1, 0};
        semop(semid, &opp, 1);
    }

    ~Semafor() {
        semctl(semid, 0, IPC_RMID);
    }
};
