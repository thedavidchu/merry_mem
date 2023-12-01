#include <cassert>
#include <iostream>
#include <memory>

#include "parallel/parallel.hpp"

static std::mutex cout_mutex;

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
    assert(false);
    return false;
}

void
SegmentLock::lock()
{
    this->mutex_.lock();
    ++this->version_;
    // ++this->locked_count_;
}

void
SegmentLock::unlock()
{
    // make a local copy, then reset locked count while we still have the lock.
    // const unsigned locked_count = this->locked_count_;
    // this->locked_count_ = 0;

    // Then, release the lock.
    // for (unsigned i = 0; i < locked_count; ++i) {
        this->mutex_.unlock();
    // }
}

////////////////////////////////////////////////////////////////////////////////
/// THREAD MANAGER
////////////////////////////////////////////////////////////////////////////////

ThreadManager::ThreadManager(ParallelRobinHoodHashTable *const hash_table)
    : 
    hash_table_(hash_table),
    is_segment_locked_(hash_table_->segment_locks_.size())
{
    // locked_segments_.reserve(hash_table->capacity_);
}

void
ThreadManager::lock(size_t index)
{
    std::vector<std::unique_lock<SegmentLock>> locked_segments;

    size_t segment_index = get_segment_index(index);
    if (this->is_segment_locked_[segment_index]) {
        return;
    }

    // this->locked_segments_.emplace_back(this->hash_table_->segment_locks_[segment_index]);
    this->is_segment_locked_[segment_index] = true;
    {
        std::scoped_lock lock(cout_mutex);
        std::cout << "thread id: " << std::this_thread::get_id() << " segment index: " << segment_index << " ACQUIRED" << std::endl;
    }

    // std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    // for (auto& s : this->locked_segments_){
    //     std::cout << s << " " ;
    // }
    // std::cout << std::endl;
}

void
ThreadManager::release_all_locks()
{
#if 0
    for (size_t i : this->locked_segments_) {
        this->hash_table_->segment_locks_[i].unlock();
        this->is_segment_locked_[i] = false;
        {
            std::scoped_lock lock(cout_mutex);
            std::cout << "thread id: " << std::this_thread::get_id() << " segment index: " << i << " RELEASED" << std::endl;
        }
    }
    this->locked_segments_.clear();
#else
    // this->locked_segments_.clear();
#endif
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
    // bool segment_locked = segment_lock.is_locked();
    bool segment_locked = this->is_segment_locked_[segment_index];
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
            // this->locked_segments_.clear();
            return false;
        }
    }
    this->segment_lock_index_and_version_.clear();
    // this->locked_segments_.clear();
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

    // Fast-path: try distance_zero_insert
    const InsertStatus insert_status = this->distance_zero_insert(key, value, home);
    if (insert_status == InsertStatus::inserted_at_home ||
            insert_status == InsertStatus::updated_at_home) {
        return true;
    }
    
    // TODO: (fast path) search with RH invariant to see if can atomically update
    // Currently commented out because we need to handle the edge case when the
    // fast-path update encounters a section of the table that is currently deleting
    // and shifting over elements.

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

    // Slow path: locked insert
    ThreadManager manager = this->get_thread_lock_manager();
    KeyValue new_kv = {.key=key, .value=value};
    ParallelBucket entry_to_insert(new_kv);
    // auto [next_index, found] = this->find_next_index_lock(manager, home, key);

    // // locked search + update 
    // if (found) {
    //     bool updated = this->compare_and_set_key_val(next_index, this->buckets_[next_index].load(), new_kv);
    //     if (updated == true) {
    //         manager.release_all_locks();
    //         return true;
    //     }
    // }

    // Locked insert
    bool inserted = this->locked_insert(entry_to_insert, home);
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

    // First, try fast path
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

    // Now try slow path
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

    // Shift entries on the right of deleted entry to the left
    while(next_offset > 0) {
        manager.lock(next_index); 
        KeyValue entry_to_move = this->do_atomic_swap(empty_entry, next_index);
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
    // First, try fast path
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
    // NOTE We arbitrarily chose a max_tries of 10
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
#if 0
    std::thread::id t_id = std::this_thread::get_id();
    this->thread_managers_mutex_.lock();
    std::cout << "created new thread: id " << t_id << std::endl;
    assert(!this->thread_managers_.contains(t_id) &&
           "thread manager for current thread already exists");
    // std::cout << "Locking..." << std::endl;
    
    // std::cout << "Locked." << std::endl;
    this->thread_managers_.emplace(t_id, std::make_unique<ThreadManager>(this));
    // std::cout << "Unlocking..." << std::endl;
    this->thread_managers_mutex_.unlock();
    // std::cout << "Unlocked!" << std::endl;
#endif
}

void
ParallelRobinHoodHashTable::remove_thread_lock_manager()
{
#if 0
    std::thread::id t_id = std::this_thread::get_id();
    this->thread_managers_mutex_.lock();
    assert(this->thread_managers_.contains(t_id) &&
           "thread manager for current thread DNE");
    this->thread_managers_.erase(t_id);
    this->thread_managers_mutex_.unlock();
#endif
}


std::pair<size_t, bool>
ParallelRobinHoodHashTable::find_next_index_lock(ThreadManager &manager,
                                                 size_t start_index,
                                                 KeyType key)
{
    LOG_TRACE("Enter");
    const size_t capacity_with_buffer = this->capacity_with_buffer_;
    for (size_t i = 0; i < capacity_with_buffer; ++i) {
        const size_t real_index = start_index + i;
        if (real_index >= capacity_with_buffer)
        {
            // assert("the map should never be completely full" && false);
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
#if 0
    std::thread::id t_id = std::this_thread::get_id();
    this->thread_managers_mutex_.lock();
    if (!this->thread_managers_.contains(t_id)) {
        std::cout << "thread manager does not contain " << t_id << std::endl;
        for (auto& [key, value]: this->thread_managers_) {  
            std::cout << key << " " << value << std::endl; 
        }
    }
    assert(this->thread_managers_.contains(t_id) &&
           "thread manager for current thread DNE");
    ThreadManager &r = *this->thread_managers_.at(t_id);
    this->thread_managers_mutex_.unlock();
    return r;
#else
    thread_local ThreadManager manager(this);
    return manager;
#endif
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
        if (found) {
            swap_index = next_swap_index;
        } else {
            return false;
        }
        if (swap_index == next_swap_index){
            return true;
        } 
    }
}

std::tuple<ValueType, bool, bool>
ParallelRobinHoodHashTable::find_speculate(KeyType key, size_t start_index)
{   
    bool speculative_success = false;
    bool found = false;
    ValueType value = 0; 
    size_t index = start_index;
    ThreadManager manager = this->get_thread_lock_manager();
    
    size_t offset = index - get_home(hash(atomic_load_key_val(index).key), this->capacity_);

    for(size_t distance = 0; offset >= distance && index <= this->capacity_with_buffer_ ; ++distance){
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
