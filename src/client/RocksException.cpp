
#include "client/RocksException.h"


RocksException::RocksException(char* message) {
    msg.append(message).append("\n");
}

RocksException::RocksException(const char* message) {
    msg.append(message).append("\n");
}

RocksException::RocksException(const char* file, long line, const char* message) {
    msg.append(file).append(":").append(l2s(line)).append(": ").append(message).append("\n");
}

RocksException::RocksException(std::string message) {
    msg.append(message).append("\n");
}

RocksException::RocksException(std::string& message) {
    msg.append(message).append("\n");
}

RocksException::RocksException(const char* file, long line, std::string& message) {
    msg.append(file).append(":").append(l2s(line)).append(": ").append(message).append("\n");
}

std::string RocksException::l2s(long value) {
    char buffer[64] = { 0 };
    sprintf_s(buffer, 64, "%ld", value);
    return std::string(buffer);
}

const char* RocksException::what() const {
    return msg.c_str();
}
