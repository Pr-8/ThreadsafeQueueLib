#include "include/lockfree_mpsc_bounded/queue.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

void run_mpsc_benchmark() {
    const size_t num_producers = 4;
    const size_t ops_per_producer = 2500000; // 2.5 Million each = 10 Million total
    const size_t capacity = 65536;
    
    tsfqueue::__impl::lockfree_mpsc_bounded<int, capacity> queue;
    std::atomic<size_t> total_received{0};

    auto start = std::chrono::high_resolution_clock::now();

    // Start Producers
    std::vector<std::thread> producers;
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < ops_per_producer; ++j) {
                queue.wait_and_push(j);
            }
        });
    }

    // Start Consumer
    std::thread consumer([&]() {
        int val;
        for (size_t i = 0; i < num_producers * ops_per_producer; ++i) {
            queue.wait_and_pop(val);
            total_received++;
        }
    });

    for (auto& p : producers) p.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "--- MPSC Bounded Queue Benchmark ---" << std::endl;
    std::cout << "Producers: " << num_producers << " | Consumer: 1" << std::endl;
    std::cout << "Total Operations: " << total_received.load() << std::endl;
    std::cout << "Total Time: " << diff.count() << " seconds" << std::endl;
    std::cout << "Throughput: " << (total_received / diff.count()) / 1e6 << " million ops/sec" << std::endl;
}

int main() {
    run_mpsc_benchmark();
    return 0;
}
