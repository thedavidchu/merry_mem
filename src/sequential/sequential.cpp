#include <algorithm>  // std::swap

#include "sequential.hpp"


////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

bool
SequentialBucket::is_empty() const {
  LOG_TRACE("Enter");
  return this->offset == SIZE_MAX;
}

void
SequentialBucket::invalidate() {
  LOG_TRACE("Enter");
  // Not necessary
  this->key = 0;
  this->value = 0;
  this->hashcode = 0;
  // Necessary
  this->offset = SIZE_MAX;
}

bool
SequentialBucket::equal_by_key(const KeyType key, const HashCodeType hashcode) const {
  LOG_TRACE("Enter");
  assert(!this->is_empty() && "should not compare to empty bucket!");
  // Assume equality between hashcodes is simpler (because it is fixed for any
  // type of key)
  return this->hashcode == hashcode && this->key == key;
}

void
SequentialBucket::print(const size_t capacity) const {
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
  return static_cast<HashCodeType>(k);
}

size_t
get_home(const HashCodeType hashcode, const size_t capacity) {
  LOG_TRACE("Enter");
  size_t h = static_cast<size_t>(hashcode);
  return h % capacity;
}

/// @brief  Get real bucket index.
size_t
get_real_index(const size_t home, const size_t offset, const size_t capacity) {
  LOG_TRACE("Enter");
  return (home + offset) % capacity;
}

std::pair<SearchStatus, size_t>
get_wouldbe_offset(const std::vector<SequentialBucket> &buckets_buf,
                   const KeyType key,
                   const HashCodeType hashcode,
                   const size_t home) {
  LOG_TRACE("Enter");
  size_t i = 0;
  size_t capacity = buckets_buf.size();
  for (i = 0; i < capacity; ++i) {
    size_t real_index = get_real_index(home, i, capacity);
    const SequentialBucket &bkt = buckets_buf[real_index];
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
  }
  return {SearchStatus::found_nohole, SIZE_MAX};
}

ErrorType
insert_without_resize(      std::vector<SequentialBucket> &tmp_buckets,
                      const KeyType key,
                      const ValueType value,
                      const HashCodeType hashcode) {
  LOG_TRACE("Enter");
  SequentialBucket tmp = {.key = key,
                          .value = value,
                          .hashcode = hashcode,
                          .offset = /*arbitrary value*/0,};
  const size_t capacity = tmp_buckets.size();
  // This could also be upper-bounded by the number of valid elements (num_elem)
  // in tmp_buckets. This is because you need to bump at most num_elem elements
  // (if they are all sitting in a row) to insert something.
  while (true) {
    size_t home = get_home(tmp.hashcode, capacity);
    const auto [status, offset] = get_wouldbe_offset(tmp_buckets, tmp.key, tmp.hashcode, home);
    switch (status) {
      case SearchStatus::found_match: {
        LOG_DEBUG("SearchStatus::found_match");
        size_t real_index = get_real_index(home, offset, capacity);
        SequentialBucket &bkt = tmp_buckets[real_index];
        bkt.value = tmp.value;
        return ErrorType::ok;
      }
      case SearchStatus::found_swap: {
        LOG_DEBUG("SearchStatus::found_swap");
        // NOTE(dchu): could be buggy
        size_t real_index = get_real_index(home, offset, capacity);
        SequentialBucket &bkt = tmp_buckets[real_index];
        tmp.offset = offset;
        std::swap(bkt, tmp);
        continue;
      }
      case SearchStatus::found_hole: {
        LOG_DEBUG("SearchStatus::found_hole");
        // NOTE(dchu): could be buggy
        size_t real_index = get_real_index(home, offset, capacity);
        SequentialBucket &bkt = tmp_buckets[real_index];
        tmp.offset = offset;
        std::swap(bkt, tmp);
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


////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

void
SequentialRobinHoodHashTable::print() const {
  LOG_TRACE("Enter");
  std::cout << "(Length: " << this->length_ << "/Capacity: " << this->capacity_ << ") [\n";
  for (size_t i = 0; i < this->capacity_; ++i) {
    std::cout << "\t" << i << ": ";
    const SequentialBucket &bkt = this->buckets_[i];
    bkt.print(this->capacity_);
    std::cout << ",\n";
  }
  std::cout << "]" << std::endl;
}

ErrorType
SequentialRobinHoodHashTable::insert(KeyType key, ValueType value) {
  LOG_TRACE("Enter");
  // TODO: ensure key and value are valid
  // 1. Error check arguments
  // 2. Check if already present
  // 3.   If not, check if room to insert
  // 4.     If not, resize
  // 5. Insert (with swapping if necessary)
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->capacity_);

  const auto [status, offset] = get_wouldbe_offset(this->buckets_, key, hashcode, home);
  switch (status) {
  case SearchStatus::found_match: {
    LOG_DEBUG("SearchStatus::found_match");
    ErrorType e = insert_without_resize(this->buckets_, key, value, hashcode);
    assert(e == ErrorType::ok && "error in insert_without_resize");
    return e;
  }
  case SearchStatus::found_hole: {
    LOG_DEBUG("SearchStatus::found_hole");
    ErrorType e = insert_without_resize(this->buckets_, key, value, hashcode);
    assert(e == ErrorType::ok && "error in insert_without_resize");
    ++this->length_;
    return e;
  }
  case SearchStatus::found_swap:
  case SearchStatus::found_nohole: {
    LOG_DEBUG("SearchStatus::FOUND_{SWAP,NOHOLE}");
    // Ensure suitably empty and there is at least one hole
    if (static_cast<double>(this->length_) >= 0.9 * static_cast<double>(this->capacity_) ||
        this->length_ + 1 >= this->capacity_) {
      ErrorType e = this->resize(2 * this->capacity_);
      assert(e == ErrorType::ok && "error in resize");
    }
    ErrorType e = insert_without_resize(this->buckets_, key, value, hashcode);
    assert(e == ErrorType::ok && "error in insert_without_resize");
    ++this->length_;
    return e;
  }
  default:
    assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}

std::optional<ValueType>
SequentialRobinHoodHashTable::search(KeyType key) const {
  LOG_TRACE("Enter");
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->capacity_);

  const auto [status, offset] = get_wouldbe_offset(this->buckets_, key, hashcode, home);
  switch (status) {
    case SearchStatus::found_match: {
      size_t real_index = get_real_index(home, offset, this->capacity_);
      const SequentialBucket &bkt = this->buckets_[real_index];
      return bkt.value;
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
SequentialRobinHoodHashTable::remove(KeyType key) {
  LOG_TRACE("Enter");
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->capacity_);

  const auto [status, offset] = get_wouldbe_offset(this->buckets_, key, hashcode, home);
  switch (status) {
    case SearchStatus::found_match: {
      for (size_t i = 0; i < this->capacity_; ++i) {
        // NOTE(dchu): real_index is the previous iteration's next_real_index
        size_t real_index = get_real_index(home, offset + i, this->capacity_);
        SequentialBucket &bkt = this->buckets_[real_index];
        size_t next_real_index = get_real_index(home, offset + i + 1, this->capacity_);
        SequentialBucket &next_bkt = this->buckets_[next_real_index];
        // Next element is empty or already in its home bucket
        if (next_bkt.is_empty() || next_bkt.offset == 0) {
          bkt.invalidate();
          --this->length_;
          return ErrorType::ok;
        }
        // I argue that this sliding is efficient if the average home has only a
        // single element belonging to it. In this case, it would not have any
        // elements belonging to the same home, over which it may leap-frog.
        bkt = std::move(next_bkt);
        --bkt.offset;
      }
      assert(0 && "impossible! Should have a hole");
    }
    // Not found
    case SearchStatus::found_hole:
    case SearchStatus::found_nohole:
    case SearchStatus::found_swap:
      return ErrorType::e_notfound;
    default:
      assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}

ErrorType
SequentialRobinHoodHashTable::resize(size_t new_size) {
  LOG_TRACE("Enter");
  std::vector<SequentialBucket> tmp_bkts;
  tmp_bkts.resize(new_size);
  assert(new_size >= this->length_ && "not enough room in new array!");
  for (auto &bkt : this->buckets_) {
    if (!bkt.is_empty()) {
      ErrorType e = insert_without_resize(tmp_bkts, bkt.key, bkt.value, bkt.hashcode);
      assert(e == ErrorType::ok && "should not have error in insert_without_resize");
    }
  }
  this->buckets_ = tmp_bkts;
  this->capacity_ = new_size;
  return ErrorType::ok;
}

