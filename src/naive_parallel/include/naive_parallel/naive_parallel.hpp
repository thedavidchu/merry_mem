#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <utility>
#include <tuple>
#include <vector>

#include "common/logger.hpp"
#include "common/status.hpp"
#include "common/types.hpp"
#include "utility/utility.hpp"

////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

/// @brief  Bucket for the Robin Hood hash table.
struct NaiveParallelBucket {
  KeyType key = 0;
  ValueType value = 0;
  HashCodeType hashcode = 0;
  // A value of SIZE_MAX means that the bucket is empty. I do this hack so that
  // we can fit this bucket into 4 words, which is more amenable to the hardware.
  // A value of SIZE_MAX would be attrocious for performance anyways.
  size_t offset = SIZE_MAX;

  bool
  is_empty() const;

  void
  invalidate();

  bool
  equal_by_key(const KeyType key, const HashCodeType hashcode) const;

  /// @brief  Pretty print bucket
  ///
  /// This prints in the format (using Python's f-string syntax):
  /// f"({hashcode}=>{home}+{offset}) {key}: {value}"
  void
  print(const size_t capacity) const;
};

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

class NaiveParallelRobinHoodHashTable {
  std::vector<std::tuple<NaiveParallelBucket, std::mutex>> buckets_{1<<20};
  std::mutex meta_mutex_;
  size_t length_ = 0;
  size_t capacity_ = 1<<20;
public:
  void
  print();

  std::pair<SearchStatus, size_t>
  get_wouldbe_offset(
    const KeyType key,
    const HashCodeType hashcode,
    const size_t home,
    const std::vector<size_t> &locked_buckets
  );

  /// @brief Insert <key, value> pair.
  ///
  /// @return 0 on good; 1 on failure
  /// @exception throws any exception because I don't want to catch exceptions.
  ErrorType
  insert(KeyType key, ValueType value);

  /// @brief Search for <key, value>.
  ///
  /// @return 0 on found; 1 otherwise.
  std::optional<ValueType>
  search(KeyType key);

  /// @brief Remove <key, value> pair.
  /// N.B. 'delete' is a keyword, so I used 'remove'.
  ///
  /// @return 0 on found; 1 otherwise.
  ErrorType
  remove(KeyType key);

  std::vector<ValueType>
  getElements();

private:
  __attribute__((always_inline)) NaiveParallelBucket &
  get_bucket(const size_t index)
  {
    std::tuple<NaiveParallelBucket, std::mutex> &r = this->buckets_[index];
    return std::get<0>(r);
  }

  __attribute__((always_inline)) void
  lock_index(const size_t index)
  {
    std::tuple<NaiveParallelBucket, std::mutex> &r = this->buckets_[index];
    std::get<1>(r).lock();
  }

  __attribute__((always_inline)) void
  unlock_index(const size_t index)
  {
    std::tuple<NaiveParallelBucket, std::mutex> &r = this->buckets_[index];
    std::get<1>(r).unlock();
  }
};

