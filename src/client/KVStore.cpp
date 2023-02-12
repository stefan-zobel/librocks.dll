
#include "client/KVStore.h"
#include "client/RocksException.h"

KVStore::KVStore(Store* pStore) : store(pStore) {
}

KVStore::~KVStore() {
    if (store) {
        delete store;
        store = nullptr;
    }
}

void KVStore::close() {
    if (store) {
        store->close();
    }
}

bool KVStore::isOpen() const noexcept {
    return store->isOpen();
}

const Kind& KVStore::getDefaultKind() const {
    int status = Ok;
    const Kind& k = getKindManager().getDefaultKind(&status);
    if (status != Ok) {
        throwForStatus(status);
    }
    return k;
}

const Kind& KVStore::getOrCreateKind(std::string& kindName) {
    int status = Ok;
    const Kind& k = getKindManager().getOrCreateKind(&status, kindName.c_str());
    if (status != Ok) {
        throwForStatus(status);
    }
    return k;
}

bool comparator(const std::reference_wrapper<const Kind>& a, const std::reference_wrapper<const Kind>& b) noexcept {
    return a.get() < b.get();
}

const KindSet KVStore::getKinds() const {
    int status = Ok;
    size_t resultLen = 0;
    const Kind** ppK = getKindManager().getKinds(&status, &resultLen);
    KindSet kinds(&comparator);
    if (status == Ok) {
        for (size_t i = 0; i < resultLen; ++i) {
            const Kind& kind = dynamic_cast<const Kind&>(*ppK[i]);
            kinds.insert(std::cref(kind));
        }
        delete[] ppK;
    }
    else {
        throwForStatus(status);
    }
    return kinds;
}

KindManager& KVStore::getKindManager() const {
    int status = Ok;
    KindManager& mgr = store->getKindManager(&status);
    if (status != Ok) {
        throwForStatus(status);
    }
    return mgr;
}

void KVStore::put(const Kind& kind, std::string& key, std::string& value) {
    int status = Ok;
    store->put(&status, kind, key.data(), key.size(), value.data(), value.size());
    if (status != Ok) {
        throwForStatus(status);
    }
}

void KVStore::remove(const Kind& kind, std::string& key) {
    int status = Ok;
    store->remove(&status, kind, key.data(), key.size());
    if (status != Ok) {
        throwForStatus(status);
    }
}

bytes KVStore::get(const Kind& kind, std::string& key) const {
    int status = Ok;
    size_t resultLen = 0;
    char* val = store->get(&status, kind, &resultLen, key.data(), key.size());
    if (!(status == Ok || status == NotFound)) {
        throwForStatus(status);
    }
    return bytes(val, resultLen);
}

bytes KVStore::updateIfPresent(const Kind& kind, std::string& key, std::string& value) {
    int status = Ok;
    size_t resultLen = 0;
    char* oldVal = store->updateIfPresent(&status, kind, &resultLen, key.data(), key.size(), value.data(), value.size());
    if (!(status == Ok || status == NotFound)) {
        throwForStatus(status);
    }
    return bytes(oldVal, resultLen);
}

void KVStore::singleRemove(const Kind& kind, std::string& key) {
    int status = Ok;
    store->singleRemove(&status, kind, key.data(), key.size());
    if (status != Ok) {
        throwForStatus(status);
    }
}

bytes KVStore::singleRemoveIfPresent(const Kind& kind, std::string& key) {
    int status = Ok;
    size_t resultLen = 0;
    char* removed = store->singleRemoveIfPresent(&status, kind, &resultLen, key.data(), key.size());
    if (!(status == Ok || status == NotFound)) {
        throwForStatus(status);
    }
    return bytes(removed, resultLen);
}

bytes KVStore::removeIfPresent(const Kind& kind, std::string& key) {
    int status = Ok;
    size_t resultLen = 0;
    char* removed = store->removeIfPresent(&status, kind, &resultLen, key.data(), key.size());
    if (!(status == Ok || status == NotFound)) {
        throwForStatus(status);
    }
    return bytes(removed, resultLen);
}

bool KVStore::putIfAbsent(const Kind& kind, std::string& key, std::string& value) {
    int status = Ok;
    store->putIfAbsent(&status, kind, key.data(), key.size(), value.data(), value.size());
    if (status == Ok) {
        return true;
    }
    else if (status == AlreadyExists) {
        return false;
    }
    return throwForStatus(status);
}

bytes KVStore::findMinKey(const Kind& kind) const {
    int status = Ok;
    size_t resultLen = 0;
    char* minKey = store->findMinKey(&status, kind, &resultLen);
    if (status != Ok) {
        throwForStatus(status);
    }
    return bytes(minKey, resultLen);
}

bytes KVStore::findMaxKey(const Kind& kind) const {
    int status = Ok;
    size_t resultLen = 0;
    char* maxKey = store->findMaxKey(&status, kind, &resultLen);
    if (status != Ok) {
        throwForStatus(status);
    }
    return bytes(maxKey, resultLen);
}

void KVStore::compact(const Kind& kind) {
    int status = Ok;
    store->compact(&status, kind);
    if (status != Ok) {
        throwForStatus(status);
    }
}

void KVStore::compactAll() {
    int status = Ok;
    store->compactAll(&status);
    if (status != Ok) {
        throwForStatus(status);
    }
}


bool KVStore::throwForStatus(int status) {
    if (status != Ok) {
        if (KVStore::codes.count(status)) {
            throw RocksException(KVStore::codes.at(status));
        }
        else {
            throw RocksException("Unknown");
        }
    }
    return false;
}

static std::map<int, std::string> initCodes() {
    std::map<int, std::string> m;
    m[NoIterator] = "NoIterator";
    m[AlreadyExists] = "AlreadyExists";
    m[NoTransaction] = "NoTransaction";
    m[Closed] = "Closed";
    m[Ok] = "Ok";
    m[NotFound] = "NotFound";
    m[Corruption] = "Corruption";
    m[NotSupported] = "NotSupported";
    m[InvalidArgument] = "InvalidArgument";
    m[IOError] = "IOError";
    m[MergeInProgress] = "MergeInProgress";
    m[Incomplete] = "Incomplete";
    m[ShutdownInProgress] = "ShutdownInProgress";
    m[TimedOut] = "TimedOut";
    m[Aborted] = "Aborted";
    m[Busy] = "Busy";
    m[Expired] = "Expired";
    m[TryAgain] = "TryAgain";
    m[CompactionTooLarge] = "CompactionTooLarge";
    m[ColumnFamilyDropped] = "ColumnFamilyDropped";
    m[Unknown] = "Unknown";
    return m;
}

const std::map<int, std::string> KVStore::codes = initCodes();
