#include "parallel.hpp"

////////////////////////////////////////////////////////////////////////////////
/// THREAD MANAGER
////////////////////////////////////////////////////////////////////////////////

void
ThreadManager::lock(size_t next_index)
{
    // TODO
}

void
ThreadManager::speculate_index(size_t index)
{
    // TODO
}

bool
ThreadManager::finish_speculate()
{
    // TODO
}

void
ThreadManager::release_all_locks()
{
    // TODO
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
