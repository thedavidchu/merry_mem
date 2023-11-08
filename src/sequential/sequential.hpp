#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
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
#define LOG_LEVEL_TRACE   6
#define LOG_LEVEL_DEBUG   5
#define LOG_LEVEL_INFO    4
#define LOG_LEVEL_WARN    3
#define LOG_LEVEL_ERROR   2
#define LOG_LEVEL_FATAL   1
#define LOG_LEVEL_OFF     0

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_TRACE(x)  do { if (LOG_LEVEL >= LOG_LEVEL_TRACE)  { std::cout << "[TRACE]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)
#define LOG_DEBUG(x)  do { if (LOG_LEVEL >= LOG_LEVEL_DEBUG)  { std::cout << "[DEBUG]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)
#define LOG_INFO(x)   do { if (LOG_LEVEL >= LOG_LEVEL_INFO)   { std::cout << "[INFO]\t["  << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)
#define LOG_WARN(x)   do { if (LOG_LEVEL >= LOG_LEVEL_WARN)   { std::cout << "[WARN]\t["  << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)
#define LOG_ERROR(x)  do { if (LOG_LEVEL >= LOG_LEVEL_ERROR)  { std::cout << "[ERROR]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)
#define LOG_FATAL(x)  do { if (LOG_LEVEL >= LOG_LEVEL_FATAL)  { std::cout << "[FATAL]\t[" << __FILE__ << ":" << __LINE__ << "]\t[" << __FUNCTION__ << "]\t" << x << std::endl; } } while (0)


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

/// @brief  Bucket for the Robin Hood hash table.
struct SequentialBucket {
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

/// Source: https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
HashCodeType
hash(const KeyType key);

size_t
get_home(const HashCodeType hashcode, const size_t capacity);

/// @brief  Get real bucket index.
size_t
get_real_index(const size_t home, const size_t offset, const size_t capacity);

/// @brief  Get offset from home or where it would be if not found.
///         Return SIZE_MAX if no hole is found.
std::pair<SearchStatus, size_t>
get_wouldbe_offset(const std::vector<SequentialBucket> &buckets_buf,
                   const KeyType key,
                   const HashCodeType hashcode,
                   const size_t home);

/// @brief  Insert but assume no resize is necessary.
ErrorType
insert_without_resize(      std::vector<SequentialBucket> &tmp_buckets,
                      const KeyType key,
                      const ValueType value,
                      const HashCodeType hashcode);

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

class SequentialRobinHoodHashTable {
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

private:
  std::vector<SequentialBucket> buckets_ = {SequentialBucket()};
  size_t length_ = 0;
  size_t capacity_ = 1;

  ErrorType
  resize(size_t new_size);

};
