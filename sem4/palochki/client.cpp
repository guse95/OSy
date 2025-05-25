#include <cmath>
#include <iostream>
#include <string>
#include <exception>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <filesystem>

#define BUFSIZE 4096

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

    void sendBytes(const void* buff, const size_t size) const {
        if (send(socket_fd, &size, sizeof(size), 0) == -1) {
            throw std::runtime_error("Failed to send size");
        }

        if (send(socket_fd, buff, size, 0) == -1) {
            throw std::runtime_error("Failed to send data");
        }
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
    void sendFile(FILE* file) const {
        char buf[BUFSIZE];
        size_t bytes;
        while ((bytes = fread(buf, sizeof(char), BUFSIZE, file)) > 0) {
            sendBytes(buf, bytes);
        }
    }

    void rcvFile(FILE* file, const std::string& sz) const {
        const size_t file_sz = stoi(sz);
        for (size_t i = 0; i < file_sz; i++) {
            std::string resp = receiveString();
            fwrite(resp.c_str(), sizeof(char), resp.size(), file);
        }
    }

    void registerName(const std::string& name) const {
        while (true) {
            sendString(name);
            std::string response = receiveString();
            if (response == "OK") {
                std::cout << "Name registered successfully!" << std::endl;
            } else {
                std::cout << "Name already taken. Enter another name: ";
                std::string new_name;
                std::getline(std::cin, new_name);
                continue;
            }
            break;
        }
    }

    void run() const {
        std::cout << "Enter your name: ";
        std::string name;
        std::getline(std::cin, name);
        registerName(name);

        std::cout << "Available commands:\ncompile\ngame\ndisconnect" << std::endl;
        std::string message, response;

        while (true) {
            std::cout << "Enter command:" << std::endl;
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


                FILE* file;
                while (true) {
                    std::getline(std::cin, file_path);
                    if (!std::filesystem::exists(file_path)) {
                        std::cout << "No such file or directory" << std::endl;
                        continue;
                    }

                    auto file_sz = std::filesystem::file_size(file_path);
                    file_sz = file_sz / BUFSIZE + ((file_sz % BUFSIZE) ? 1 : 0);
                    sendString(std::to_string(file_sz));

                    file = fopen(file_path.c_str(), "rb");
                    if (file == nullptr) {
                        std::cout << "File not found. Try again:" << std::endl;
                        continue;
                    }
                    break;
                }
                sendFile(file);
                fclose(file);

                response = receiveString();
                if (response == "NO") {
                    std::cout << "File cant be compilated." << std::endl;
                    continue;
                }

                std::cout << "Enter name of compiled file:" << std::endl;

                while (true){
                    std::getline(std::cin, file_path);
                    file = fopen(file_path.c_str(), "wb");
                    if (file == nullptr) {
                        std::cout << "File not found. Try again:" << std::endl;
                        continue;
                    }
                    break;
                }
                std::string size_text = receiveString();
                rcvFile(file, size_text);
                fclose(file);

                std::string cmd = "chmod +x " + file_path;
                system(cmd.c_str());

                //TODO: получить скомпилированный файл
            } else if (message == "game") {
                std::cout << "game started" << std::endl;

            } else if (message == "disconnect") {
                std::cout << "disconnecting..." << std::endl;
                break;
            } else {
                std::cout << "Something went wrong." << std::endl;
            }
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