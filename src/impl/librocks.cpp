
#include "api/librocks.h"
#include "impl/utils.h"
#include "impl/StoreImpl.h"


Store* openStore(int* status, const char* path) {
    assign(Ok, status);
    StoreImpl* impl = new StoreImpl(path);
    if (impl->isOpen()) {
        return impl;
    }
    assign(impl->getCode(), status);
    impl->close();
    delete impl;
    return nullptr;
}

// TODO: only for temporary testing
//KueueManagerImpl* openKueueManagerImpl(int* status, const char* path) {
//    assign(Ok, status);
//    KueueManagerImpl* mgr = new KueueManagerImpl(status, path);
//    if (mgr->isOpen()) {
//        return mgr;
//    }
//    assign(Closed, status);
//    mgr->close();
//    delete mgr;
//    return nullptr;
//}
