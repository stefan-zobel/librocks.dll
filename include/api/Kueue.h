#pragma once

#include "api/api.h"
#include "api/Clearable.h"

struct LIBROCKS_API Kueue : public Clearable {

    virtual void put(int* status, const char* value, size_t valLen) noexcept = 0;

    virtual char* take(int* status, size_t* valLen) noexcept = 0;

    virtual void clear(int* status) noexcept override = 0;

    virtual ~Kueue() = default;
};
