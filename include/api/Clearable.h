#pragma once

#include <api/api.h>

struct LIBROCKS_API Clearable {

    virtual bool clear() = 0;

    virtual ~Clearable() = default;
};
