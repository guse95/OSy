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

    ip_stat& operator+= (const ip_stat& other) {
        data_rcv += other.data_rcv;
        data_snd += other.data_snd;
        con_cnt += other.con_cnt;
        con_ip.insert(con_ip.end(), other.con_ip.begin(), other.con_ip.end());
        return *this;
    }
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

void generator(std::mutex& life, std::mutex& commands, bool& is_working, int msg_id, std::condition_variable& cv) {
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
        if (log[0] == 'S' || log[0] == 'R') {
            log += " " + std::to_string(size_dist(gen)) + " Kb\n";
        }

        logger->info(log);
        life.unlock();
        sleep(1);
        // TODO: че-то неправильно паузится
        std::unique_lock<std::mutex> lock(commands);
        cv.wait(lock, [is_working] {return is_working;});
    }
}

void analyzer(std::mutex& life, std::mutex& commands, bool& is_working, const int msg_id, std::condition_variable& cv, std::map<std::string, ip_stat>& data) {
    msgb msg{};

    while (life.try_lock()) {
        if (msgrcv(msg_id, &msg, sizeof(msg.mtext), 0, 0) != -1) {
            char operation = msg.mtext[0];
            char ip_from[16], ip_to[16], sz[4];

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
            while (msg.mtext[i++] != ' ' || msg.mtext[i] != '\0') {}
            int ind_sz = 0;
            while (msg.mtext[i] != ' ' || msg.mtext[i] != '\0') {
                sz[ind_sz] = msg.mtext[i];
                ++ind_sz;
                ++i;
            }
            sz[ind_sz] = '\0';
            int pck_sz = (ind_sz == 0) ? 0 : atoi(sz);

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
        // TODO: че-то неправильно паузится
        std::unique_lock<std::mutex> lock(commands);
        cv.wait(lock, [is_working] {return is_working;});
    }
}

int main() {
    std::mutex com;
    std::mutex life;
    std::condition_variable cv;

    remove("nasrano");
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

    bool is_working = true;
    int num_of_threads = 4;

    const pid_t generator_pid = fork();
    {
        if (generator_pid == 0) {

            std::cout << "Generator started (PID: " << getpid() << ")\n";
            while (true) {
                msgb msg;
                if (ssize_t ret = msgrcv(msg_id_gen, &msg, sizeof(msg.mtext), 0, 0); ret == -1) {
                    perror("msgrcv in generator");
                    break;
                }
                switch (msg.mtype) {
                    case begin: {
                        std::vector<std::thread> threads(num_of_threads);

                        for (int i = 0; i < num_of_threads; ++i) {
                            threads[i] = std::thread(generator,
                                std::ref(life),
                                std::ref(com),
                                std::ref(is_working),
                                msg_id_public,
                                std::ref(cv));
                        }
                        for (auto &th : threads) {
                            th.join();
                        }
                        break;
                    }
                    case quit: {
                        std::cout << "Generator exiting...\n";
                        life.lock();
                        return 0;
                    }
                    case stop: {
                        is_working = false;
                        break;
                    }
                    case cont: {
                        is_working = true;
                        cv.notify_all();
                        break;
                    }
                    default: {
                        std::cout << "something went wrong" << msg.mtype << "\n";
                        break;
                    }
                }
            }
        }
    }

    const pid_t analyzer_pid = fork();
    {
        if (analyzer_pid == 0) {
            std::cout << "Analyzer started (PID: " << getpid() << ")\n";
            while (true) {
                msgb msg;
                if (ssize_t ret = msgrcv(msg_id_an, &msg, sizeof(msg.mtext), 0, 0); ret == -1) {
                    perror("msgrcv in analyzer");
                    break;
                }
                std::vector<std::map<std::string, ip_stat>> stata(num_of_threads);
                switch (msg.mtype) {
                    case begin: {
                        std::vector<std::thread> threads(num_of_threads);

                        for (int i = 0; i < num_of_threads; ++i) {
                            threads[i] = std::thread(analyzer,
                                std::ref(life),
                                std::ref(com),
                                std::ref(is_working),
                                msg_id_public,
                                std::ref(cv),
                                std::ref(stata[i]));
                        }
                        for (auto &th : threads) {
                            th.join();
                        }
                        break;
                    }
                    case quit: {
                        std::cout << "Analyzer exiting...\n";
                        life.lock();
                        return 0;
                    }
                    case stop: {
                        is_working = false;
                        break;
                    }
                    case cont: {
                        is_working = true;
                        cv.notify_all();
                    }
                    case stat: {
                        ip_stat res{};
                        for (auto el : stata) {
                            res += el[msg.mtext];
                        }

                        std::cout << "{ data_rcv: " << res.data_rcv
                        << "\n\tdata_snd: " << res.data_snd
                        << "\n\tcon_cnt: " << res.con_cnt
                        << "\n\tcon_ip: [ ";
                        for (auto el : res.con_ip) {
                            std::cout << el << " ";
                        }
                        std::cout << "]\n}\n";
                    }
                    default: {
                        std::cout << "something went wrong" << msg.mtype << "\n";
                        break;
                    }
                }
            }
            return 0;
        }
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
            message.mtype = stat;
            strncpy(message.mtext, command.c_str() + 5 , sizeof(message.mtext)-1);
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
