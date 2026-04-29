#pragma once

#include <atomic>

namespace lock_free::utils {

    template<typename T>
    struct ReadySlot {
        std::atomic<bool> ready_{false};
        T value_;
    };

} // namespace lock_free::utils
