#include <iostream>
#include <string>
#include <exception>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <map>
#include <cstring>  // Added for memset
#include <filesystem>
#include "semafor.h"
#include "shmem.h"

#define BUFSIZE 4096

class Server {
    std::string ip;
    int port, sk, maximumListen;
    sockaddr_in serverAddr{};
    std::map<std::string, int> clients;

public:
    explicit Server(const std::string& ip_ = "0.0.0.0", const int port_ = 5000, const int maximumListen_ = 10) :
        ip(ip_), port(port_), maximumListen(maximumListen_) {
        sk = socket(AF_INET, SOCK_STREAM, 0);
        if (sk == -1) {
            throw std::runtime_error("Failed to create socket");
        }

        memset(&serverAddr, 0, sizeof(serverAddr)); // Initialize struct
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (ip != "0.0.0.0") {
            if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
                throw std::runtime_error("Invalid IP address");
            }
        } else {
            serverAddr.sin_addr.s_addr = INADDR_ANY;
        }
    }

    [[nodiscard]] bool userExists(const std::string& name) const {
        return (clients.find(name) == clients.end() && !clients.empty());
    }

    bool userAdd(const std::string& name, const int cl) {
        if (userExists(name)) {
            return false;
        }
        clients[name] = cl;
        return true;
    }

    bool userRemove(const std::string& name) {
        if (userExists(name)) {
            clients.erase(name);
            return true;
        }
        return false;
    }

    int userGetSocket(const std::string &name) {
        if (userExists(name)) {
            return clients[name];
        }
        return -1;
    }

    void bind() {
        if (::bind(sk, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(sockaddr_in)) < 0) {
            throw std::runtime_error("Bind failed");
        }
    }

    void listen() const {
        if (::listen(sk, maximumListen) < 0) {
            throw std::runtime_error("Listen failed");
        }
    }

    [[nodiscard]] std::pair<int, sockaddr_in> accept() const {
        sockaddr_in clientAddr{};
        socklen_t addrSize = sizeof(clientAddr);
        int clientDescr;
        if ((clientDescr = ::accept(sk, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrSize)) == -1) {
            throw std::runtime_error("Accept failed");
        }
        return std::make_pair(clientDescr, clientAddr);
    }

    static std::vector<char> receiveBytes(const int clientDescr) {
        size_t len = 0;
        if (recv(clientDescr, &len, sizeof(len), 0) == -1) {
            throw std::runtime_error("Failed to receive size");
        }

        std::vector<char> buf(len);

        ssize_t received = recv(clientDescr, buf.data(), len, 0);
        if (received == -1) {
            throw std::runtime_error("Failed to receive data");
        }
        return buf;
    }

    static std::string receiveString(const int clientDescr) {
        std::vector<char> ret = receiveBytes(clientDescr);
        return {ret.begin(), ret.end()};
    }

    static void receiveFile(const int clientDescr, FILE* file, const std::string& sz) {
        const size_t file_sz = stoi(sz);
        for (size_t i = 0; i < file_sz; i++) {
            std::string resp = receiveString(clientDescr);
            fwrite(resp.c_str(), sizeof(char), resp.size(), file);
        }
    }

    static ssize_t sendBytes(const int clientSk, const void* buff, const size_t size) {
        ssize_t sendedBytes = send(clientSk, &size, sizeof(size), 0);
        if (sendedBytes == -1) {
            throw std::runtime_error("Failed to send size");
        }

        if ((sendedBytes = send(clientSk, buff, size, 0)) == -1) {
            throw std::runtime_error("Failed to send data");
        }
        return sendedBytes;
    }

    static ssize_t sendString(const int clientSk, const std::string& str) {
        return sendBytes(clientSk, str.c_str(), str.length());
    }

    static void sendFile(const int clientSk, FILE* file) {
        char buf[BUFSIZE];
        size_t bytes;
        while ((bytes = fread(buf, sizeof(char), BUFSIZE, file)) > 0) {
            if (sendBytes(clientSk, buf, bytes) == -1) {
                throw std::runtime_error("Send failed");
            }
        }
    }


    void stop() {
        if (sk != -1) {
            close(sk);
            sk = -1;
        } else {
            throw std::runtime_error("Socket already closed");
        }
    }

    virtual ~Server() {
        if (sk != -1) {
            close(sk);
        }
    }
};

void processClient(Server& server, const std::pair<int, sockaddr_in> &pr) {
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(pr.second.sin_addr), clientIP, INET_ADDRSTRLEN);
    std::cout << "Client " << clientIP << " connected to the server" << std::endl;

    std::string senderName;
    try {
        // Registration loop
        while (true) {
            senderName = server.receiveString(pr.first);
            if (senderName.empty() || senderName == "exit") {
                close(pr.first);
                return;
            }

            if (server.userExists(senderName)) {
                Server::sendString(pr.first, "NO");
            } else {
                server.userAdd(senderName, pr.first);
                Server::sendString(pr.first, "OK");
                break;
            }
        }

        // Messaging loop
        while (true) {
            std::string command = Server::receiveString(pr.first);
            std::string response;

            if (command == "compile") {
                std::cout << "compilation started" << std::endl;
                Server::sendString(pr.first, "OK"); //TODO: ответ сервера че надо написать
                std::string size_text = Server::receiveString(pr.first); // размер файла в блоках

                std::string file_name = "tmp_file_" + std::to_string(pr.first) + ".cpp";
                std::string comp_file_name = "tmp_file_" + std::to_string(pr.first);

                FILE* file = fopen(file_name.c_str(), "wb");
                if (file == nullptr) {
                    throw std::runtime_error("Failed to open file for writing");
                }

                Server::receiveFile(pr.first, file, size_text);
                fclose(file);

                int res = system(("g++ " + file_name + " -o " + comp_file_name).c_str());
                if (res != 0) {
                    Server::sendString(pr.first, "NO");
                    continue;
                }
                Server::sendString(pr.first, "OK");

                auto file_sz = std::filesystem::file_size(comp_file_name);
                file_sz = file_sz / BUFSIZE + ((file_sz % BUFSIZE) ? 1 : 0);
                Server::sendString(pr.first, std::to_string(file_sz));

                file = fopen(comp_file_name.c_str(), "rb");
                if (file == nullptr) {
                    throw std::runtime_error("Failed to open file for writing");
                }
                Server::sendFile(pr.first, file);
                fclose(file);

            } else if (command == "game") {
                Server::sendString(pr.first, "OK");
                std::cout << "game started" << std::endl;

            } else if (command == "disconnect") {
                std::cout << "disconnecting..." << std::endl;
                Server::sendString(pr.first, "OK");
                break;
                //TODO: Проверить завершение сессии
            } else {
                Server::sendString(pr.first, "NO");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error with client " << senderName << ": " << e.what() << std::endl;
    }

    if (!senderName.empty()) {
        server.userRemove(senderName);
    }
    close(pr.first);
    std::cout << "Client " << senderName << " disconnected" << std::endl;
}

int main() {
    // Shmemory chat(1234, 512);
    // chat.connect();
    //
    // Semafor sem(2513, 2, CREATE);


    // const pid_t pid_game = fork();
    // if (pid_game == 0) {
    //     execl("./child_process", "./child_process", std::to_string(key_), nullptr);
    //     std::cerr << "ERROR: starting game server" << std::endl;
    //     return 0;
    // }
    // if (pid_game < 0) {
    //     std::cerr << "ERROR: fork" << std::endl;
    //     return 0;
    // }


    try {
        Server server{};
        server.bind();
        server.listen();

        std::vector<std::thread> threads;

        while (true) {
            try {
                auto clt = server.accept();
                threads.emplace_back(
                    processClient,
                    std::ref(server),
                    clt
                    );

                for (auto it = threads.begin(); it != threads.end(); ) {
                    if (it->joinable()) {
                        it->join();
                        it = threads.erase(it);
                    } else {
                        ++it;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Accept error: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 0;
    }
} // /mnt/c/Users/Кря/CLionProjects/OSy/sem4/palochki