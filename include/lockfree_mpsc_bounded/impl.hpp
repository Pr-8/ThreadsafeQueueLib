#ifndef LOCKFREE_MPSC_BOUNDED_IMPL
#define LOCKFREE_MPSC_BOUNDED_IMPL

#include "defs.hpp"
#include <utility>

namespace tsfqueue::__impl {

template <typename T, size_t Capacity>
lockfree_mpsc_bounded<T, Capacity>::lockfree_mpsc_bounded() : tail(0), head(0) {
  for (size_t i = 0; i < Capacity; ++i) {
    buffer[i].sequence.store(i, std::memory_order_relaxed);
  }
}

template <typename T, size_t Capacity>
bool lockfree_mpsc_bounded<T, Capacity>::try_push(const T &value) {
  return try_push(T(value));
}

template <typename T, size_t Capacity>
bool lockfree_mpsc_bounded<T, Capacity>::try_push(T &&value) {
  cell_t *cell;
  size_t pos = tail.load(std::memory_order_relaxed);
  while (true) {
    cell = &buffer[pos & mask];
    size_t seq = cell->sequence.load(std::memory_order_acquire);
    intptr_t dif = (intptr_t)seq - (intptr_t)pos;

    if (dif == 0) {
      if (tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        break;
      }
    } else if (dif < 0) {
      // Queue is full
      return false;
    } else {
      // Another thread already pushed here, reload tail
      pos = tail.load(std::memory_order_relaxed);
    }
  }

  cell->data = std::move(value);
  cell->sequence.store(pos + 1, std::memory_order_release);
  return true;
}

template <typename T, size_t Capacity>
void lockfree_mpsc_bounded<T, Capacity>::wait_and_push(T value) {
  while (!try_push(std::move(value))) {
    // Spin
  }
}

template <typename T, size_t Capacity>
bool lockfree_mpsc_bounded<T, Capacity>::try_pop(T &value) {
  cell_t *cell;
  size_t pos = head.load(std::memory_order_relaxed);
  while (true) {
    cell = &buffer[pos & mask];
    size_t seq = cell->sequence.load(std::memory_order_acquire);
    intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

    if (dif == 0) {
      if (head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        break;
      }
    } else if (dif < 0) {
      // Queue is empty
      return false;
    } else {
      // Reload head
      pos = head.load(std::memory_order_relaxed);
    }
  }

  value = std::move(cell->data);
  // Important: Use pos + Capacity to reset the sequence for the next producer to use this slot.
  cell->sequence.store(pos + mask + 1, std::memory_order_release);
  return true;
}

template <typename T, size_t Capacity>
void lockfree_mpsc_bounded<T, Capacity>::wait_and_pop(T &value) {
  while (!try_pop(value)) {
    // Spin
  }
}

template <typename T, size_t Capacity>
bool lockfree_mpsc_bounded<T, Capacity>::empty() const {
  return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
}

template <typename T, size_t Capacity>
size_t lockfree_mpsc_bounded<T, Capacity>::size() const {
  size_t h = head.load(std::memory_order_relaxed);
  size_t t = tail.load(std::memory_order_relaxed);
  return (t > h) ? t - h : 0;
}

} // namespace tsfqueue::__impl

#endif
