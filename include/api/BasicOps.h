#pragma once

#include "api/api.h"
#include "api/Kind.h"

struct LIBROCKS_API BasicOps {

    virtual void put(int* status, const Kind& kind, const char* key, size_t keyLen, const char* value,
        size_t valLen) noexcept = 0;

    virtual void remove(int* status, const Kind& kind, const char* key, size_t keyLen) noexcept = 0;

    virtual char* get(int* status, const Kind& kind, size_t* resultLen, const char* key,
        size_t keyLen) const noexcept = 0;

    virtual char* updateIfPresent(int* status, const Kind& kind, size_t* resultLen, const char* key,
        size_t keyLen, const char* value, size_t valLen) noexcept = 0;

    virtual ~BasicOps() = default;
};
