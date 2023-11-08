#include "parallel.hpp"

constexpr unsigned indices_per_segment = 16;

////////////////////////////////////////////////////////////////////////////////
/// SEGMENT LOCK
////////////////////////////////////////////////////////////////////////////////

size_t
SegmentLock::get_counter()
{
    return this->counter_;
}

bool
SegmentLock::is_locked()
{
    bool locked_by_me = this->mutex_.try_lock();
    if (locked_by_me) {
        this->mutex_.unlock();
        return false;
    }
    return true;
}

void
SegmentLock::lock()
{
    this->mutex_.lock();
    ++this->counter_;
}

void
SegmentLock::unlock()
{
    this->mutex_.unlock();
}

////////////////////////////////////////////////////////////////////////////////
/// BUCKET
////////////////////////////////////////////////////////////////////////////////

void
ParallelBucket::lock()
{
    this->mutex.lock();
}

void
ParallelBucket::unlock()
{
    this->mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////
/// THREAD MANAGER
////////////////////////////////////////////////////////////////////////////////

void
ThreadManager::lock(size_t index)
{
    size_t segment_index = this->get_segment_index(index);
    this->hash_table_->segment_locks_[segment_index].lock();
    this->locked_segments_.push_back(segment_index);
}

void
ThreadManager::release_all_locks()
{
    for (auto i : this->locked_segments_) {
        this->hash_table_->segment_locks_[i].unlock();
    }
    this->locked_segments_.resize(0);
}

bool
ThreadManager::speculate_index(size_t index)
{
    size_t segment_index = this->get_segment_index(index);
    SegmentLock &segment_lock = this->hash_table_->segment_locks_[segment_index];
    // Perform the get_counter() and then the is_locked() functions in that
    // order to avoid a race condition. If the check for lockedness happens
    // before the lock count check, then another thread could lock and increment
    // the count before we capture the count check so that they are modifying
    // the hash table while we are searching it but when they finish, we won't
    // be able to tell!
    // This Thread                      Other Thread
    // -----------                      ------------
    // 1. Check unlocked
    //                                  1. Lock
    //                                  2. Increment lock count
    // 2. Get lock count
    // 3. Search for key...             3. Modify hash table...
    size_t segment_lock_count = segment_lock.get_counter();
    bool segment_locked = segment_lock.is_locked();
    if (!segment_locked) {
        this->segment_lock_index_and_count_.push_back({segment_index, segment_lock_count});
        return true;
    }
    return false;
}

bool
ThreadManager::finish_speculate()
{
    for (auto &[seg_idx, old_seg_cnt] : this->segment_lock_index_and_count_) {
        size_t new_seg_cnt = this->hash_table_->segment_locks_[seg_idx].get_counter();
        if (old_seg_cnt != new_seg_cnt) {
            this->segment_lock_index_and_count_.resize(0);
            this->locked_segments_.resize(0);
            return false;
        }
    }
    this->segment_lock_index_and_count_.resize(0);
    this->locked_segments_.resize(0);
    return true;
}


size_t
ThreadManager::get_segment_index(size_t index)
{
    return index / indices_per_segment;
}



////////////////////////////////////////////////////////////////////////////////
/// PARALLEL BUCKET
////////////////////////////////////////////////////////////////////////////////
bool
ParallelBucket::is_empty() 
{
    if (this->offset == SIZE_MAX) {
        return true;
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

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE
////////////////////////////////////////////////////////////////////////////////

bool
ParallelRobinHoodHashTable::insert(KeyType key, ValueType value, size_t)
{
    // TODO
    HashCodeType hashcode = hash(key);
    bool is_inserted, key_exists, found; 
    size_t home = get_home(hashcode, capacity_);
    auto [is_inserted, key_exists] = distance_zero_insert(key, value, home);
    if(is_inserted) {
        return true;
    }
    if(key_exists) {
        return false;
    }
    ThreadManager manager = get_thread_lock_manager();

    size_t offset = 0;
    ParallelBucket entry_to_swap = {.key = key, .value = value, .hashcode = hashcode, .offset = offset};
    size_t next_index;

    auto [next_index, found] = find_next_index_lock(manager, home, key, offset);

    if (found) {
        manager.release_all_locks();
        return false;
    }

    locked_insert(entry_to_swap, next_index);
    manager.release_all_locks();
    return true;
}

void
ParallelRobinHoodHashTable::insert_or_update(KeyType key, ValueType value)
{
    ThreadManager manager = get_thread_lock_manager();
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, capacity_);
    size_t idx = home;

    KeyType prev_key = key;
    ValueType prev_val = value;
    bool is_inserted, is_found, found;

    for(size_t distance = 0; buckets_[home].offset >= distance; ++distance, ++idx){
        while(buckets_[idx].key == key) {
            KeyType prev_key_copy = prev_key;
            ValueType prev_val_copy = prev_val;
            auto [prev_key, prev_val] = compare_and_set_key_val(home, prev_key_copy, buckets_[idx].key, prev_val_copy);
            if (prev_key == key) {
                return;
            }
        }
    }
    auto [is_inserted, is_found] = distance_zero_insert(key, value, home);
    size_t offset = 0;
    ParallelBucket entry_to_insert = {.key = key, .value = value, .hashcode = hashcode, .offset = offset}; //offset is arbitrary
    size_t next_index;
    auto [next_index, found] = find_next_index_lock(manager, home, key, offset);

    if(found){
        buckets_[next_index].value = value;
        manager.release_all_locks();
        return;
    }

    locked_insert(entry_to_insert, next_index);
    manager.release_all_locks();
    return;
}

bool
ParallelRobinHoodHashTable::remove(KeyType key, ValueType value)
{
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, this->capacity_);
    ThreadManager manager = get_thread_lock_manager();
    ParallelBucket entry_to_insert = {.key = key, .value = value, .hashcode = hashcode, .offset = 0}; //offset is arbitrary

    auto[index, found] = this->find_next_index_lock(manager, home, key, entry_to_insert.offset);

    if(!found) {
        manager.release_all_locks();
        return false;
    }

    size_t next_index = index++;
    size_t curr_index = index;
    
    manager.lock(curr_index);
    while(buckets_[next_index].offset > 0 && !entry_to_insert.is_empty()) {
        manager.lock(next_index); 
        ParallelBucket &entry_to_move = do_atomic_swap(entry_to_insert, next_index); //na this is fucked up 
        buckets_[curr_index].key = entry_to_move.key;
        buckets_[curr_index].value = entry_to_move.value;
        buckets_[curr_index].hashcode = entry_to_move.hashcode;
        buckets_[curr_index].offset = entry_to_move.offset--;
        curr_index = next_index;
        ++next_index;
    }
    
    entry_to_insert.unlock();
    ParallelBucket empty_entry; //does this do the default vals? 
    buckets_[curr_index].key = empty_entry.key;
    buckets_[curr_index].value = empty_entry.value;
    buckets_[curr_index].hashcode = empty_entry.hashcode;
    buckets_[curr_index].offset = empty_entry.offset;
    manager.release_all_locks();
    return true;
}

std::pair<ValueType, bool>
ParallelRobinHoodHashTable::find(KeyType key)
{
    // TODO
}

std::pair<size_t, bool>
ParallelRobinHoodHashTable::find_next_index_lock(ThreadManager &manager,
                                                 size_t start_index,
                                                 KeyType key,
                                                 size_t &distance_key)
{
    // TODO
}

ThreadManager &
ParallelRobinHoodHashTable::get_thread_lock_manager()
{
    // TODO
}

std::pair<KeyType, ValueType>
ParallelRobinHoodHashTable::compare_and_set_key_val(size_t index,
                                                    KeyType prev_key,
                                                    KeyType new_key,
                                                    ValueType new_val)
{
    // TODO
}

ParallelBucket &
ParallelRobinHoodHashTable::do_atomic_swap(ParallelBucket &swap_entry, size_t index)
{
    // TODO
}

std::pair<bool, bool>
ParallelRobinHoodHashTable::distance_zero_insert(KeyType key,
                                                 ValueType value,
                                                 size_t dist_zero_slot)
{
    // TODO
}

bool
ParallelRobinHoodHashTable::locked_insert(ParallelBucket &entry_to_insert, size_t swap_index)
{
    // TODO
    //size_t swap_index = entry_to_insert.offset; //? i thnk its redoing it bc of possible contention but its locked so??
    
    ThreadManager manager = get_thread_lock_manager(); 
    while(true) {
        ParallelBucket &swapped_entry = do_atomic_swap(entry_to_insert, swap_index);
        if(swapped_entry.key == SIZE_MAX) { //size max is empty 
            return true;
        }
        size_t swap_index_copy = swap_index;
        auto [swap_index, is_found] = find_next_index_lock(manager, swap_index_copy, entry_to_insert.key, entry_to_insert.offset);
    }

    //add conditional check to check is found or not?
}

std::tuple<ValueType, bool, bool>
ParallelRobinHoodHashTable::find_speculate(KeyType key, size_t start_index)
{   
    // TODO    
    bool speculative_sucess = false;
    bool found = false;
    ValueType value = 0; 
    size_t index = start_index;
    ThreadManager manager = get_thread_lock_manager();

    for(size_t distance = 0; buckets_[index].offset >= distance; ++distance, ++index){
        manager.speculate_index(index);
        if(buckets_[index].key == key){
            value = buckets_[index].value;
            found = true;
            break;
        }
    }
    speculative_sucess = manager.finish_speculate();
    return std::make_tuple(value, found, speculative_sucess);
}