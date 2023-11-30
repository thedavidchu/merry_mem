#include "utilities/utilities.hpp"

HashCodeType
hash(const KeyType key) {
  LOG_TRACE("Enter");
  size_t k = static_cast<size_t>(key);
  // I use the suffix '*ULL' to denote that the literal is at least an int64.
  // I've had weird bugs in the past to do with literal conversion. I'm not sure
  // the details. I only remember it was a huge pain.
  k = ((k >> 30) ^ k) * 0xbf58476d1ce4e5b9ULL;
  k = ((k >> 27) ^ k) * 0x94d049bb133111ebULL;
  k =  (k >> 31) ^ k;
  // We could theoretically use `reinterpret_cast` to ensure there is no
  // overhead in this cast because HashCodeType is typedef'ed to size_t.
  return k;
}


size_t
get_home(const HashCodeType hashcode, const size_t capacity) {
  LOG_TRACE("Enter");
  size_t h = static_cast<size_t>(hashcode);
  return h % capacity;
}