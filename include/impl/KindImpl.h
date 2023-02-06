#pragma once

#include "api/Kind.h"
#include "impl/SameType.h"
#include "rocksdb/db.h"
#include <string>

class KindImpl : public Kind, protected SameType {
public:

    KindImpl();

    KindImpl(const char* name, rocksdb::ColumnFamilyHandle* handle);

    const char* name() const noexcept override;

    bool isValid() const noexcept override;

    bool operator<(const Kind& other) const noexcept override;

    bool equals(const RocksType* other) const noexcept override;

    size_t hash() const noexcept override;

    bool operator==(const RocksType& other) const noexcept override;

    const char* toString() const noexcept override;

    rocksdb::ColumnFamilyHandle* handle() const noexcept;

    void closeHandle();

    ~KindImpl() override;

private:
    std::string name_;
    rocksdb::ColumnFamilyHandle* handle_;
};
