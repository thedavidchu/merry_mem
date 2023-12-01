#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
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
/// STATIC HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief  Get real bucket index.
size_t
get_real_index(const size_t home, const size_t offset, const size_t capacity);

/// @brief  Get offset from home or where it would be if not found.
///         Return SIZE_MAX if no hole is found.
std::pair<SearchStatus, size_t>
get_wouldbe_offset(const std::vector<NaiveParallelBucket> &buckets_buf,
                   const KeyType key,
                   const HashCodeType hashcode,
                   const size_t home);

/// @brief  Insert but assume no resize is necessary.
ErrorType
insert_without_resize(      std::vector<NaiveParallelBucket> &tmp_buckets,
                      const KeyType key,
                      const ValueType value,
                      const HashCodeType hashcode);

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

class NaiveParallelRobinHoodHashTable {
public:
  void
  print() const;

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
  search(KeyType key) const;

  /// @brief Remove <key, value> pair.
  /// N.B. 'delete' is a keyword, so I used 'remove'.
  ///
  /// @return 0 on found; 1 otherwise.
  ErrorType
  remove(KeyType key);

  std::vector<ValueType> 
  getElements() const;



private:
  std::vector<NaiveParallelBucket> buckets_{1<<20};
  size_t length_ = 0;
  size_t capacity_ = 1<<20;

  ErrorType
  resize(size_t new_size);

};
