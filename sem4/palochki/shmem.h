#pragma once
#include <stdexcept>
#include <sys/shm.h>

class Shmemory {
    size_t sz;
    int shmid;
    void *data = nullptr;
public:
    Shmemory(const int id_, const size_t sz_) : sz(sz_) {
        const key_t key_ = ftok(("tg_" + std::to_string(id_)).c_str(), id_);
        shmid = shmget(key_, sz, 666);
        if (shmid == -1) {
            throw std::runtime_error("Error creating shared memory segment");
        }
    }

    void connect() {
        data = shmat(shmid, nullptr, 0);
    }

    void disconnect() {
        if (data != nullptr) {
            shmdt(data);
            shmctl(shmid, IPC_RMID, nullptr);
            data = nullptr;
        }
    }

    ~Shmemory() {
        disconnect();
    }
};