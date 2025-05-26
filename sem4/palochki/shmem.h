#pragma once
#include <stdexcept>
#include <sys/shm.h>

#define CREATE 1
#define CONNECT 2

class Shmemory {
    size_t sz;
    int shmid;
    char *data = nullptr;
    std::string name;
public:
    Shmemory(const int id_, const size_t sz_, const int op) : sz(sz_) {
        name = "tg_" + std::to_string(id_);
        const key_t key_ = ftok(name.c_str(), id_);
        if (op == CREATE) {
            shmid = shmget(key_, sz, 0666 | IPC_CREAT);
        } else {
            shmid = shmget(key_, sz, 0 | 0666);
        }
        if (shmid == -1) {
            throw std::runtime_error("Error creating shared memory segment");
        }
    }

    void connect() {
        data = static_cast<char *>(shmat(shmid, nullptr, 0));
    }

    void write(const std::string& str) const {
        strcpy(data, str.c_str());
    }

    [[nodiscard]] std::string read() const {
        return data;
    }


    void detach(const bool rm = false) const {
        shmdt(data);
        if (rm && shmid != -1) {
            shmctl(shmid, IPC_RMID, nullptr);
            std::remove(name.c_str());
        }
    }

    ~Shmemory() {
        detach();
    }
};