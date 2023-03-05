#pragma once

#include "impl/StoreImpl.h"
#include "impl/KueueImpl.h"
#include "impl/utils.h"
#include "api/KueueManager.h"

class KueueManagerImpl : public KueueManager {
public:
    KueueManagerImpl(int* status, const char* path) : store(nullptr) {
        assign(Ok, status);
        StoreImpl* impl = new StoreImpl(path);
        if (impl->isOpen()) {
            store = impl;
        }
        else {
            assign(impl->getCode(), status);
            impl->close();
            delete impl;
        }
    }

    KueueImpl* get(int* status, const char* id) noexcept {
        synchronize(monitor);
        if (validateOpen(status)) {
            if (auto queue = queues.find(std::string(id)); queue != queues.end()) {
                return queue->second;
            }
            else {
                std::string identifier = id;
                KueueImpl* pKueue = new KueueImpl(store, identifier);
                queues[identifier] = pKueue;
                return pKueue;
            }
        }
        return nullptr;
    }

    inline bool isOpen() const noexcept {
        return store != nullptr && store->isOpen();
    }

    void close() noexcept {
        synchronize(monitor);
        close_();
    }

    ~KueueManagerImpl() {
        close_();
    }

private:
    void close_() {
        if (isOpen()) {
            for (auto& it : queues) {
                delete it.second;
            }
            queues.clear();
            if (store) {
                delete store;
            }
            store = nullptr;
        }
    }

    inline bool validateOpen(int* status) const noexcept {
        if (isOpen()) {
            assign(Ok, status);
            return true;
        }
        assign(Closed, status);
        return false;
    }

private:
    mutable std::mutex monitor;
    StoreImpl* store;
    std::unordered_map<std::string, KueueImpl*> queues;
};
