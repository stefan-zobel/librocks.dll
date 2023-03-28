
#include "impl/utils.h"
#include "api/api.h"
#include <string>

constexpr size_t MAX_BUF_LEN = 64;
constexpr size_t MAX_PATH_LEN = 2048;

static bool dummy;

static std::string ownPath() {
    HMODULE h = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR) &dummy, &h)) {
        char buf[MAX_PATH_LEN] = {};
        if (GetModuleFileNameA(h, &buf[0], MAX_PATH_LEN)) {
            char* last = strrchr(buf, '\\');
            if (last) {
                size_t pos = last - &buf[0];
                return std::string(&buf[0], pos).append("\\");
            }
        }
    }
    return {};
}

static const std::string today() {
    char buffer[MAX_BUF_LEN];
    if (GetDateFormatA(LOCALE_USER_DEFAULT, 0, nullptr, "-yyyy'-'MM'-'dd", &buffer[0], MAX_BUF_LEN) == 0) {
        return std::string("Error in today");
    }
    return std::string(&buffer[0]);
}

FILE* openLogfile() {
    FILE* f = nullptr;
    std::string path = ownPath().append("librocks").append(today()).append(".log");
    if (fopen_s(&f, path.c_str(), "a+") == 0) {
        return f;
    }
    return nullptr;
}
