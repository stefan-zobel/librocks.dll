#pragma once

#include "api/api.h"
#include <chrono>
#include <string>
#include <mutex>


#define synchronize(mut) \
    std::lock_guard<std::recursive_mutex> guard((mut))


inline void assign(int code, int* status) {
    if (status) {
        *status = code;
    }
}


FILE* openLogfile();


template <
    class result_t = std::chrono::milliseconds,
    class clock_t = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
std::chrono::milliseconds since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}
