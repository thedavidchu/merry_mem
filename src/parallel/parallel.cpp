#include <cassert>

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
    : hash_table_(hash_table) {}

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
        this->segment_lock_index_and_count_.emplace_back(segment_index, segment_lock_count);
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
/// HASH TABLE
////////////////////////////////////////////////////////////////////////////////

bool
ParallelRobinHoodHashTable::insert(KeyType key, ValueType value)
{
    HashCodeType hashcode = hash(key);

    size_t home = get_home(hashcode, this->capacity_);
    LOG_DEBUG(key << " hashes to " << home);
    const auto [inserted, key_exists] = this->distance_zero_insert(key, value, home);
    if (inserted && key_exists) {
        return true;
    }
    assert(0 && "TODO: slow path");
}

void
ParallelRobinHoodHashTable::insert_or_update(KeyType key, ValueType value)
{
    // TODO
}

bool
ParallelRobinHoodHashTable::remove(KeyType key, ValueType value)
{
    // TODO
}

std::pair<ValueType, bool>
ParallelRobinHoodHashTable::find(KeyType key)
{
    // TODO
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
                                                 KeyType key,
                                                 size_t &distance_key)
{
    // TODO
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
    return this->buckets_[index].key_value.compare_exchange_strong(prev_kv, new_kv);
}

ParallelBucket
ParallelRobinHoodHashTable::do_atomic_swap(ParallelBucket &swap_entry, size_t index)
{
    // TODO
}

InsertStatus
ParallelRobinHoodHashTable::distance_zero_insert(KeyType key,
                                                 ValueType value,
                                                 size_t dist_zero_slot)
{
    KeyValue blank_kv;
    KeyValue new_kv = {key, value};
    // Attempt fast-path insertion if the key-value pair is empty
    if (this->buckets_[dist_zero_slot].key_value.load() == blank_kv) {
        if (compare_and_set_key_val(dist_zero_slot, blank_kv, new_kv)) {
            return InsertStatus::inserted_at_home;
        };
    }
    // Attempt fast-path update if key matches our key
    if (this->buckets_[dist_zero_slot].key_value.load().key == key) {
        KeyValue old_kv = {key, this->buckets_[dist_zero_slot].key_value.load().value};
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
}

std::tuple<ValueType, bool, bool>
ParallelRobinHoodHashTable::find_speculate(KeyType key, size_t start_index)
{
    // TODO
}
