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
/// STATIC HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

HashCodeType
hash(const KeyType key)
{
    LOG_TRACE("Enter");
    size_t k = static_cast<size_t>(key);
    // I use the suffix '*ULL' to denote that the literal is at least an int64.
    // I've had weird bugs in the past to do with literal conversion. I'm not sure
    // the details. I only remember it was a huge pain.
    k = ((k >> 30) ^ k) * 0xbf58476d1ce4e5b9ULL;
    k = ((k >> 27) ^ k) * 0x94d049bb133111ebULL;
    k = (k >> 31) ^ k;
    // We could theoretically use `reinterpret_cast` to ensure there is no
    // overhead in this cast because HashCodeType is typedef'ed to size_t.
    return reinterpret_cast<HashCodeType>(k);
}

size_t
get_home(const HashCodeType hashcode, const size_t size)
{
    LOG_TRACE("Enter");
    size_t h = static_cast<size_t>(hashcode);
    return h % size;
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
    
    // first try fast path
    const InsertStatus insert_status = this->distance_zero_insert(key, value, home);
    if (insert_status == InsertStatus::inserted_at_home ||
            insert_status == InsertStatus::updated_at_home) {
        return true;
    }

    // now try slow path
    ThreadManager manager = this->get_thread_lock_manager();
    KeyValue key_value = {.key=key, .value=value};
    ParallelBucket entry_to_insert(key_value);
    auto [next_index, found] = this->find_next_index_lock(manager, home, key);
    if (found) {
        manager.release_all_locks();
        return false;
    }

    locked_insert(entry_to_insert, next_index);
    manager.release_all_locks();
    return true;
}

bool
ParallelRobinHoodHashTable::remove(KeyType key, ValueType value)
{
    HashCodeType hashcode = hash(key);
    size_t home = get_home(hashcode, this->capacity_);
    // first try fast path
    // TODO: Fast path

    // now try slow path
    KeyValue key_value = {.key=key, .value=value};
    ParallelBucket entry_to_insert(key_value);

    ThreadManager manager = this->get_thread_lock_manager();

    auto[index, found] = this->find_next_index_lock(manager, home, key);

    if(!found) {
        manager.release_all_locks();
        return false;
    }

    size_t next_index = index++;
    size_t curr_index = index;
    
    manager.lock(curr_index);
    while(this->buckets_[next_index].offset > 0 && !entry_to_insert.is_empty()) {
        manager.lock(next_index); 
        ParallelBucket &entry_to_move = this->do_atomic_swap(entry_to_insert, next_index); //na this is fucked up 
        this->buckets_[curr_index].key = entry_to_move.key;
        this->buckets_[curr_index].value = entry_to_move.value;
        this->buckets_[curr_index].hashcode = entry_to_move.hashcode;
        this->buckets_[curr_index].offset = entry_to_move.offset--;
        curr_index = next_index;
        ++next_index;
    }

    entry_to_insert.unlock();
    ParallelBucket empty_entry; //does this do the default vals? 
    this->buckets_[curr_index].key = empty_entry.key;
    this->buckets_[curr_index].value = empty_entry.value;
    this->buckets_[curr_index].hashcode = empty_entry.hashcode;
    this->buckets_[curr_index].offset = empty_entry.offset;
    manager.release_all_locks();
    return true;
}

std::pair<ValueType, bool>
ParallelRobinHoodHashTable::find(KeyType key)
{
    HashCodeType hashcode =this->hash(key);
    size_t home = this->get_home(hashcode, this->capacity_);
    ParallelBucket &zero_distance_key_pair = atomic_load_key_index(home);

    if(zero_distance_key_pair.key == key) {
        return {zero_distance_key_pair.key, true};
    }
    if(zero_distance_key_pair.offset == 0) {
        return {-1, false};
    }
    
    size_t tries = 0;
    //ValueType value_copy = 0;
    while (tries < MAX_TRIES) {
        tries++;
        auto [value, found, speculative_success] = this->find_speculate(key, home);

        if (speculative_success) {
            return {value, found};
        }
    }
    ParallelBucket &entry_to_find = {.key = key, .value = value_copy, .hashcode = hashcode, .offset = 0};
    ThreadManager &manager = this->get_thread_lock_manager();
    auto [index, found] = this->find_next_index_lock(manager, home, key, entry_to_find.offset);
    ValueType value = this->buckets_[index].value;
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

/// @brief  Get real bucket index.
static size_t
get_real_index(const size_t home, const size_t offset, const size_t capacity)
{
    LOG_TRACE("Enter");
    return (home + offset) % capacity; 
}

std::pair<size_t, bool>
ParallelRobinHoodHashTable::find_next_index_lock(ThreadManager &manager,
                                                 size_t start_index,
                                                 KeyType key)
{
    LOG_TRACE("Enter");
    const size_t capacity = this->buckets_.size();
    for (size_t i = 0; i < capacity; ++i) {
        const size_t real_index = get_real_index(start_index, i, capacity);
        manager.lock(real_index);

        const KeyValue pair = this->atomic_load_key_val(real_index);
        const size_t home = hash(pair.key);
        const size_t offset = real_index - home;

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
                                                    KeyValue new_kv)
{
    return this->buckets_[index].compare_exchange_strong(prev_kv, new_kv);
}

KeyValue
ParallelRobinHoodHashTable::do_atomic_swap(ParallelBucket &swap_entry, size_t index)
{
    return this->buckets_[index].exchange(swap_entry);
}

ParallelBucket &
ParallelRobinHoodHashTable::atomic_load_key_index(size_t index)
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
    // TODO
    //size_t swap_index = entry_to_insert.offset; //? i thnk its redoing it bc of possible contention but its locked so??
    
    ThreadManager manager = this->get_thread_lock_manager(); 
    while(true) {
        ParallelBucket &swapped_entry = this->do_atomic_swap(entry_to_insert, swap_index);
        if(swapped_entry.offset == SIZE_MAX) { //size max is empty 
            return true;
        }
        size_t swap_index_copy = swap_index;
        auto [swap_index, is_found] = this->find_next_index_lock(manager, swap_index_copy, entry_to_insert.key, entry_to_insert.offset);
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
    ThreadManager manager = this->get_thread_lock_manager();

    for(size_t distance = 0; this->buckets_[index].offset >= distance; ++distance, ++index){
        manager.speculate_index(index);
        if(this->buckets_[index].key == key){
            value = this->buckets_[index].value;
            found = true;
            break;
        }
    }
    speculative_sucess = manager.finish_speculate();
    return std::make_tuple(value, found, speculative_sucess);
}