#ifndef LOCKFREE_MPSC_BOUNDED_DEFS
#define LOCKFREE_MPSC_BOUNDED_DEFS

#include "../utils.hpp"
#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace tsfqueue::__impl {

/**
 * @brief A lock-free Multi-Producer Single-Consumer (MPSC) bounded queue.
 *
 * This implementation uses a circular buffer with a fixed capacity.
 * For MPSC, we use an array of atomic sequence numbers to ensure
 * synchronization between multiple producers and the single consumer.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam Capacity The maximum number of elements in the queue (must be power of 2).
 */
template <typename T, size_t Capacity> class lockfree_mpsc_bounded {
  static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of 2");

private:
  struct cell_t {
    std::atomic<size_t> sequence;
    T data;
  };

  static constexpr size_t CLS = tsfq::__impl::cache_line_size;
  static constexpr size_t mask = Capacity - 1;

  // Producer-side data (Shared among multiple producers)
  alignas(CLS) std::atomic<size_t> tail;

  // Consumer-side data (Only accessed by one consumer)
  alignas(CLS) std::atomic<size_t> head;

  // The storage array of cells
  alignas(CLS) cell_t buffer[Capacity];

public:
  lockfree_mpsc_bounded();
  ~lockfree_mpsc_bounded() = default;

  // Disable copy/move
  lockfree_mpsc_bounded(const lockfree_mpsc_bounded &) = delete;
  lockfree_mpsc_bounded &operator=(const lockfree_mpsc_bounded &) = delete;

  /**
   * @brief Tries to push an element. Non-blocking (Wait-free).
   * Multiple producers can call this concurrently.
   */
  bool try_push(const T &value);
  bool try_push(T &&value);

  /**
   * @brief Pushes an element, spinning if the queue is full.
   */
  void wait_and_push(T value);

  /**
   * @brief Tries to pop an element. Non-blocking (Wait-free).
   * Only ONE consumer thread can call this.
   */
  bool try_pop(T &value);

  /**
   * @brief Pops an element, spinning if the queue is empty.
   */
  void wait_and_pop(T &value);

  bool empty() const;
  size_t size() const;
};

} // namespace tsfqueue::__impl

#endif
