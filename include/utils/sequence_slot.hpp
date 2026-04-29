#pragma once

#include <atomic>
#include <cstddef>

namespace lock_free::utils {

    template<typename T>
    struct SequenceSlot {
        std::atomic<size_t> seq_;
        T value_;
    };

} // namespace lock_free::utils
