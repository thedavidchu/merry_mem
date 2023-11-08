#pragma once

#include <cstdint>
#include <iostream>
#include <tuple>
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

enum class ErrorType {
    // We want !error to imply an ok status
    ok = 0,
    e_unknown,
    e_oom,
    e_notfound,
    e_nohole,
};

////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

struct ParallelBucket {
    KeyType key = 0;
    ValueType value = 0;
    HashCodeType hashcode = 0;
    // A value of SIZE_MAX means that the bucket is empty. I do this hack so that
    // we can fit this bucket into 4 words, which is more amenable to the hardware.
    // A value of SIZE_MAX would be attrocious for performance anyways.
    size_t offset = SIZE_MAX;
};

class ThreadManager {
private:
    class ParallelRobinHoodHashTable *hash_table_;
    std::vector<size_t> locked_indices_;

public:
    void
    lock(size_t next_index);

    void
    speculate_index(size_t index);

    bool
    finish_speculate();

    void
    release_all_locks();
};

class ParallelRobinHoodHashTable {
public:
    bool
    insert(KeyType key, ValueType value);

    void
    insert_or_update(KeyType key, ValueType value);

    bool
    remove(KeyType key, ValueType value);

    std::pair<ValueType, bool>
    find(KeyType key);

private:
    std::vector<ParallelBucket> buckets_;
    size_t length_;
    size_t capacity_;
    size_t capacity_with_buffer_;

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

    std::pair<KeyType, ValueType>
    compare_and_set_key_val(size_t index, KeyType prev_key, KeyType new_key, ValueType new_val);

    ParallelBucket &
    do_atomic_swap(ParallelBucket &swap_entry, size_t index);

    ////////////////////////////////////////////////////////////////////////////
    /// HELPER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////
    std::pair<bool, bool>
    distance_zero_insert(KeyType key, ValueType value, size_t dist_zero_slot);

    bool
    locked_insert(ParallelBucket &entry_to_insert, size_t swap_index);

    std::tuple<ValueType, bool, bool>
    find_speculate(KeyType key, size_t start_index);
};
