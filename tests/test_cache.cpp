#include "concurrent_lru_cache.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

void test_lru_eviction() {
    ConcurrentLruCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");
    assert(cache.get(1) == "one");

    cache.put(3, "three");
    assert(!cache.get(2).has_value());
    assert(cache.get(1) == "one");
    assert(cache.get(3) == "three");
    assert(cache.stats().evictions == 1);
}

void test_ttl_expiration() {
    ConcurrentLruCache<std::string, int> cache(4);
    cache.put("short-lived", 42, 20ms);
    std::this_thread::sleep_for(30ms);

    assert(!cache.get("short-lived").has_value());
    assert(cache.size() == 0);
    assert(cache.stats().expirations == 1);
}

void test_update_and_erase() {
    ConcurrentLruCache<int, int> cache(2);
    cache.put(7, 1);
    cache.put(7, 2);
    assert(cache.size() == 1);
    assert(cache.get(7) == 2);
    assert(cache.erase(7));
    assert(!cache.erase(7));
}

void test_concurrent_access() {
    ConcurrentLruCache<int, int> cache(128);
    std::vector<std::thread> threads;

    for (int worker = 0; worker < 8; ++worker) {
        threads.emplace_back([worker, &cache] {
            for (int index = 0; index < 1000; ++index) {
                const int key = worker * 1000 + index;
                cache.put(key, index);
                assert(cache.get(key).has_value());
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    assert(cache.size() <= cache.capacity());
    assert(cache.stats().hits == 8000);
}

int main() {
    test_lru_eviction();
    test_ttl_expiration();
    test_update_and_erase();
    test_concurrent_access();
    std::cout << "All concurrent LRU cache tests passed.\n";
}

