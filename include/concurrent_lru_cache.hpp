#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

template <typename Key, typename Value, typename Clock = std::chrono::steady_clock>
class ConcurrentLruCache {
public:
    using Duration = typename Clock::duration;
    using TimePoint = typename Clock::time_point;

    struct Stats {
        std::uint64_t hits{};
        std::uint64_t misses{};
        std::uint64_t evictions{};
        std::uint64_t expirations{};
    };

    explicit ConcurrentLruCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("cache capacity must be greater than zero");
        }
    }

    void put(const Key& key, Value value,
             std::optional<Duration> ttl = std::nullopt) {
        std::scoped_lock lock(mutex_);
        const auto existing = index_.find(key);
        if (existing != index_.end()) {
            entries_.erase(existing->second);
            index_.erase(existing);
        }

        Entry entry{key, std::move(value), TimePoint{}, false};
        if (ttl.has_value()) {
            entry.expires_at = Clock::now() + *ttl;
            entry.has_expiry = true;
        }

        entries_.push_front(std::move(entry));
        index_[entries_.front().key] = entries_.begin();
        evict_if_needed();
    }

    std::optional<Value> get(const Key& key) {
        std::scoped_lock lock(mutex_);
        const auto found = index_.find(key);
        if (found == index_.end()) {
            ++stats_.misses;
            return std::nullopt;
        }

        auto iterator = found->second;
        if (is_expired(*iterator)) {
            erase(iterator);
            ++stats_.misses;
            ++stats_.expirations;
            return std::nullopt;
        }

        entries_.splice(entries_.begin(), entries_, iterator);
        ++stats_.hits;
        return entries_.front().value;
    }

    bool erase(const Key& key) {
        std::scoped_lock lock(mutex_);
        const auto found = index_.find(key);
        if (found == index_.end()) {
            return false;
        }
        erase(found->second);
        return true;
    }

    std::size_t purge_expired() {
        std::scoped_lock lock(mutex_);
        std::size_t removed = 0;
        for (auto iterator = entries_.begin(); iterator != entries_.end();) {
            if (is_expired(*iterator)) {
                iterator = erase(iterator);
                ++removed;
                ++stats_.expirations;
            } else {
                ++iterator;
            }
        }
        return removed;
    }

    [[nodiscard]] std::size_t size() const {
        std::scoped_lock lock(mutex_);
        return entries_.size();
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

    [[nodiscard]] Stats stats() const {
        std::scoped_lock lock(mutex_);
        return stats_;
    }

private:
    struct Entry {
        Key key;
        Value value;
        TimePoint expires_at;
        bool has_expiry;
    };

    using List = std::list<Entry>;
    using Iterator = typename List::iterator;

    bool is_expired(const Entry& entry) const {
        return entry.has_expiry && Clock::now() >= entry.expires_at;
    }

    Iterator erase(Iterator iterator) {
        index_.erase(iterator->key);
        return entries_.erase(iterator);
    }

    void evict_if_needed() {
        while (entries_.size() > capacity_) {
            auto oldest = std::prev(entries_.end());
            erase(oldest);
            ++stats_.evictions;
        }
    }

    const std::size_t capacity_;
    mutable std::mutex mutex_;
    List entries_;
    std::unordered_map<Key, Iterator> index_;
    Stats stats_;
};

