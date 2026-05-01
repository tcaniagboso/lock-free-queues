#pragma once

#include <array>
#include <cstdint>

namespace benchmark::payload {
    struct Payload16 {
        uint64_t id;
        std::array<char, 16 - sizeof(uint64_t)> data;
    };
    static_assert(sizeof(Payload16) == 16);

    struct alignas(64) Payload64 {
        uint64_t id;
        std::array<char, 64 - sizeof(uint64_t)> data;
    };
    static_assert(sizeof(Payload64) == 64);

    struct Payload256 {
        uint64_t id;
        std::array<char, 256 - sizeof(uint64_t)> data;
    };
    static_assert(sizeof(Payload256) == 256);
} // namespace benchmark::payload