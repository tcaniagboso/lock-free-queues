#pragma once

#include <immintrin.h>
#include <memory>

#include "../utils/ready_slot.hpp"
#include "../utils/padded_atomic.hpp"

namespace lock_free::mpsc {
    using utils::PaddedAtomic;
    using utils::ReadySlot;

    template<typename T>
    class Queue {
    private:
        PaddedAtomic<size_t> head_;
        PaddedAtomic<size_t> tail_;

        size_t capacity_;

        std::unique_ptr<ReadySlot<T>[]> buffer_;

    public:
        static constexpr const char *name() {
            return "Lock-Free MPSC Queue";
        }

        explicit Queue(size_t capacity);

        [[nodiscard]] bool full() const;

        [[nodiscard]] bool empty() const;

        [[nodiscard]] size_t size() const;

        void pop(T &value);

        bool try_pop(T &value);

        void push(const T &value);

        bool try_push(const T &value);
    };

    template<typename T>
    Queue<T>::Queue(size_t capacity)
            : head_{},
              tail_{},
              capacity_{capacity},
              buffer_{std::make_unique<ReadySlot<T>[]>(capacity)} {}

    template<typename T>
    bool Queue<T>::full() const {
        auto head = head_.value_.load(std::memory_order_relaxed);
        auto tail = tail_.value_.load(std::memory_order_relaxed);

        return tail - head >= capacity_;
    }

    template<typename T>
    bool Queue<T>::empty() const {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        return tail == head;
    }

    template<typename T>
    size_t Queue<T>::size() const {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        size_t used = (tail >= head) ? (tail - head) : 0;
        return (used > capacity_) ? capacity_ : used;
    }

    template<typename T>
    void Queue<T>::pop(T &value) {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_acquire);

        while (tail == head) {
            _mm_pause();
            tail = tail_.value_.load(std::memory_order_acquire);
        }

        size_t index = head % capacity_;
        while (!buffer_[index].ready_.load(std::memory_order_acquire)) {
            _mm_pause();
        }

        value = buffer_[index].value_;
        buffer_[index].ready_.store(false, std::memory_order_release);
        head_.value_.store(head + 1, std::memory_order_release);
    }

    template<typename T>
    bool Queue<T>::try_pop(T &value) {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_acquire);

        if (tail == head) {
            return false;
        }

        size_t index = head % capacity_;
        if (!buffer_[index].ready_.load(std::memory_order_acquire)) {
            return false;
        }

        value = buffer_[index].value_;
        buffer_[index].ready_.store(false, std::memory_order_release);
        head_.value_.store(head + 1, std::memory_order_release);
        return true;
    }

    template<typename T>
    void Queue<T>::push(const T &value) {
        size_t head = head_.value_.load(std::memory_order_acquire);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        while (tail - head >= capacity_) {
            _mm_pause();
            head = head_.value_.load(std::memory_order_acquire);
            tail = tail_.value_.load(std::memory_order_relaxed);
        }

        size_t pos = tail_.value_.fetch_add(1, std::memory_order_relaxed);

        while (pos - head >= capacity_) {
            _mm_pause();
            head = head_.value_.load(std::memory_order_acquire);
        }

        size_t index = pos % capacity_;

        while (buffer_[index].ready_.load(std::memory_order_acquire)) {
            _mm_pause();
        }

        buffer_[index].value_ = value;
        buffer_[index].ready_.store(true, std::memory_order_release);
    }

    template<typename T>
    bool Queue<T>::try_push(const T &value) {
        size_t head = head_.value_.load(std::memory_order_acquire);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        if (tail - head >= capacity_) return false;

        size_t index = tail % capacity_;

        if (buffer_[index].ready_.load(std::memory_order_acquire)) {
            return false;
        }

        if (!tail_.value_.compare_exchange_weak(tail, tail + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
            return false;
        }

        buffer_[index].value_ = value;
        buffer_[index].ready_.store(true, std::memory_order_release);
        return true;
    }
} // namespace lock_free::mpsc