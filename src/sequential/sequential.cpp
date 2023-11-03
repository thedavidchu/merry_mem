#include "sequential.hpp"


////////////////////////////////////////////////////////////////////////////////
/// HELPER CLASSES
////////////////////////////////////////////////////////////////////////////////

bool
SequentialBucket::is_empty() {
  LOG_TRACE("Enter");
  return this->offset == SIZE_MAX;
}

void
SequentialBucket::set(const KeyType key,
                      const ValueType value,
                      const HashCodeType hashcode,
                      const size_t offset) {
  LOG_TRACE("Enter");
  assert(this->is_empty() && "overwriting non-empty SequentialBucket");
  this->key = key;
  this->value = value;
  this->hashcode = hashcode;
  this->offset = offset;
}

void
SequentialBucket::swap(SequentialBucket &other) {
  LOG_TRACE("Enter");
  KeyType tmp_key = this->key;
  this->key = other.key;
  other.key = tmp_key;

  ValueType tmp_value = this->value;
  this->value = other.value;
  other.value = tmp_value;

  HashCodeType tmp_hc = this->hashcode;
  this->hashcode = other.hashcode;
  other.hashcode = tmp_hc;

  size_t tmp_offset = this->offset;
  this->offset = other.offset;
  other.offset = tmp_offset;
}

bool
SequentialBucket::equal_key(const KeyType key, const HashCodeType hashcode) {
  LOG_TRACE("Enter");
  assert(!this->is_empty() && "comparing to empty bucket!");
  // Assume equality between hashcodes is simpler (because it is fixed for any
  // type of key)
  return this->hashcode == hashcode && this->key == key;
}

void
SequentialBucket::print(const size_t size) {
  if (this->is_empty()) {
    std::cout << "(empty)";
  } else {
    std::cout << "(" << this->hashcode << "=>" << this->hashcode % size <<
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
  // I use the suffix '*ULL' to denote that the literal is an int64. I've had
  // weird bugs in the past to do with literal conversion. I'm not sure the
  // details. I only remember it was a right pain.
  k = ((k >> 30) ^ k) * 0xbf58476d1ce4e5b9ULL;
  k = ((k >> 27) ^ k) * 0x94d049bb133111ebULL;
  k =  (k >> 31) ^ k;
  return reinterpret_cast<HashCodeType>(k);
}

size_t
get_home(const HashCodeType hashcode, const size_t size) {
  LOG_TRACE("Enter");
  size_t h = reinterpret_cast<HashCodeType>(hashcode);
  return h % size;
}

/// @brief  Get real bucket index.
size_t
get_real_index(const size_t home, const size_t offset, const size_t size) {
  LOG_TRACE("Enter");
  return (home + offset) % size;
}

std::pair<SearchStatus, size_t>
get_wouldbe_offset(      std::vector<SequentialBucket> &tmp_buckets,
                   const KeyType key,
                   const HashCodeType hashcode,
                   const size_t home) {
  LOG_TRACE("Enter");
  size_t i = 0;
  size_t size = tmp_buckets.size();
  for (i = 0; i < size; ++i) {
    size_t real_index = get_real_index(home, i, tmp_buckets.size());
    SequentialBucket &bkt = tmp_buckets[real_index];
    // If not found
    if (bkt.is_empty()) {
      // This is first, because equality on an empty bucket is not well defined.
      return {FOUND_HOLE, i};
    } else if (bkt.offset < i) { // This means that bkt belongs to a nearer home
      return {FOUND_SWAP, i};
    // If found
    } else if (bkt.equal_key(key, hashcode)) {
      return {FOUND_MATCH, i};
    }
  }
  return {FOUND_NOHOLE, SIZE_MAX};
}

int
insert_without_resize(      std::vector<SequentialBucket> &tmp_buckets,
                          const KeyType key,
                          const ValueType value,
                          const HashCodeType hashcode) {
  LOG_TRACE("Enter");
  SequentialBucket tmp = SequentialBucket(key, value, hashcode, /*random value*/0);
  // This could also be upper-bounded by the number of valid elements (num_elem)
  // in tmp_buckets. This is because you need to bump at most num_elem elements
  // (if they are all sitting in a row) to insert something.
  while (true) {
    size_t home = get_home(tmp.hashcode, tmp_buckets.size());
    const auto [status, offset] = get_wouldbe_offset(tmp_buckets, tmp.key, tmp.hashcode, home);
    switch (status) {
      case FOUND_MATCH: {
        LOG_DEBUG("FOUND_MATCH");
        size_t real_index = get_real_index(home, offset, tmp_buckets.size());
        SequentialBucket &bkt = tmp_buckets[real_index];
        bkt.value = tmp.value;
        return 0;
      }
      case FOUND_SWAP: {
        LOG_DEBUG("FOUND_SWAP");
        size_t real_index = get_real_index(home, offset, tmp_buckets.size());
        SequentialBucket &bkt = tmp_buckets[real_index];
        tmp.offset = offset;
        bkt.swap(tmp);
        // TODO(dchu): likely to be buggy
        continue;
      }
      case FOUND_HOLE: {
        LOG_DEBUG("FOUND_HOLE");
        size_t real_index = get_real_index(home, offset, tmp_buckets.size());
        SequentialBucket &bkt = tmp_buckets[real_index];
        tmp.offset = offset;
        bkt.swap(tmp);
        // TODO(dchu): likely to be buggy
        return 0;
      }
      case FOUND_NOHOLE:
        assert(0 && "no hole found!");
        exit(1);
      default:
        assert(0 && "impossible!");
        exit(1);
    }
  }
  assert(0 && "impossible!");
  exit(1);
}


////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE CLASS
////////////////////////////////////////////////////////////////////////////////

int SequentialRobinHoodHashTable::insert(KeyType key, ValueType value) {
  LOG_TRACE("Enter");
  // TODO: ensure key and value are valid
  // 1. Error check arguments
  // 2. Check if already present
  // 3.   If not, check if room to insert
  // 4.     If not, resize
  // 5. Insert (with swapping if necessary)
  int r = 0;
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->size);

  const auto [status, offset] = get_wouldbe_offset(this->buckets, key, hashcode, home);
  switch (status) {
  case FOUND_MATCH:
    LOG_DEBUG("FOUND_MATCH");
    r = insert_without_resize(this->buckets, key, value, hashcode);
    assert(!r && "error in insert_without_resize");
    return r;
  case FOUND_HOLE:
    LOG_DEBUG("FOUND_HOLE");
    r = insert_without_resize(this->buckets, key, value, hashcode);
    assert(!r && "error in insert_without_resize");
    ++this->length;
    return r;
  case FOUND_SWAP:
  case FOUND_NOHOLE:
    LOG_DEBUG("FOUND_{SWAP,NOHOLE}");
    // Ensure suitably empty and there is at least one hole
    if (this->length >= 0.9 * this->size || this->length + 1 >= this->size) {
      int r = this->resize(2 * this->size);
      assert(!r && "error in resize");
    }
    r = insert_without_resize(this->buckets, key, value, hashcode);
    assert(!r && "error in insert_without_resize");
    ++this->length;
    return r;
  default:
    assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}

int SequentialRobinHoodHashTable::search(KeyType key, ValueType *value) {
  LOG_TRACE("Enter");
  // TODO
  // 1. Error check arguments
  // 2. Check if key present
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->size);

  const auto [status, offset] = get_wouldbe_offset(this->buckets, key, hashcode, home);
  switch (status) {
    case FOUND_MATCH: {
      size_t real_index = get_real_index(home, offset, this->size);
      SequentialBucket &bkt = this->buckets[real_index];
      *value = bkt.value;
      return 0;
    }
    case FOUND_HOLE:
    case FOUND_NOHOLE:
    case FOUND_SWAP:
      return -1;
    default:
      assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}

int SequentialRobinHoodHashTable::remove(KeyType key) {
  LOG_TRACE("Enter");
  HashCodeType hashcode = hash(key);
  size_t home = get_home(hashcode, this->size);

  const auto [status, offset] = get_wouldbe_offset(this->buckets, key, hashcode, home);
  switch (status) {
    case FOUND_MATCH: {
        size_t real_index = get_real_index(home, offset, this->size);
        SequentialBucket tmp = SequentialBucket();
        SequentialBucket &bkt = this->buckets[real_index];
        bkt.swap(tmp);
        this->remove(tmp.key);
        return 0;
    }
    // Not found
    case FOUND_HOLE:
    case FOUND_NOHOLE:
    case FOUND_SWAP:
      return -1;
    default:
      assert(0 && "impossible");
  }
  assert(0 && "unreachable");
}
