#include <immintrin.h>
#include <memory>

#include "../utils/padded_atomic.hpp"
#include "../utils/sequence_slot.hpp"

namespace lock_free::mpmc {
    using utils::PaddedAtomic;
    using utils::SequenceSlot;

    template<typename T>
    class Queue {
    private:
        PaddedAtomic<size_t> head_;
        PaddedAtomic<size_t> tail_;

        size_t capacity_;

        std::unique_ptr<SequenceSlot<T>[]> buffer_;

    public:
        static constexpr const char *name() {
            return "MPMC";
        }

        static constexpr const char* full_name() {
            return "Lock-Free Multi Producer Multi Consumer Queue";
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
              buffer_{std::make_unique<SequenceSlot<T>[]>(capacity)} {
        for (size_t i = 0; i < capacity; i++) {
            buffer_[i].seq_.store(i, std::memory_order_relaxed);
        }
    }

    template<typename T>
    bool Queue<T>::full() const {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        return tail - head >= capacity_;
    }

    template<typename T>
    bool Queue<T>::empty() const {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        return tail <= head;
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
        size_t pos = head_.value_.fetch_add(1, std::memory_order_relaxed);

        size_t index = pos % capacity_;

        while (buffer_[index].seq_.load(std::memory_order_acquire) != (pos + 1)) {
            _mm_pause();
        }

        value = buffer_[index].value_;
        buffer_[index].seq_.store(pos + capacity_, std::memory_order_release);
    }

    template<typename T>
    bool Queue<T>::try_pop(T &value) {
        size_t head = head_.value_.load(std::memory_order_relaxed);
        size_t tail = tail_.value_.load(std::memory_order_acquire);

        if (head >= tail) {
            return false;
        }

        size_t index = head % capacity_;

        if (buffer_[index].seq_.load(std::memory_order_acquire) != (head + 1)) {
            return false;
        }

        if (!head_.value_.compare_exchange_weak(head, head + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            return false;
        }

        value = buffer_[index].value_;
        buffer_[index].seq_.store(head + capacity_, std::memory_order_release);
        return true;
    }

    template<typename T>
    void Queue<T>::push(const T &value) {
        size_t pos = tail_.value_.fetch_add(1, std::memory_order_relaxed);

        size_t index = pos % capacity_;

        while (buffer_[index].seq_.load(std::memory_order_acquire) != pos) {
            _mm_pause();
        }

        buffer_[index].value_ = value;
        buffer_[index].seq_.store(pos + 1, std::memory_order_release);
    }

    template<typename T>
    bool Queue<T>::try_push(const T &value) {
        size_t head = head_.value_.load(std::memory_order_acquire);
        size_t tail = tail_.value_.load(std::memory_order_relaxed);

        if (tail - head >= capacity_) {
            return false;
        }

        size_t index = tail % capacity_;

        if (buffer_[index].seq_.load(std::memory_order_acquire) != tail) {
            return false;
        }

        if (!tail_.value_.compare_exchange_weak(tail, tail + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            return false;
        }

        buffer_[index].value_ = value;
        buffer_[index].seq_.store(tail + 1, std::memory_order_release);
        return true;
    }
} // namespace lock_free::mpmc