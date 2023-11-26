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

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITIONS (for easy refactor)
////////////////////////////////////////////////////////////////////////////////

/// N.B.  I use uint32_t for the key and value so that they fit into a 64-bit
///       word if I so choose to do that. This way, we can do atomic updates.
///       The extra metadata is a bit of an oof though.
/// N.B.  I do not use *_t because this is a reserved name in POSIX.
/// N.B.  I also named them something unique so it's easy to find/replace.
using KeyType = uint32_t;
using ValueType = uint32_t;
/// N.B.  I use 'HashCode' because I want to distinguish 'hash' (verb) and
///       'hash' (noun). Thus, I use 'hash code' to denote the latter.
using HashCodeType = size_t;

////////////////////////////////////////////////////////////////////////////////
/// LOGGING MACROS (not thread safe)
////////////////////////////////////////////////////////////////////////////////

/// Create my own sketchy logger
#define LOG_LEVEL_TRACE 6
#define LOG_LEVEL_DEBUG 5
#define LOG_LEVEL_INFO  4
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_FATAL 1
#define LOG_LEVEL_OFF   0

#define LOG_LEVEL       LOG_LEVEL_DEBUG

#define LOG_TRACE(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_TRACE) {                                                        \
            std::cout << "[TRACE]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_DEBUG(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {                                                        \
            std::cout << "[DEBUG]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_INFO(x)                                                                                \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_INFO) {                                                         \
            std::cout << "[INFO]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__      \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_WARN(x)                                                                                \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_WARN) {                                                         \
            std::cout << "[WARN]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__      \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_ERROR(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_ERROR) {                                                        \
            std::cout << "[ERROR]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)
#define LOG_FATAL(x)                                                                               \
    do {                                                                                           \
        if (LOG_LEVEL >= LOG_LEVEL_FATAL) {                                                        \
            std::cout << "[FATAL]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__     \
                      << "]\t" << x << std::endl;                                                  \
        }                                                                                          \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

enum class SearchStatus {
    found_match,
    found_swap,
    found_hole,
    found_nohole,
};

enum class InsertStatus {
    // Successful insertion
    inserted_at_home,
    updated_at_home,
    inserted,
    updated,
    // Unsuccessful insertion
    not_inserted,
};

enum class ErrorType {
    // We want !error to imply an ok status
    ok = 0,
    e_unknown,
    e_oom,
    e_notfound,
    e_nohole,
};

/// We want the AtomicCounter to allow for overflow. This means that we should
/// not use ordering operators to allow for overflow. This also makes the
/// assumption that the AtomicCounter will not be incremented until it overflows
/// and reaches the formerly held value again.
using AtomicCounter = std::atomic<unsigned>;

class SegmentLock {
public:
    size_t
    get_counter();

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
    // N.B. Using counter_ = 0 causes the linter to complain that the copying
    //      invokes a deleted constructor
    // N.B. Using counter_(0) causes the linter to complain, thinking counter_
    //      is a function.
    AtomicCounter counter_{0};
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
};

/// This ensures we are guaranteed to use atomic operations (instead of locks).
/// This for performance (to follow the algorithm's 'fast-path') rather than for
/// correctness.
static_assert(std::atomic<KeyValue>::is_always_lock_free)

struct ParallelBucket {
    std::atomic<KeyValue> key_value;
    // A value of SIZE_MAX means that the bucket is empty. I do this hack so that
    // we can fit this bucket into 4 words, which is more amenable to the hardware.
    // A value of SIZE_MAX would be attrocious for performance anyways.
    size_t offset = SIZE_MAX;
};

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
    std::vector<std::pair<size_t, size_t>> segment_lock_index_and_count_;

    size_t
    get_segment_index(size_t index);
};

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

    ParallelBucket
    do_atomic_swap(ParallelBucket &swap_entry, size_t index);

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
