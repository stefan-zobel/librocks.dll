#pragma once

#include "api/api.h"

struct LIBROCKS_API Kueue {

    virtual void put(int* status, const char* value, size_t valLen) noexcept = 0;

    virtual char* take(int* status, size_t* valLen) noexcept = 0;

    virtual void clear(int* status) noexcept = 0;

    virtual ~Kueue() = default;
};
