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
/// HASH TABLE
////////////////////////////////////////////////////////////////////////////////

bool
ParallelRobinHoodHashTable::insert(KeyType key, ValueType value)
{
    // TODO
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
}

std::tuple<ValueType, bool, bool>
ParallelRobinHoodHashTable::find_speculate(KeyType key, size_t start_index)
{
    // TODO
}
