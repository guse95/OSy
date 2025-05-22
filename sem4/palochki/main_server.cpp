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
#include <stdexcept> // For better exception handling

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
        return clients.find(name) != clients.end();
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

    std::string receiveString(const int clientDescr) {
        std::vector<char> ret = receivedBytes(clientDescr);
        return {ret.begin(), ret.end()};
    }

    ssize_t sendBytes(const int clientSk, const void* buff, const size_t size) {
        ssize_t sendedBytes = send(clientSk, &size, sizeof(size), 0);
        if (sendedBytes == -1) {
            throw std::runtime_error("Failed to send size");
        }

        if ((sendedBytes = send(clientSk, buff, size, 0)) == -1) {
            throw std::runtime_error("Failed to send data");
        }
        return sendedBytes;
    }

    std::vector<char> receivedBytes(const int clientDescr) {
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

    ssize_t sendString(const int clientSk, const std::string& str) {
        return sendBytes(clientSk, str.c_str(), str.length());
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

void processClient(Server& server, std::pair<int, sockaddr_in> pr) {
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
                server.sendString(pr.first, "NO");
            } else {
                server.userAdd(senderName, pr.first);
                server.sendString(pr.first, "OK");
                break;
            }
        }

        // Messaging loop
        while (true) {
            std::string receiverName = server.receiveString(pr.first);
            if (receiverName.empty() || receiverName == "exit") {
                break;
            }

            if (!server.userExists(receiverName)) {
                server.sendString(pr.first, "NO");
                continue;
            }

            server.sendString(pr.first, "OK");
            int receiverSocket = server.userGetSocket(receiverName);
            std::string text = server.receiveString(pr.first);
            server.sendString(receiverSocket, senderName + " : " + text);
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
    try {
        Server server{};
        server.bind();
        server.listen();

        std::vector<std::thread> threads;

        while (true) {
            try {
                auto clt = server.accept();
                threads.emplace_back(processClient, std::ref(server), clt);

                // Clean up finished threads
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
        return 1;
    }
}