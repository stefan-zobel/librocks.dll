
#include "impl/KindImpl.h"

KindImpl::KindImpl()
    : name_(""), handle_(nullptr) {
}

KindImpl::KindImpl(const char* name, rocksdb::ColumnFamilyHandle* handle)
    : name_(name), handle_(handle) {
}

const char* KindImpl::name() const noexcept {
    return name_.c_str();
}

bool KindImpl::isValid() const noexcept {
    return handle_ != nullptr;
}

bool KindImpl::operator<(const Kind& other) const noexcept {
    return name_ < other.name();
}

bool KindImpl::equals(const RocksType* other) const noexcept {
    if (isSameType(other)) {
        return name_ == static_cast<const KindImpl&>(*other).name_;
    }
    return false;
}

size_t KindImpl::hash() const noexcept {
    return std::hash<std::string>{}(name_);
}

bool KindImpl::operator==(const RocksType& other) const noexcept {
    return equals(&other);
}

const char* KindImpl::toString() const noexcept {
    return name();
}

rocksdb::ColumnFamilyHandle* KindImpl::handle() const noexcept {
    return handle_;
}

void KindImpl::closeHandle() {
    delete handle_;
    handle_ = nullptr;
}
