#pragma once
#include <ostream>
#include <sys/ipc.h>
#include <sys/sem.h>

#define CREATE 1
#define CONNECT 2

class Semafor {
    int num;
    int semid;
    std::string name;
public:
    Semafor(const int id_, const int num_, const int op) : num(num_) {
        name = "sem_" + std::to_string(id_);
        const key_t key_ = ftok(name.c_str(), id_);
        if (op == CREATE) {
            semid = semget(key_, num, IPC_CREAT | IPC_EXCL | 0666);
        } else {
            semid = semget(key_, num, 0 | 0666);
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

    void wait_for(const int exp0, const int exp1, const int delay = 100) const {
        while(true) {
            int v0 = semctl(semid, 0, GETVAL);
            int v1 = semctl(semid, 1, GETVAL);
            if (v0 == -1 || v1 == -1) {
                throw std::runtime_error("semctl GETVAL failed");
            }

            if (v0 == exp0 && v1 == exp1) {
                return;
            }

            usleep(delay * 1000);
        }
    }

    void minus(const unsigned short int ind) const {
        if (ind >= num) {
            throw std::runtime_error("Incorrect size");
        }
        sembuf opp = {ind, -1, 0};
        semop(semid, &opp, 1);
    }

    void plus(const unsigned short int ind) const {
        if (ind >= num) {
            throw std::runtime_error("Incorrect size");
        }
        sembuf opp = {ind, 1, 0};
        semop(semid, &opp, 1);
    }

    void detach(const bool rm = false) const {
        if (rm && semid != -1) {
            if (semctl(semid, 0, IPC_RMID) == -1) {
                throw std::runtime_error("Failed to remove semaphore");
            }
            std::remove(name.c_str());
        }
    }

    ~Semafor() {
        detach();
    }
};
