
#include "api/librocks.h"
#include "impl/utils.h"
#include "impl/StoreImpl.h"
#include "impl/KueueManagerImpl.h"


Store* openStore(int* status, const char* path) {
    assign(Ok, status);
    auto impl = new StoreImpl(path);
    if (impl->isOpen()) {
        return impl;
    }
    assign(impl->getCode(), status);
    impl->close();
    delete impl;
    return nullptr;
}

KueueManager* openKueueManager(int* status, const char* path) {
    assign(Ok, status);
    auto mgr = new KueueManagerImpl(status, path);
    if (mgr->isOpen()) {
        return mgr;
    }
    mgr->close();
    delete mgr;
    return nullptr;
}
