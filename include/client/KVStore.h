#pragma once

#include <string>
#include <map>
#include "client/bytes.h"
#include "api/Kind.h"
#include "api/Store.h"

class KVStore {
public:

    explicit KVStore(Store* store);

    void put(const Kind& kind, std::string& key, std::string& value);

    void remove(const Kind& kind, std::string& key);

    bytes get(const Kind& kind, std::string& key) const;

    bytes updateIfPresent(const Kind& kind, std::string& key, std::string& value);

    void singleRemove(const Kind& kind, std::string& key);

    bytes singleRemoveIfPresent(const Kind& kind, std::string& key);

    bytes removeIfPresent(const Kind& kind, std::string& key);

    bool putIfAbsent(const Kind& kind, std::string& key, std::string& value);

    bytes findMinKey(const Kind& kind) const;

    bytes findMaxKey(const Kind& kind) const;

private:
    Store* store;

private:
    static const std::map<int, std::string> codes;
    static bool throwForStatus(int status);
};
