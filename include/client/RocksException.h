#pragma once

#include <string>

class RocksException {

public:
    RocksException() = default;
    virtual ~RocksException() = default;

    RocksException(char* message);
    RocksException(const char* message);
    RocksException(const char* file, long line, const char* message);
    RocksException(std::string message);
    RocksException(std::string& message);
    RocksException(const char* file, long line, std::string& message);

    const char* what() const;

private:
    std::string msg;

private:
    std::string l2s(long value);
};
