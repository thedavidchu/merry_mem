#include <algorithm>  // std::swap

#include "sequential/sequential.hpp"


////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////
void
NaiveParallelBucket::lock() {
  LOG_TRACE("Enter");
  this->mutex.lock();
}

void
NaiveParallelBucket::unlock() {
  LOG_TRACE("Enter");
  this->mutex.unlock();
}

bool
NaiveParallelBucket::is_empty() const {
  LOG_TRACE("Enter");
  return this->offset == SIZE_MAX;
}

void
NaiveParallelBucket::invalidate() {
  LOG_TRACE("Enter");
  // Not necessary
  this->key = 0;
  this->value = 0;
  this->hashcode = 0;
  // Necessary
  this->offset = SIZE_MAX;
}

bool
NaiveParallelBucket::equal_by_key(const KeyType key, const HashCodeType hashcode) const {
  LOG_TRACE("Enter");
  assert(!this->is_empty() && "should not compare to empty bucket!");
  // Assume equality between hashcodes is simpler (because it is fixed for any
  // type of key)
  return this->hashcode == hashcode && this->key == key;
}

void
NaiveParallelBucket::print(const size_t capacity) const {
  if (this->is_empty()) {
    std::cout << "(empty)";
  } else {
    std::cout << "(" << this->hashcode << "=>" << this->hashcode % capacity <<
        "+" << this->offset << ") " << this->key << ": " << this->value;
  }
}


////////////////////////////////////////////////////////////////////////////////
/// STATIC HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


/// @brief  Get real bucket index.
static size_t
get_real_index(const size_t home, const size_t offset, const size_t capacity) {
  LOG_TRACE("Enter");
  return (home + offset) % capacity;
}

static std::pair<SearchStatus, size_t>
get_wouldbe_offset(const std::vector<NaiveParallelBucket> &buckets_buf,
                   const KeyType key,
                   const HashCodeType hashcode,
                   const size_t home) {
  LOG_TRACE("Enter");
  size_t capacity = buckets_buf.size();
  for (size_t i = 0; i < capacity; ++i) {
    size_t real_index = get_real_index(home, i, capacity);
    const NaiveParallelBucket &bkt = buckets_buf[real_index];
    bkt.lock();
    // If not found
    if (bkt.is_empty()) {
      // This is first, because equality on an empty bucket is not well defined.
      return {SearchStatus::found_hole, i};
    } else if (bkt.offset < i) { // This means that bkt belongs to a nearer home
      return {SearchStatus::found_swap, i};
    // If found
    } else if (bkt.equal_by_key(key, hashcode)) {
      return {SearchStatus::found_match, i};
    }
    bkt.unlock();
  }
  return {SearchStatus::found_nohole, SIZE_MAX};
}

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

void
NaiveParallelRobinHoodHashTable::print() {
  LOG_TRACE("Enter");
  std::cout << "(Length: " << this->length_ << "/Capacity: " << this->capacity_ << ") [\n";
  for (size_t i = 0; i < this->capacity_; ++i) {
    std::cout << "\t" << i << ": ";
    const NaiveParallelBucket &bkt = this->buckets_[i];
    bkt.lock();
    bkt.print(this->capacity_);
    std::cout << ",\n";
    bkt.unlock();
  }
  std::cout << "]" << std::endl;
}

ErrorType
NaiveParallelRobinHoodHashTable::insert(KeyType key, ValueType value) {
  LOG_TRACE("Enter");
  // TODO: ensure key and value are valid
  // 1. Error check arguments
  // 2. Check if already present
  // 3.   If not, check if room to insert
  // 4.     If not, resize
  // 5. Insert (with swapping if necessary)
  HashCodeType hashcode = hash(key);
  NaiveParallelBucket tmp = {.key = key,
                          .value = value,
                          .hashcode = hashcode,
                          .offset = /*arbitrary value*/0,};
  const size_t capacity = this->buckets_.size();
  // This could also be upper-bounded by the number of valid elements (num_elem)
  // in this->buckets_. This is because you need to bump at most num_elem elements
  // (if they are all sitting in a row) to insert something.
  while (true) {
    size_t home = get_home(tmp.hashcode, capacity);
    const auto [status, offset] = get_wouldbe_offset(this->buckets_, tmp.key, tmp.hashcode, home);
    switch (status) {
      case SearchStatus::found_match: {
        LOG_DEBUG("SearchStatus::found_match");
        size_t real_index = get_real_index(home, offset, capacity);
        NaiveParallelBucket &bkt = this->buckets_[real_index];
        bkt.value = tmp.value;
        bkt.unlock();
        return ErrorType::ok;
      }
      case SearchStatus::found_swap: {
        LOG_DEBUG("SearchStatus::found_swap");
        // NOTE(dchu): could be buggy
        size_t real_index = get_real_index(home, offset, capacity);
        NaiveParallelBucket &bkt = this->buckets_[real_index];
        tmp.offset = offset;
        std::swap(bkt, tmp);
        // bkt is unlocked by the swap
        continue;
      }
      case SearchStatus::found_hole: {
        LOG_DEBUG("SearchStatus::found_hole");
        // NOTE(dchu): could be buggy
        size_t real_index = get_real_index(home, offset, capacity);
        NaiveParallelBucket &bkt = this->buckets_[real_index];
        tmp.offset = offset;
        std::swap(bkt, tmp);
        // bkt is unlocked by the swap
        ++this->length_;
        return ErrorType::ok;
      }
      case SearchStatus::found_nohole:
        assert(0 && "should not call this function if we need to resize!");
      default:
        assert(0 && "impossible!");
    }
  }
  assert(0 && "impossible!");
}

std::optional<ValueType>
NaiveParallelRobinHoodHashTable::search(KeyType key) {
  LOG_TRACE("Enter");
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->capacity_);

  const auto [status, offset] = get_wouldbe_offset(this->buckets_, key, hashcode, home);
  switch (status) {
    case SearchStatus::found_match: {
      size_t real_index = get_real_index(home, offset, this->capacity_);
      const NaiveParallelBucket &bkt = this->buckets_[real_index];
      ValueType v = bkt.value;
      bkt.unlock();
      return v;
    }
    case SearchStatus::found_hole:
    case SearchStatus::found_nohole:
    case SearchStatus::found_swap:
      return std::nullopt;
    default:
      assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}

ErrorType
NaiveParallelRobinHoodHashTable::remove(KeyType key) {
  LOG_TRACE("Enter");
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->capacity_);

  const auto [status, offset] = get_wouldbe_offset(this->buckets_, key, hashcode, home);
  switch (status) {
    case SearchStatus::found_match: {
      for (size_t i = 0; i < this->capacity_; ++i) {
        // NOTE(dchu): real_index is the previous iteration's next_real_index
        size_t real_index = get_real_index(home, offset + i, this->capacity_);
        NaiveParallelBucket &bkt = this->buckets_[real_index];
        size_t next_real_index = get_real_index(home, offset + i + 1, this->capacity_);
        NaiveParallelBucket &next_bkt = this->buckets_[next_real_index];
        next_bkt.lock();
        // Next element is empty or already in its home bucket
        if (next_bkt.is_empty() || next_bkt.offset == 0) {
          bkt.invalidate();
          bkt.unlock();
          next_bkt.unlock();
          --this->length_;
          return ErrorType::ok;
        }
        // I argue that this sliding is efficient if the average home has only a
        // single element belonging to it. In this case, it would not have any
        // elements belonging to the same home, over which it may leap-frog.
        bkt = std::move(next_bkt);
        --bkt.offset;
        // Now, next_bkt is the old locked bucket
        next_bkt.unlock();
      }
      assert(0 && "impossible! Should have a hole");
    }
    // Not found
    case SearchStatus::found_hole:
    case SearchStatus::found_swap:
      size_t real_index = get_real_index(home, offset, this->capacity_);
      NaiveParallelBucket &bkt = this->buckets_[real_index];
      bkt.unlock();
      return ErrorType::e_notfound;
    case SearchStatus::found_nohole:
      return ErrorType::e_notfound;
    default:
      assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}
