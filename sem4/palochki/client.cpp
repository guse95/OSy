#include <iostream>
#include <string>
#include <exception>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <filesystem>

#define BUFSIZ 4096

class Client {
    std::string ip;
    int port, socket_fd;
    sockaddr_in server_addr{};

public:
    explicit Client(const std::string& ip_ = "127.0.0.1", const int port = 5000) : ip(ip_), port(port), socket_fd(-1) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            throw std::runtime_error("Socket creation failed");
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address/Address not supported");
        }
    }

    void connectToServer() {
        if (connect(socket_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
            throw std::runtime_error("Connection failed");
        }
        std::cout << "Connected to server at " << ip << ":" << port << std::endl;
    }

    [[nodiscard]] std::string receiveString() const {
        size_t len = 0;
        ssize_t rcv_bytes = recv(socket_fd, &len, sizeof(len), 0);
        if (rcv_bytes <= 0) {
            throw std::runtime_error("Connection closed or error occurred");
        }

        std::vector<char> buf(len);
        rcv_bytes = recv(socket_fd, buf.data(), len, 0);

        if (rcv_bytes <= 0) {
            throw std::runtime_error("Connection closed or error occurred");
        }
        if (rcv_bytes != len) {
            throw std::runtime_error("Receive failed");
        }

        return {buf.begin(), buf.end()};
    }

    void sendString(const std::string& str) const {
        const size_t len = str.length();
        if (send(socket_fd, &len, sizeof(len), 0) == -1) {
            throw std::runtime_error("Send failed");
        }
        if (send(socket_fd, str.c_str(), len, 0) == -1) {
            throw std::runtime_error("Send failed");
        }
    }

    void registerName(const std::string& name) {
        {
            sendString(name);
            std::string response = receiveString();
            if (response == "OK") {
                std::cout << "Name registered successfully!" << std::endl;
            } else {
                std::cout << "Name already taken. Enter another name: ";
                std::string new_name;
                std::getline(std::cin, new_name);
                registerName(new_name);
            }
        }
    }

    void run() {
        std::cout << "Enter your name: ";
        std::string name;
        std::getline(std::cin, name);
        registerName(name);

        std::cout << "Available commands:\ncompile\ngame\ndisconnect\nEnter command:" << std::endl;
        std::string message, response;

        while (true) {
            std::getline(std::cin, message);
            sendString(message);
            response = receiveString();

            if (response == "NO") {
                std::cout << "No such command. Try again:" << std::endl;
                continue;
            }
            if (message == "compile") {
                std::cout << "Enter path to file: ";
                std::string file_path;
                std::getline(std::cin, file_path);
                if (!std::filesystem::exists(file_path)) {
                    throw std::runtime_error("No such file or directory");
                }
                auto file_sz = std::filesystem::file_size(file_path);
                if (send(socket_fd, &file_sz, sizeof(file_sz), 0) == -1) {
                    throw std::runtime_error("Send failed");
                }

                char buf[BUFSIZ];
                FILE* file = fopen(file_path.c_str(), "rb");
                if (file == nullptr) {
                    throw std::runtime_error("File not found");
                }
                size_t bytes;
                while ((bytes = fread(buf, sizeof(char), BUFSIZ, file)) > 0) {
                    if (send(socket_fd, buf, bytes, 0) == -1) {
                        throw std::runtime_error("Send failed");
                    }
                }
                response = receiveString();
                if (response == "NO") {
                    throw std::runtime_error("Dead file");
                }
                //TODO: получить скомпилированный файл
            } else if (message == "game") {

            } else if (message == "disconnect") {
                break;
            } else {

            }

        }
        // Wait for any incoming messages
        try {
            std::string incoming = receiveString();
            if (!incoming.empty()) {
                std::cout << "\nNew message: " << incoming << std::endl;
            }
        } catch (...) {
            // No messages or connection error
        }
    }

    ~Client() {
        if (socket_fd != -1) {
            close(socket_fd);
        }
    }
};

int main() {
    try {
        Client client("127.0.0.1", 5000);
        client.connectToServer();
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}