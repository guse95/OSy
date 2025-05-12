#include "Logger.h"

#include <fstream>


int main() {
    Logger* my_logger = LoggerBuilder().set_level(1)
                            .add_handler(std::cout)
                            .add_handler(std::make_unique<std::ofstream>("test.txt"))
                            .make_object();
    my_logger->message(INFO, "Hello World!");
    delete my_logger;
}
