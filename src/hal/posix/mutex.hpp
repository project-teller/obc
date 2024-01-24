#pragma once

#include <cassert>
#include <mutex>

namespace teller::hal {

using mutex = std::mutex;
template <typename T = mutex>
using lock_guard = std::lock_guard<T>;

}
