#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>


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

    void put() noexcept { // XXX
        long long c = -1L;
        {
            std::lock_guard<std::mutex> lock(putLock);
            // TODO: put(...)
            c = count.fetch_add(1L);
            ++totalPuts_;
        }
        if (c == 0L) {
            signalNotEmpty();
        }
    }

    void take() noexcept { // XXX
        long long c = -1L;
        std::unique_lock<std::mutex> lock(takeLock);
        while (count.load() == 0L) {
            notEmpty.wait(lock);
        }
        // TODO: take(...)
        c = count.fetch_sub(1L);
        ++totalTakes_;
        if (c > 1L) {
            // signal other waiting takers
            notEmpty.notify_one();
        }
        // TODO: return ...
    }

    void clear() noexcept {
        // Locks to prevent both puts and takes
        std::lock_guard<std::mutex> put(putLock);
        std::lock_guard<std::mutex> take(takeLock);
        while (count.load() > 0L) {
            // TODO: clear(...)
            count.fetch_sub(1L);
            ++totalTakes_;
        }
        // Unlocks to allow both puts and takes
    }

    long long size() const noexcept {
        return count.load();
    }

    bool isEmpty() const noexcept {
        return size() == 0L;
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

private:
    // Total number of successful puts
    unsigned long long totalPuts_{0uLL};
    // Total number of successful takes
    unsigned long long totalTakes_{0uLL};
    // Current number of messages
    std::atomic_llong count{0LL};
    // Lock held by put
    std::mutex putLock;
    // Lock held by take
    std::mutex takeLock;
    // Wait queue for waiting takes
    std::condition_variable notEmpty;
};
