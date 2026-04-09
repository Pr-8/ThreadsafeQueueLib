#ifndef LOCKFREE_SPSC_BOUNDED_IMPL_CT
#define LOCKFREE_SPSC_BOUNDED_IMPL_CT

#include "defs.hpp"
#include <utility>

template <typename T, size_t Capacity>
using queue = tsfqueue::__impl::lockfree_spsc_bounded<T, Capacity>;

template <typename T, size_t Capacity>
void queue<T, Capacity>::wait_and_push(T value) {
  // Busy wait until try_push succeeds.
  // We use std::move to avoid unnecessary copies.
  while (!try_push(std::move(value))) {
    // Optionally: add a CPU hint like __builtin_ia32_pause() or std::this_thread::yield()
  }
}

template <typename T, size_t Capacity>
bool queue<T, Capacity>::try_push(T value) {
  // Load tail (relaxed is fine because only the producer thread modifies it)
  size_t const t = tail.load(std::memory_order_relaxed);

  // Check if queue is full using the cached head pointer first.
  if (t - head_cache >= Capacity) {
    // Refresh head_cache from the atomic head with acquire semantics
    head_cache = head.load(std::memory_order_acquire);
    if (t - head_cache >= Capacity) {
      return false;
    }
  }

  // Use the mask trick for fast indexing
  arr[t & mask] = std::move(value);

  // Release the new tail so the consumer can see the data
  tail.store(t + 1, std::memory_order_release);
  return true;
}

template <typename T, size_t Capacity>
bool queue<T, Capacity>::try_pop(T &value) {
  // Load head (relaxed is fine because only the consumer thread modifies it)
  size_t const h = head.load(std::memory_order_relaxed);

  // Check if queue is empty using the cached tail pointer first.
  if (h == tail_cache) {
    // Refresh tail_cache from the atomic tail with acquire semantics
    tail_cache = tail.load(std::memory_order_acquire);
    if (h == tail_cache) {
      return false;
    }
  }

  value = std::move(arr[h & mask]);

  // Release the new head so the producer knows a slot is free
  head.store(h + 1, std::memory_order_release);
  return true;
}

template <typename T, size_t Capacity>
void queue<T, Capacity>::wait_and_pop(T &value) {
  while (!try_pop(value)) {
    // Busy wait
  }
}

template <typename T, size_t Capacity>
bool queue<T, Capacity>::peek(T &value) const {
  size_t const h = head.load(std::memory_order_relaxed);
  // Peek needs to check the current tail with acquire semantics
  if (h == tail.load(std::memory_order_acquire)) {
    return false;
  }
  value = arr[h & mask];
  return true;
}

template <typename T, size_t Capacity>
bool queue<T, Capacity>::empty() const {
  // Simple check for equality. 
  // Note: This is an "approximate" check if called from a third thread.
  return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
}

template <typename T, size_t Capacity>
size_t queue<T, Capacity>::size() const {
  // Standard size calculation for SPSC using ever-increasing indices.
  // head and tail are loaded with relaxed ordering.
  size_t h = head.load(std::memory_order_relaxed);
  size_t t = tail.load(std::memory_order_relaxed);
  // Unsigned subtraction handles the overflow/wrap-around correctly.
  return t - h;
}

template <typename T, size_t Capacity>
template <typename... Args>
bool queue<T, Capacity>::emplace_back(Args &&...args) {
  size_t const t = tail.load(std::memory_order_relaxed);
  if (t - head_cache >= Capacity) {
    head_cache = head.load(std::memory_order_acquire);
    if (t - head_cache >= Capacity) {
      return false;
    }
  }

  // Construct T in-place. Note: Since arr[Capacity] is already allocated, 
  // this performs assignment. For true in-place construction, 
  // we would need aligned_storage/placement new.
  arr[t & mask] = T(std::forward<Args>(args)...);

  tail.store(t + 1, std::memory_order_release);
  return true;
}

#endif

// 1. Add static asserts
// 2. Add emplace_back using perfect forwarding and variadic templates (you
// can use this in push then)
// 3. Add size() function
// 4. Any more suggestions ??
