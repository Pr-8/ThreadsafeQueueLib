#include "include/lockfree_spsc_bounded/queue.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

void run_benchmark() {
    const size_t num_operations = 10000000; // 10 Million items
    const size_t capacity = 65536; // Must be power of 2
    
    tsfqueue::__impl::lockfree_spsc_bounded<int, capacity> queue;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&]() {
        for (int i = 0; i < num_operations; ++i) {
            queue.wait_and_push(i);
        }
    });

    std::thread consumer([&]() {
        int val;
        for (int i = 0; i < num_operations; ++i) {
            queue.wait_and_pop(val);
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "--- SPSC Bounded Queue Benchmark ---" << std::endl;
    std::cout << "Operations: " << num_operations << std::endl;
    std::cout << "Total Time: " << diff.count() << " seconds" << std::endl;
    std::cout << "Throughput: " << (num_operations / diff.count()) / 1e6 << " million ops/sec" << std::endl;
    std::cout << "Latency per op: " << (diff.count() / num_operations) * 1e9 << " nanoseconds" << std::endl;
}

int main() {
    run_benchmark();
    return 0;
}
