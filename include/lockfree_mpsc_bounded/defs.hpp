#ifndef LOCKFREE_MPSC_BOUNDED_DEFS
#define LOCKFREE_MPSC_BOUNDED_DEFS

#include "../utils.hpp"
#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace tsfqueue::__impl {
  
template <typename T, size_t Capacity> class lockfree_mpsc_bounded {
  static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of 2");

private:
  struct cell_t {
    std::atomic<size_t> sequence;
    T data;
  };

  static constexpr size_t mask = Capacity - 1;

  alignas(std::hardware_destructive_interference_size) std::atomic<size_t> tail;

  alignas(std::hardware_destructive_interference_size) std::atomic<size_t> head;

  // The storage array of cells
  alignas(std::hardware_destructive_interference_size) cell_t buffer[Capacity];

public:
  lockfree_mpsc_bounded();
  ~lockfree_mpsc_bounded() = default;

  // Disable copy/move
  lockfree_mpsc_bounded(const lockfree_mpsc_bounded &) = delete;
  lockfree_mpsc_bounded &operator=(const lockfree_mpsc_bounded &) = delete;

  bool try_push(const T &value);
  bool try_push(T &&value);
  void wait_and_push(T value);
  bool try_pop(T &value);
  void wait_and_pop(T &value);

  bool empty() const;
  size_t size() const;
};

} // namespace tsfqueue::__impl

#endif
