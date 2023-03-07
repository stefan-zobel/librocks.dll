#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include "api/Kueue.h"
#include "impl/StoreImpl.h"


constexpr unsigned __int64 MIN_KEY = 0x0000000000000000;
constexpr unsigned __int64 MAX_KEY = 0xFFFFFFFFFFFFFFFF;


static char* putU64BE(unsigned __int64 x, char* dst) {
    unsigned __int64 be = _byteswap_uint64(x);
    memcpy(dst, reinterpret_cast<unsigned char*>(&be), sizeof(unsigned __int64));
    return dst;
}

static unsigned __int64 getU64BE(const char* src) {
    unsigned __int64 be = 0uLL;
    memcpy(&be, src, sizeof(unsigned __int64));
    return _byteswap_uint64(be);
}

class KueueImpl : public Kueue {
public:

    KueueImpl(StoreImpl* kvStore, std::string id) : count(0LL), minKey(MIN_KEY), maxKey(MIN_KEY), store(kvStore) {
        int state = Ok;
        KindManager& manager = store->getKindManager(&state);
        if (state == Ok) {
            const Kind& kind = manager.getOrCreateKind(&state, id.c_str());
            if (state == Ok && kind.isValid()) {
                queueKind = &const_cast<Kind&>(kind);
                size_t length = 0;
                char* smallestKey = store->findMinKey(&state, kind, &length);
                if (state == Ok) {
                    if (smallestKey && length == sizeof(unsigned __int64)) {
                        minKey = getU64BE(smallestKey);
                    }
                    else {
                        // minKey remains at MIN_KEY
                    }
                    char* greatestKey = store->findMaxKey(&state, kind, &length);
                    if (state == Ok) {
                        unsigned __int64 lastMax = maxKey;
                        if (greatestKey && length == sizeof(unsigned __int64)) {
                            lastMax = getU64BE(greatestKey);
                            unsigned __int64 nextMax = lastMax;
                            maxKey = ++nextMax;
                        }
                        else {
                            // maxKey and lastMax remain at MIN_KEY
                        }
                        if (lastMax >= minKey) {
                            count.store(maxKey - minKey);
                            isValid_ = (count.load() >= 0LL);
                        }
                    }
                }
            }
        }
    }

    void put(int* status, const char* value, size_t valLen) noexcept {
        int r = Ok;
        int* state = status ? status : &r;
        if (isValid_) {
            long long c = -1LL;
            {
                char key[sizeof(unsigned __int64)];
                std::lock_guard<std::mutex> lock(putLock);
                store->put(state, getKindRef(), putU64BE(maxKey++, &key[0]), sizeof(unsigned __int64), value, valLen);
                if (*state == Ok) {
                    c = count.fetch_add(1LL);
                    ++totalPuts_;
                }
                else {
                    --maxKey;
                }
            }
            if (c == 0LL) {
                signalNotEmpty();
            }
        }
        else {
            *state = Invalid;
        }
    }

    char* take(int* status, size_t* valLen) noexcept {
        int r = Ok;
        int* state = status ? status : &r;
        if (isValid_) {
            long long c = -1LL;
            char key[sizeof(unsigned __int64)];
            std::unique_lock<std::mutex> lock(takeLock);
            while (count.load() == 0LL) {
                notEmpty.wait(lock);
            }
            char* value = store->singleRemoveIfPresent(state, getKindRef(), valLen, putU64BE(minKey, &key[0]),
                sizeof(unsigned __int64));
            if (*state == Ok) {
                ++minKey;
                c = count.fetch_sub(1LL);
                ++totalTakes_;
            }
            if (c > 1LL) {
                // signal other waiting takers
                notEmpty.notify_one();
            }
            return value;
        }
        else {
            *state = Invalid;
            return nullptr;
        }
    }

    void clear(int* status) noexcept {
        int r = Ok;
        int* state = status ? status : &r;
        if (isValid_) {
            char key[sizeof(unsigned __int64)];
            const Kind& kindRef = getKindRef();
            // Locks to prevent both puts and takes
            std::lock_guard<std::mutex> put(putLock);
            std::lock_guard<std::mutex> take(takeLock);
            while (count.load() > 0LL) {
                store->singleRemove(state, kindRef, putU64BE(minKey, &key[0]), sizeof(unsigned __int64));
                if (*state == Ok) {
                    ++minKey;
                    count.fetch_sub(1LL);
                    ++totalTakes_;
                }
                else {
                    break;
                }
            }
            // Unlocks to allow both puts and takes
        }
        else {
            *state = Invalid;
        }
    }

    long long size() const noexcept {
        return count.load();
    }

    bool isEmpty() const noexcept {
        return size() == 0LL;
    }

    bool isValid() const noexcept {
        return isValid_;
    }

    unsigned long long totalPuts() const noexcept {
        return totalPuts_;
    }

    unsigned long long totalTakes() const noexcept {
        return totalTakes_;
    }

private:

    // Signals a waiting take. Called only from put.
    void signalNotEmpty() noexcept {
        std::unique_lock<std::mutex> lock(takeLock);
        notEmpty.notify_one();
    }

    const Kind& getKindRef() {
        return *const_cast<const Kind*>(queueKind);
    }

private:
    // Total number of successful puts
    unsigned long long totalPuts_{0uLL};
    // Total number of successful takes
    unsigned long long totalTakes_{0uLL};
    // Current number of messages
    std::atomic_llong count;
    // Lock held by put
    std::mutex putLock;
    // Lock held by take
    std::mutex takeLock;
    // Wait queue for waiting takes
    std::condition_variable notEmpty;
    unsigned __int64 minKey;
    unsigned __int64 maxKey;
    // backing store
    StoreImpl* store;
    // Kind object for this Kueue
    Kind* queueKind = nullptr;
    // true if Kueue initialization succeeds, otherwise false
    bool isValid_ = false;
};
