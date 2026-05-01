#pragma once

namespace benchmark::policies {
    struct TryPush {
        static constexpr const char* name() {
            return "Try";
        }

        template<typename Queue, typename Payload>
        void operator()(Queue &queue, const Payload &payload) {
            while (!queue.try_push(payload)) {}
        }
    };

    struct BlockingPush {
        static constexpr const char* name() {
            return "Blocking";
        }

        template<typename Queue, typename Payload>
        void operator()(Queue &queue, const Payload &payload) {
            queue.push(payload);
        }
    };

    struct TryPop {
        static constexpr const char* name() {
            return "Try";
        }

        template<typename Queue, typename Payload>
        void operator()(Queue &queue, Payload &payload) {
            while (!queue.try_pop(payload)) {}
        }
    };

    struct BlockingPop {
        static constexpr const char* name() {
            return "Blocking";
        }

        template<typename Queue, typename Payload>
        void operator()(Queue &queue, Payload &payload) {
            queue.pop(payload);
        }
    };
} // namespace benchmark::policies