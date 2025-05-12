#include <iostream>
#include <queue>
#include <string>
#include <list>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <random>
#include <sys/msg.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <cstring>
#include "Logger.h"


struct msgb {
    long mtype;
    char mtext[256];
};

enum {
    quit = 1,
    begin = 2,
    stop = 3,
    cont = 4,
    stat = 5,
};

struct ip_stat {
    long long data_rcv = 0;
    long long data_snd = 0;
    long long con_cnt = 0;
    std::list<std::string> con_ip;
};

class MsgQueueBuf final : public std::streambuf {
private:
    int msgid;
    std::string current_line;

protected:
    int overflow(int ch) override {
        if (ch == traits_type::eof()) return ch;

        if (ch == '\n') {
            send_line();
        } else {
            current_line += static_cast<char>(ch);
        }
        return ch;
    }

    int sync() override {
        send_line();
        return 0;
    }

    void send_line() {
        if (!current_line.empty()) {
            msgb msg{};
            msg.mtype = 1;
            strncpy(msg.mtext, current_line.c_str(), sizeof(msg.mtext) - 1);
            msgsnd(msgid, &msg, strlen(msg.mtext) + 1, 0);
            current_line.clear();
        }
    }

public:
    explicit MsgQueueBuf(int msgid) : msgid(msgid) {}
};

class MsgQueueOStream final : public std::ostream {
    MsgQueueBuf buffer;

public:
    explicit MsgQueueOStream(int msgid) : std::ostream(&buffer), buffer(msgid) {}
};

void generator(std::mutex& life, std::mutex& commands, int msg_id, std::condition_variable& cv) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> port_dist(1, 65535);
    std::uniform_int_distribution<> size_dist(1, 1024);
    std::uniform_int_distribution<> op_ind(0, 4);

    auto msg_stream = std::make_unique<MsgQueueOStream>(msg_id);

    Logger *logger = LoggerBuilder().set_level(INFO)
            .add_handler(std::move(msg_stream))
            .add_handler(std::make_unique<std::ofstream>("test.txt"))
            .make_object();

    const std::string operations[] = {"Connect", "Disconnect", "Receive", "Send"};
    while (life.try_lock()) {
        std::string log = operations[op_ind(gen)];
        log += " from sourse: 192.168.1.0:";
        log += std::to_string(port_dist(gen));
        log += " to destination: 10.0.0.0:";
        log += std::to_string(port_dist(gen));
        if (log.substr(0, 4) == "Send") {
            log += " " + std::to_string(size_dist(gen)) + " Kb\n";
        }

        logger->info(log);
        life.unlock();
        sleep(1);
        std::unique_lock<std::mutex> lock(commands);
        cv.wait(lock);
        cv.notify_all();
    }
}

void analizer(std::mutex& life, std::mutex& commands, int msg_id,std::condition_variable& cv, std::map<std::string, ip_stat>& data) {
    msgb msg{};
    std::string operations[] = {"Connect", "Disconnect", "Receive", "Send"};

    while (life.try_lock()) {
        if (msgrcv(msg_id, &msg, sizeof(msg.mtext), 0, 0) != -1) {
            char operation = msg.mtext[0];
            char ip_from[16], ip_to[16], sz[4];
            int pck_sz;

            int i = 16;
            while (msg.mtext[i++] != ':') {}
            ++i;
            int ind_from = 0;
            while (msg.mtext[i] != ':') {
                ip_from[ind_from] = msg.mtext[i];
                ++ind_from;
                ++i;
            }
            ip_from[ind_from] = '\0';
            ++i;
            while (msg.mtext[i++] != ':') {}
            ++i;
            int ind_to = 0;
            while (msg.mtext[i] != ':') {
                ip_to[ind_to] = msg.mtext[i];
                ++ind_to;
                ++i;
            }
            sz[ind_to] = '\0';
            while (msg.mtext[i++] != ' ') {}
            int ind_sz = 0;
            while (msg.mtext[i] != ' ' || msg.mtext[i] != '\0') {
                sz[ind_sz] = msg.mtext[i];
                ++ind_sz;
                ++i;
            }
            sz[ind_sz] = '\0';
            pck_sz = (ind_sz == 0) ? 0 : atoi(sz);

            std::string from_ip(ip_from);
            std::string to_ip(ip_to);

            data.insert(std::pair<std::string, ip_stat>(ip_from, ip_stat()));
            data.insert(std::pair<std::string, ip_stat>(ip_to, ip_stat()));

            if (operation == 'C' || operation == 'D') {
                bool is_connected = false;
                for (auto & connected_ip : data[ip_from].con_ip) {
                    if (connected_ip == ip_to) {
                        is_connected = true;
                    }
                }
                if (operation == 'C') {
                    if (!is_connected) {
                        data[ip_from].con_cnt += 1;
                        data[ip_to].con_cnt += 1;
                        data[ip_from].con_ip.emplace_back(ip_to);
                        data[ip_to].con_ip.emplace_back(ip_from);
                    }

                } else {
                    if (is_connected) {
                        data[ip_to].con_cnt -= 1;
                        data[ip_from].con_cnt -= 1;
                        data[ip_from].con_ip.remove(ip_to);
                        data[ip_to].con_ip.remove(ip_from);
                    }
                }
            }

            if (operation == 'S') {
                data[ip_from].data_snd += pck_sz;
                data[ip_to].data_rcv += pck_sz;

            } else if (operation == 'R') {
                data[ip_from].data_rcv += pck_sz;
                data[ip_to].data_snd += pck_sz;
            }
        }
        life.unlock();
        sleep(1);
        std::unique_lock<std::mutex> lock(commands);
        cv.wait(lock);
        cv.notify_all();
    }
}

int main() {
    std::mutex com;
    std::mutex life;
    std::condition_variable cv;


    std::ofstream("nasrano").close();

    const key_t key_public = ftok("nasrano", 'C');
    int msg_id_public = msgget(key_public, 0666 | IPC_CREAT);
    if (msg_id_public == -1) {
        perror("msgget_public");
        exit(EXIT_FAILURE);
    }

    remove("tg_gen");
    remove("tg_an");


    std::ofstream("tg_gen").close();
    std::ofstream("tg_an").close();


    const key_t key_gen = ftok("tg_gen", 'A');
    int msg_id_gen = msgget(key_gen, 0666 | IPC_CREAT);
    if (msg_id_gen == -1) {
        perror("msgget_gen");
        exit(EXIT_FAILURE);
    }

    const key_t key_an = ftok("tg_an", 'B');
    int msg_id_an = msgget(key_an, 0666 | IPC_CREAT);
    if (msg_id_an == -1) {
        perror("msgget_an");
        exit(EXIT_FAILURE);
    }

    const pid_t generator_pid = fork();
    if (generator_pid == 0) {

        std::cout << "Generator started (PID: " << getpid() << ")\n";
        while (true) {
            msgb msg;
            ssize_t ret = msgrcv(msg_id_gen, &msg, sizeof(msg.mtext), 0, 0);
            if (ret == -1) {
                perror("msgrcv in generator");
                break;
            }
            switch (msg.mtype) {
                case begin: {
                    std::vector<std::thread> threads(4);

                    for (int i = 0; i < 4; ++i) {
                        threads[i] = std::thread(generator, life, com, msg_id_public, cv);
                    }
                    for (auto &th : threads) {
                        th.join();
                    }
                    break;
                }
                case quit: {
                    std::cout << "Generator exiting...\n";
                    life.lock();
                }
            }

            std::cout << "Generator received message:\n";
            std::cout << "Type: " << msg.mtype << "\n";
            std::cout << "Text: " << msg.mtext << "\n\n";

            if (msg.mtype == quit) {
                std::cout << "Generator exiting...\n";
                break;
            }
        }
        exit(0);
    }

    const pid_t analyzer_pid = fork();
    if (analyzer_pid == 0) {
        std::cout << "Analyzer started (PID: " << getpid() << ")\n";
        while (true) {
            msgb msg;
            ssize_t ret = msgrcv(msg_id_an, &msg, sizeof(msg.mtext), 0, 0);
            if (ret == -1) {
                perror("msgrcv in analyzer");
                break;
            }

            std::cout << "Analyzer received message:\n";
            std::cout << "Type: " << msg.mtype << "\n";
            std::cout << "Text: " << msg.mtext << "\n\n";

            if (msg.mtype == quit) {
                std::cout << "Analyzer exiting...\n";
                break;
            }
        }
        exit(0);
    }

    sleep(1);

    std::cout << "Manager started (PID: " << getpid() << ")\n";
    std::cout << "Available commands:\n"
              << "begin - start processing\n"
              << "stat <ip> - get statistics for IP\n"
              << "stop - pause processing\n"
              << "continue - resume processing\n"
              << "quit - exit program\n";

    msgb message;
    std::string command;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, command)) {
            break;
        }

        memset(&message, 0, sizeof(message));

        if (command == "quit") {
            message.mtype = quit;
            strcpy(message.mtext, "quit command");

            if (msgsnd(msg_id_gen, &message, strlen(message.mtext)+1, IPC_NOWAIT) == -1) {
                perror("msgsnd to generator");
            }
            if (msgsnd(msg_id_an, &message, strlen(message.mtext)+1, IPC_NOWAIT) == -1) {
                perror("msgsnd to analyzer");
            }
            sleep(1);
            break;
        }
        else if (command == "begin") {
            message.mtype = begin;
            strcpy(message.mtext, "begin processing");
            msgsnd(msg_id_gen, &message, strlen(message.mtext)+1, 0);
            msgsnd(msg_id_an, &message, strlen(message.mtext)+1, 0);
        }
        else if (command.substr(0, 4) == "stat") {
            if (command.length() <= 5) {
                std::cout << "Please specify IP address after 'stat'\n";
                continue;
            }
            std::string ip_str = command.substr(5);
            message.mtype = stat;
            strncpy(message.mtext, ip_str.c_str(), sizeof(message.mtext)-1);
            message.mtext[sizeof(message.mtext)-1] = '\0';
            msgsnd(msg_id_an, &message, strlen(message.mtext)+1, 0);
        }
        else if (command == "stop") {
            message.mtype = stop;
            strcpy(message.mtext, "stop processing");
            msgsnd(msg_id_gen, &message, strlen(message.mtext)+1, 0);
        }
        else if (command == "continue") {
            message.mtype = cont;
            strcpy(message.mtext, "continue processing");
            msgsnd(msg_id_gen, &message, strlen(message.mtext)+1, 0);
        }
        else {
            std::cout << "Unknown command. Type 'quit' to exit.\n";
        }
    }

    int status;
    waitpid(generator_pid, &status, 0);
    waitpid(analyzer_pid, &status, 0);

    std::cout << "Cleaning up message queues...\n";
    msgctl(msg_id_gen, IPC_RMID, nullptr);
    msgctl(msg_id_an, IPC_RMID, nullptr);
    msgctl(msg_id_public, IPC_RMID, nullptr);

    remove("tg_gen");
    remove("tg_an");
    remove("nasrano");

    std::cout << "Manager: All child processes finished successfully.\n";
    return 0;
}
