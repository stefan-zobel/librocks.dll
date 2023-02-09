
#include "client/bytes.h"
#include <stdio.h> // printf TODO: remove

bytes::bytes(bytes const& other) : size_(0), data_(nullptr) {
    if (other.size_ > 0) {
        copy(other);
    }
    printf("bytes copy constructor called\n");
}

bytes& bytes::operator=(bytes const& other) {
    printf("bytes copy assignment operator called\n");
    if (this != &other) {
        clear();
        if (other.size_ > 0) {
            copy(other);
        }
    }
    return *this;
}

bytes& bytes::operator=(std::nullptr_t) {
    clear();
    return *this;
}

bytes::bytes(bytes&& moving) noexcept : size_(0), data_(nullptr) {
    moving.swap(*this);
    printf("bytes move constructor called\n");
}

bytes& bytes::operator=(bytes&& moving) noexcept {
    moving.swap(*this);
    printf("bytes move assignment operator called\n");
    return *this;
}

bytes::~bytes() {
    clear();
}

void bytes::clear() {
    if (size_ > 0) {
        size_ = 0;
        delete[] data_;
        data_ = nullptr;
    }
}

void bytes::swap(bytes& src) noexcept {
    size_t tmp = size_;
    size_ = src.size_;
    src.size_ = tmp;
    std::swap(data_, src.data_);
}

void bytes::copy(bytes const& other) {
    size_ = other.size_;
    char* tmp = new char[other.size_];
    std::memcpy(tmp, other.data_, other.size_);
    data_ = tmp;
}

void swap(bytes& lhs, bytes& rhs) {
    lhs.swap(rhs);
}
