#pragma once

#include <array>
#include <atomic>

namespace lock_free::utils {

    template<typename T>
    struct alignas(64) PaddedAtomic {
        std::atomic<T> value_{};
        std::array<char, 64 - sizeof(std::atomic<size_t>)> padding_{};
    };

    static_assert(sizeof(PaddedAtomic<size_t>) == 64);
} // namespace lock_free::utils