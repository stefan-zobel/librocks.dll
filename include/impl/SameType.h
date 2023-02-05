#pragma once

#include "api/RocksType.h"
#include <typeinfo>

class SameType
{
protected:
    virtual bool isSameType(const RocksType* other) const final {
        return other != nullptr && typeid(*this) == typeid(*other);
    }
};

