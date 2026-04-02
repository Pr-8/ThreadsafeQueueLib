#ifndef TSF_UTILS_HPP
#define TSF_UTILS_HPP

#include <memory>
#include <new>
#include <atomic>

namespace tsfqueue::__utils {
template <typename T> struct Node {
  std::shared_ptr<T> data;
  std::unique_ptr<Node<T>> next;
};
template <typename T> struct Lockless_Node {
  T data;
  std::atomic<Lockless_Node *> next;
}; // Added semicolon here
} // namespace tsfqueue::__utils

namespace tsfq::__impl {

// Fallback for cache line size if hardware_destructive_interference_size is not available
#if defined(__cpp_lib_hardware_interference_size)
    static constexpr size_t cache_line_size = std::hardware_destructive_interference_size;
#else
    static constexpr size_t cache_line_size = 64; // Common value for most modern CPUs
#endif

} // namespace tsfq::__impl

#endif
