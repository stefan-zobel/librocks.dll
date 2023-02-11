#pragma once

#include <string>

class RocksException {

public:
    RocksException() = default;
    virtual ~RocksException() = default;

    explicit RocksException(char* message);
    explicit RocksException(const char* message);
    RocksException(const char* file, long line, const char* message);
    explicit RocksException(const std::string& message);
    explicit RocksException(std::string& message);
    RocksException(const char* file, long line, std::string& message);

    const char* what() const noexcept;

private:
    std::string msg;

private:
    static std::string l2s(long value);
};
