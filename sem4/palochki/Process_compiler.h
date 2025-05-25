#pragma once
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include "semafor.h"
#include "shmem.h"

class Process_compiler {
    pid_t pid = 0;
    std::string name;
public:
    explicit Process_compiler(std::string name_) : name(std::move(name_)) {
        pid = getpid();
        if (pid < 0) {
            throw std::runtime_error("Failed to get pid");
        }
        if (pid == 0) {
            Shmemory chat(1234, 512);
            chat.connect();

            Semafor sem(2513, 2, CONNECT);
            
        }
    }
    //TODO: остановка работы

    ~Process_compiler() = default;
};
