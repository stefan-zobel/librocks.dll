#pragma once

#include "api/api.h"
#include "api/Store.h"
//#include "impl/KueueManager.h" // TODO: only for temporary testing


#ifdef __cplusplus
extern "C" {
#endif

LIBROCKS_API Store* openStore(int* status, const char* path);

// TODO: only for temporary testing
//LIBROCKS_API KueueManagerImpl* openKueueManagerImpl(int* status, const char* path);

#ifdef __cplusplus
}
#endif
