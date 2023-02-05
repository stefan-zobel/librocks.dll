#pragma once

#include "api/api.h"
#include "api/Store.h"


#ifdef __cplusplus
extern "C" {
#endif

LIBROCKS_API Store* openStore(int* status, const char* path);

#ifdef __cplusplus
}
#endif
