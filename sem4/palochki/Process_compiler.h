#pragma once
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <sys/wait.h>

#include "semafor.h"
#include "shmem.h"

class ProcessСompiler {
    pid_t pid = 0;
    std::string name;
    bool is_working = true;
public:
    explicit ProcessСompiler(std::string name_) : name(std::move(name_)) {}
    void start() {
        pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to get pid");
        }
        if (pid == 0) {
            Shmemory chat(1234, 512, CONNECT);
            chat.connect();

            Semafor sem(2513, 2, CONNECT);

            while (is_working) {
                std::cout << "Process compile started" << std::endl;
                sem.wait_for(1, 0);
                sem.minus(0);
                std::string file_name = chat.read();
                std::cout << "file_name1: " << file_name << std::endl;
                std::string comp_file_name = file_name.substr(0, file_name.find('.'));

                int res = system(("g++ " + file_name + " -o " + comp_file_name).c_str());
                if (res != 0) {
                    chat.write("fail");
                } else {
                    chat.write(comp_file_name);
                }
                std::cout << "comp_file_name1: " << comp_file_name << std::endl;
                sem.plus(0);
                sem.plus(1);
            }
        }
    }
    void stop() {
        is_working = false;
    }

    bool wait() const {
        if (pid > 0) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                throw std::runtime_error("Failed to wait for child process");
            }
            return WIFEXITED(status);
        }
        return false;
    }

    pid_t getPid() const {
        return pid;
    }

    ~ProcessСompiler() = default;
};
