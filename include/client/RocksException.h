#pragma once

#include <string>
#include <stdexcept>

class RocksException : public std::runtime_error {

public:
    RocksException() : std::runtime_error(NAME) {}
    virtual ~RocksException() = default;

    explicit RocksException(char* message);
    explicit RocksException(const char* message);
    RocksException(const char* file, long line, const char* message);
    explicit RocksException(const std::string& message);
    explicit RocksException(std::string& message);
    RocksException(const char* file, long line, std::string& message);

private:
    static std::string l2s(long value);
    static const std::string NAME;
};
