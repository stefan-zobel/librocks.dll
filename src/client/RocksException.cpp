
#include "client/RocksException.h"


const std::string RocksException::NAME = "RocksException - ";

RocksException::RocksException(char* message) 
    : std::runtime_error(std::string(NAME).append(message).append("\n")) {
}

RocksException::RocksException(const char* message) 
    : std::runtime_error(std::string(NAME).append(message).append("\n")) {
}

RocksException::RocksException(const char* file, long line, const char* message) 
    : std::runtime_error(std::string(NAME).append(file).append(":").append(l2s(line))
        .append(": ").append(message).append("\n")) {
}

RocksException::RocksException(const std::string& message) 
    : std::runtime_error(std::string(NAME).append(message).append("\n")) {
}

RocksException::RocksException(std::string& message) 
    : std::runtime_error(std::string(NAME).append(message).append("\n")) {
}

RocksException::RocksException(const char* file, long line, std::string& message) 
    : std::runtime_error(std::string(NAME).append(file).append(":").append(l2s(line))
        .append(": ").append(message).append("\n")) {
}

std::string RocksException::l2s(long value) {
    char buffer[64] = { 0 };
    sprintf_s(buffer, 64, "%ld", value);
    return std::string(buffer);
}
