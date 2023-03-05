#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include "impl/StoreImpl.h"


constexpr unsigned __int64 MIN_KEY = 0x0000000000000000;
constexpr unsigned __int64 MAX_KEY = 0xFFFFFFFFFFFFFFFF;


static void putU64BE(unsigned __int64 x, char* dst) {
    unsigned __int64 be = _byteswap_uint64(x);
    memcpy(dst, reinterpret_cast<unsigned char*>(&be), sizeof(unsigned __int64));
}

static unsigned __int64 getU64BE(const char* src) {
    unsigned __int64 be = 0uLL;
    memcpy(&be, src, sizeof(unsigned __int64));
    return _byteswap_uint64(be);
}

class KueueImpl {
public:

    KueueImpl(StoreImpl* kvStore, std::string id) : count(0LL), minKey(MIN_KEY), maxKey(MIN_KEY), store(kvStore), queueId(id) {
        int state = Ok;
        KindManager& manager = store->getKindManager(&state);
        if (state == Ok) {
            const Kind& kind = manager.getOrCreateKind(&state, queueId.c_str());
            if (state == Ok && kind.isValid()) {
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

    void put(int* status, size_t valLen) noexcept { // XXX
        if (isValid_) {
            long long c = -1LL;
            {
                std::lock_guard<std::mutex> lock(putLock);
                //store->put()
                // TODO: put(...)
                c = count.fetch_add(1LL);
                ++totalPuts_;
            }
            if (c == 0LL) {
                signalNotEmpty();
            }
        }
        else {
            *status = Invalid;
        }
    }

    void take(int* status, size_t* valLen) noexcept { // XXX
        if (isValid_) {
            long long c = -1LL;
            std::unique_lock<std::mutex> lock(takeLock);
            while (count.load() == 0LL) {
                notEmpty.wait(lock);
            }
            // TODO: take(...)
            //store->singleRemoveIfPresent();
            c = count.fetch_sub(1LL);
            ++totalTakes_;
            if (c > 1LL) {
                // signal other waiting takers
                notEmpty.notify_one();
            }
            // TODO: return ...
        }
        else {
            *status = Invalid;
            // TODO: return
        }
    }

    void clear(int* status) noexcept {
        if (isValid_) {
            // Locks to prevent both puts and takes
            std::lock_guard<std::mutex> put(putLock);
            std::lock_guard<std::mutex> take(takeLock);
            while (count.load() > 0LL) {
                // TODO: clear(...)
                count.fetch_sub(1LL);
                ++totalTakes_;
            }
            // Unlocks to allow both puts and takes
        }
        else {
            *status = Invalid;
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

public:
    // a void Kueue
    static KueueImpl VoidKueue;

private:

    // Signals a waiting take. Called only from put.
    void signalNotEmpty() noexcept {
        std::unique_lock<std::mutex> lock(takeLock);
        notEmpty.notify_one();
    }
    // constructor for the static void queue instance
    explicit KueueImpl(bool) : count(0LL), minKey(MIN_KEY), maxKey(MIN_KEY), store(nullptr) {
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
    // column family name
    std::string queueId;
    // backing store
    StoreImpl* store;
    // false for the void queue, otherwise true if Kueue initialization succeeds
    bool isValid_ = false;
};

KueueImpl KueueImpl::VoidKueue = KueueImpl(false);
