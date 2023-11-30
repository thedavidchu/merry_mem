#include <cassert>

#include "parallel/parallel.hpp"

constexpr unsigned indices_per_segment = 16;

////////////////////////////////////////////////////////////////////////////////
/// SEGMENT LOCK
////////////////////////////////////////////////////////////////////////////////

SegmentLock::Version
SegmentLock::get_version()
{
    return this->version_;
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
    ++this->version_;
    ++this->locked_count_;
}

void
SegmentLock::unlock()
{
    for (unsigned i = 0; i < this->locked_count_; ++i) {
        this->mutex_.unlock();
    }

    this->locked_count_ = 0;
}


////////////////////////////////////////////////////////////////////////////////
/// THREAD MANAGER
////////////////////////////////////////////////////////////////////////////////

ThreadManager::ThreadManager(ParallelRobinHoodHashTable *const hash_table)
    : hash_table_(hash_table)
{
}

void
ThreadManager::lock(size_t index)
{
    size_t segment_index = get_segment_index(index);
    this->hash_table_->segment_locks_[segment_index].lock();
    this->locked_segments_.push_back(segment_index);
}

void
ThreadManager::release_all_locks()
{
    for (auto i : this->locked_segments_) {
        this->hash_table_->segment_locks_[i].unlock();
    }
    this->locked_segments_.clear();
}

bool
ThreadManager::speculate_index(size_t index)
{
    size_t segment_index = get_segment_index(index);
    SegmentLock &segment_lock = this->hash_table_->segment_locks_[segment_index];
    // Perform the get_version() and then the is_locked() functions in that
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
    SegmentLock::Version segment_lock_version = segment_lock.get_version();
    bool segment_locked = segment_lock.is_locked();
    if (!segment_locked) {
        this->segment_lock_index_and_version_.emplace_back(segment_index, segment_lock_version);
        return true;
    }
    return false;
}

bool
ThreadManager::finish_speculate()
{
    for (auto &[seg_idx, old_seg_version] : this->segment_lock_index_and_version_) {
        SegmentLock::Version new_seg_version = this->hash_table_->segment_locks_[seg_idx].get_version();
        if (old_seg_version != new_seg_version) {
            this->segment_lock_index_and_version_.clear();
            this->locked_segments_.clear();
            return false;
        }
    }
    this->segment_lock_index_and_version_.clear();
    this->locked_segments_.clear();
    return true;
}

size_t
get_segment_index(size_t index)
{
    return index / indices_per_segment;
}


////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE
////////////////////////////////////////////////////////////////////////////////

bool
ParallelRobinHoodHashTable::insert(KeyType key, ValueType value)
{
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, this->capacity_);
    LOG_DEBUG(key << " hashes to " << home);

    // (fast path) first try distance zero insert
    const InsertStatus insert_status = this->distance_zero_insert(key, value, home);
    if (insert_status == InsertStatus::inserted_at_home ||
            insert_status == InsertStatus::updated_at_home) {
        return true;
    }
    
    // TODO: (fast path) search with RH invariant to see if can atomically update
    // size_t idx = home + 1;
    // KeyValue new_kv = {.key=key, .value=value};
    // for(size_t new_offset = 1; ; ++new_offset, ++idx){
    //     KeyValue curr_kv = this->buckets_[idx].load();
    //     size_t curr_offset = idx - hash(curr_kv.key);
    //     if (curr_offset < new_offset){
    //         break;
    //     }
    //     while(curr_kv.key == key) {
    //         bool updated = this->compare_and_set_key_val(idx, curr_kv, new_kv);
    //         if (updated == true) {
    //             return true;
    //         }
    //     }
    // }

    // now slow path
    ThreadManager manager = this->get_thread_lock_manager();
    KeyValue new_kv = {.key=key, .value=value};
    ParallelBucket entry_to_insert(new_kv);
    auto [next_index, found] = this->find_next_index_lock(manager, home, key);

    // locked search + update 
    if(found){
        bool updated = this->compare_and_set_key_val(next_index, this->buckets_[next_index].load(), new_kv);
        if (updated == true) {
            manager.release_all_locks();
            return true;
        }
    }

    // locked insert
    bool inserted = this->locked_insert(entry_to_insert, next_index);
    if (inserted == true) {
        manager.release_all_locks();
        return true;
    }

    manager.release_all_locks();
    return false;
}

bool
ParallelRobinHoodHashTable::remove(KeyType key)
{
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, this->capacity_);

    // first try fast path
    const KeyValue expected = this->atomic_load_key_val(home);
    const KeyValue right_neighbor = this->atomic_load_key_val(home+1);
    bool validRight = right_neighbor.key == bucket_empty_key || 
            home+1 == get_home(hash(right_neighbor.key), this->capacity_);
    if (expected.key == key && validRight) {
        KeyValue del_key = { .key = bucket_empty_key };
        if (this->compare_and_set_key_val(home, expected, del_key)) {
            return true;
        }
    }

    // now try slow path
    ThreadManager manager = this->get_thread_lock_manager();
    auto[index, found] = this->find_next_index_lock(manager, home, key);

    if(!found) {
        manager.release_all_locks();
        return false;
    }

    size_t next_index = index++;
    size_t curr_index = index;
    manager.lock(next_index); 

    size_t next_offset = next_index - get_home(hash(atomic_load_key_val(next_index).key), this->capacity_);
    KeyValue empty_entry = {.key=bucket_empty_key}; 

    // shift to the left
    while(next_offset > 0) {
        manager.lock(next_index); 
        KeyValue entry_to_move = this->do_atomic_swap(empty_entry, next_index); //na this is fucked up 
        this->buckets_[curr_index].store(entry_to_move);
        curr_index = next_index;
        ++next_index;
        next_offset = next_index - get_home(hash(atomic_load_key_val(next_index).key), this->capacity_);
    }

    this->buckets_[curr_index].store(empty_entry);
    manager.release_all_locks();
    return true;
}

std::pair<ValueType, bool>
ParallelRobinHoodHashTable::find(KeyType key)
{
    // first try fast path
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, this->capacity_);
    KeyValue zero_distance_key_pair = atomic_load_key_val(home);

    if (zero_distance_key_pair.key == key) {
        return {zero_distance_key_pair.key, true};
    }
    if (home == get_home(hash(zero_distance_key_pair.key), this->capacity_)) {
        return {-1, false};
    }
    
    size_t tries = 0;
    ValueType value = 0;
    bool found = false;
    bool speculative_success = false;
    // As dictated by Griffin and David.
    constexpr size_t max_tries = 10;

    while (tries < max_tries) {
        tries++;
        std::tie(value, found, speculative_success) = this->find_speculate(key, home);

        if (speculative_success) {
            return {value, found};
        }
    }

    ThreadManager &manager = this->get_thread_lock_manager();
    size_t index = 0;
    std::tie(index, found) = this->find_next_index_lock(manager, home, key);
    value = this->buckets_[index].load().value;
    manager.release_all_locks();
    return {value, found};
}

void
ParallelRobinHoodHashTable::add_thread_lock_manager()
{
    std::thread::id t_id = std::this_thread::get_id();
    assert(!this->thread_managers_.contains(t_id) &&
           "thread manager for current thread already exists");
    this->thread_managers_.emplace(t_id, ThreadManager(this));
}

void
ParallelRobinHoodHashTable::remove_thread_lock_manager()
{
    std::thread::id t_id = std::this_thread::get_id();
    assert(this->thread_managers_.contains(t_id) &&
           "thread manager for current thread DNE");
    this->thread_managers_.erase(t_id);
}


std::pair<size_t, bool>
ParallelRobinHoodHashTable::find_next_index_lock(ThreadManager &manager,
                                                 size_t start_index,
                                                 KeyType key)
{
    LOG_TRACE("Enter");
    const size_t capacity = this->buckets_.size();
    for (size_t i = 0; i < capacity; ++i) {
        const size_t real_index = start_index + i;
        if (real_index == capacity)
        {
            assert("the map should never be completely full" && false);
            return {SIZE_MAX, false};
        }
        manager.lock(real_index);

        const KeyValue pair = this->atomic_load_key_val(real_index);

        if (pair.key == bucket_empty_key) {
            // This is first, because equality on an empty bucket is not well defined.
            return {real_index, false};
        } else if (pair.key == key) { // If found
            return {real_index, true};
        }
    }

    assert("the map should never be completely full" && false);
    return {SIZE_MAX, false};
}

ThreadManager &
ParallelRobinHoodHashTable::get_thread_lock_manager()
{
    std::thread::id t_id = std::this_thread::get_id();
    assert(this->thread_managers_.contains(t_id) &&
           "thread manager for current thread DNE");
    return this->thread_managers_.at(t_id);
}

bool
ParallelRobinHoodHashTable::compare_and_set_key_val(size_t index,
                                                    KeyValue prev_kv,
                                                    const KeyValue &new_kv)
{
    return this->buckets_[index].compare_exchange_strong(prev_kv, new_kv);
}

KeyValue
ParallelRobinHoodHashTable::do_atomic_swap(const KeyValue &swap_entry, size_t index)
{
    return this->buckets_[index].exchange(swap_entry);
}

KeyValue
ParallelRobinHoodHashTable::atomic_load_key_val(size_t index)
{
    return this->buckets_[index].load(); 
}

InsertStatus
ParallelRobinHoodHashTable::distance_zero_insert(KeyType key,
                                                 ValueType value,
                                                 size_t dist_zero_slot)
{
    KeyValue blank_kv;
    KeyValue new_kv = {key, value};
    // Attempt fast-path insertion if the key-value pair is empty
    if (this->buckets_[dist_zero_slot].load() == blank_kv) {
        if (compare_and_set_key_val(dist_zero_slot, blank_kv, new_kv)) {
            return InsertStatus::inserted_at_home;
        };
    }
    // Attempt fast-path update if key matches our key
    if (this->buckets_[dist_zero_slot].load().key == key) {
        KeyValue old_kv = {key, this->buckets_[dist_zero_slot].load().value};
        if (compare_and_set_key_val(dist_zero_slot, old_kv, new_kv)) {
            return InsertStatus::updated_at_home;
        };
    }
    return InsertStatus::not_inserted;
}

bool
ParallelRobinHoodHashTable::locked_insert(ParallelBucket &entry_to_insert, size_t swap_index)
{
    KeyValue entry_to_swap = entry_to_insert;
    ThreadManager manager = this->get_thread_lock_manager(); 
    while(true) {
        entry_to_swap = this->do_atomic_swap(entry_to_insert, swap_index);
        if(entry_to_swap.key == bucket_empty_key) { //size max is empty 
            return true;
        }
        auto [next_swap_index, found] = this->find_next_index_lock(manager, swap_index, entry_to_swap.key);
        swap_index = next_swap_index;
    }
}

std::tuple<ValueType, bool, bool>
ParallelRobinHoodHashTable::find_speculate(KeyType key, size_t start_index)
{   
    // TODO    
    bool speculative_success = false;
    bool found = false;
    ValueType value = 0; 
    size_t index = start_index;
    ThreadManager manager = this->get_thread_lock_manager();
    
    size_t offset = index - get_home(hash(atomic_load_key_val(index).key), this->capacity_);

    for(size_t distance = 0; offset >= distance; ++distance){
        (void)manager.speculate_index(index);
        KeyValue current_bucket = atomic_load_key_val(index);
        if(current_bucket.key == key){
            value = current_bucket.value;
            found = true;
            break;
        }
        ++index;
        offset = index - get_home(hash(atomic_load_key_val(index).key), this->capacity_);
    }
    speculative_success = manager.finish_speculate();
    return std::make_tuple(value, found, speculative_success);
}
