#pragma once

#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

enum class InsertStatus {
    // Successful insertion
    inserted_at_home,
    updated_at_home,
    inserted,
    updated,
    // Unsuccessful insertion
    not_inserted,
};

class SegmentLock {
public:
    /// We want the AtomicCounter to allow for overflow. This means that we should
    /// not use ordering operators to allow for overflow. This also makes the
    /// assumption that the AtomicCounter will not be incremented until it overflows
    /// and reaches the formerly held value again.
    using Version = size_t;
    using AtomicVersion = std::atomic<Version>;

    Version
    get_version();

    bool
    is_locked();

    void
    lock();

    void
    unlock();

private:
    // Use a recursive mutex, which allows multiple lock operations by the same
    // thread on the lock. However, if the number of locks exceeds an
    // unspecified limit, then it will throw a std::system_error.
    std::recursive_mutex mutex_;
    // N.B. Using version_ = 0 causes the linter to complain that the copying
    //      invokes a deleted constructor
    // N.B. Using version_(0) will cause the compile to parse version_ as a
    //      function.
    // N.B. This needs to be atomic since it can be read from multiple threads
    //      via get_version() while its being modified by one thread.
    AtomicVersion version_{0};
    unsigned locked_count_{0};
};

////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

/// We are assuming this will be packed into 8 (or 16... if we've changed the
/// size of the KeyType/ValueType to int64_t) bytes so that we can atomically
/// update both.
struct KeyValue {
    KeyType key = 0;
    ValueType value = 0;

    constexpr bool operator==(const KeyValue &) const = default;
};

using ParallelBucket = std::atomic<KeyValue>;

/// This ensures we are guaranteed to use atomic operations (instead of locks).
/// This for performance (to follow the algorithm's 'fast-path') rather than for
/// correctness.
static_assert(ParallelBucket::is_always_lock_free);

class ThreadManager {
public:
    ThreadManager(class ParallelRobinHoodHashTable *const hash_table);

    /// Lock the segment corresponding to the index.
    /// TODO(dchu): refactor to 'lock_segment'
    void
    lock(size_t index);

    /// Release all segment locks.
    /// TODO(dchu): refactor to 'unlock_all_segments'
    void
    release_all_locks();

    /// @brief Capture version numbers of an index
    /// TODO(dchu): refactor to 'speculate'
    bool
    speculate_index(size_t index);

    bool
    finish_speculate();

private:
    class ParallelRobinHoodHashTable *const hash_table_;
    std::vector<size_t> locked_segments_;
    std::vector<std::pair<size_t, unsigned>> segment_lock_index_and_version_;
};

size_t
get_segment_index(size_t index);

class ParallelRobinHoodHashTable {
public:
    friend class ThreadManager;

    bool
    insert(KeyType key, ValueType value);

    void
    insert_or_update(KeyType key, ValueType value);

    bool
    remove(KeyType key, ValueType value);

    std::pair<ValueType, bool>
    find(KeyType key);

    /// Call this when adding a new thread to work on the hash table.
    void
    add_thread_lock_manager();

    /// Call this when removing a worker thread that worked on the hash table.
    void
    remove_thread_lock_manager();

private:
    std::vector<ParallelBucket> buckets_ = std::vector<ParallelBucket>(1024 + 10);
    std::vector<SegmentLock> segment_locks_;
    size_t length_ = 0;
    size_t capacity_ = 1024;
    size_t capacity_with_buffer_ = 1024 + 10;
    // N.B. this data structure within the hash table itself is necessary since
    //      we want a thread manager both (a) per thread and (b) per hash table.
    std::unordered_map<std::thread::id, ThreadManager> thread_managers_;

    ////////////////////////////////////////////////////////////////////////////
    /// HELPER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////
    std::pair<size_t, bool>
    find_next_index_lock(ThreadManager &manager,
                         size_t start_index,
                         KeyType key,
                         size_t &distance_key);

    ThreadManager &
    get_thread_lock_manager();

    bool
    compare_and_set_key_val(size_t index, KeyValue prev_kv, KeyValue new_kv);

    KeyValue
    do_atomic_swap(ParallelBucket &swap_entry, size_t index);

    KeyValue
    atomic_load_key_val(size_t index);

    ////////////////////////////////////////////////////////////////////////////
    /// HELPER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief  Try to insert the key and value into the home bucket.
    ///
    /// @details    This assumes that the default-constructed key value is invalid.
    ///             This also assumes that the offset is by default 0 and the hashcode
    ///             can no longer be part of the data structure, because it cannot
    ///             be updated along side the key-value pair atomically.
    InsertStatus
    distance_zero_insert(KeyType key, ValueType value, size_t dist_zero_slot);

    bool
    locked_insert(ParallelBucket &entry_to_insert, size_t swap_index);

    std::tuple<ValueType, bool, bool>
    find_speculate(KeyType key, size_t start_index);
};
