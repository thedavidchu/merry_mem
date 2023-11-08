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
/// PARALLEL BUCKET
////////////////////////////////////////////////////////////////////////////////
bool
ParallelBucket::is_empty() 
{
    if (offset == SIZE_MAX) {
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
get_home(const HashCodeType hashcode, const size_t size) {
  LOG_TRACE("Enter");
  size_t h = static_cast<size_t>(hashcode);
  return h % size;
}

////////////////////////////////////////////////////////////////////////////////
/// HASH TABLE
////////////////////////////////////////////////////////////////////////////////

bool
ParallelRobinHoodHashTable::insert(KeyType key, ValueType value)
{
    // TODO
    HashCodeType index = hash(key);
    bool is_inserted, key_exists, found; 
    std::pair(is_inserted, key_exists) = distance_zero_insert(key, value, index);
    if(is_inserted) {
        return true;
    }
    if(key_exists) {
        return false;
    }
    ThreadManager manager = get_thread_lock_manager();

    size_t offset = 0;
    ParallelBucket entry_to_swap = ParallelBucket(key, value, index, offset);
    size_t next_index;

    std::pair(next_index, found) = find_next_index_lock(manager, index, key, offset);

    if (found) {
        manager.release_all_locks();
        return false;
    }

    locked_insert(entry_to_swap, next_index);
    manager.release_all_locks();
    return true;
}

void
ParallelRobinHoodHashTable::InsertOrUpdate(KeyType key, ValueType value)
{
    // TODO
    ThreadManager manager = get_thread_lock_manager();
    HashCodeType index = hash(key);
    HashCodeType idx = index;

    KeyType prev_key = key;
    ValueType prev_val = value;
    bool is_inserted, is_found, found;

    for(size_t distance = 0; buckets_[index].offset >= distance; ++distance, ++index){
        while(buckets_[idx].key == key) {
            std::pair(prev_key, prev_val) = compare_and_set_key_val(idx, prev_key, buckets_[idx].key, prev_val);
            if (prev_key == key) {
                return;
            }
        }
    }
    std::pair(is_inserted, is_found) = distance_zero_insert(key, value, index);
    size_t offset = 0;
    ParallelBucket entry_to_insert = ParallelBucket(key, value, index, offset);
    size_t next_index;
    std::pair(next_index, found) = find_next_index_lock(manager, index, key, offset);

    if(found){
        buckets_[next_index] = entry_to_insert;
        manager.release_all_locks();
        return;
    }

    locked_insert(entry_to_insert, next_index);
    manager.release_all_locks();
    return;
}

bool
ParallelRobinHoodHashTable::delete_element(KeyType key, ValueType value)
{
    // TODO
    ThreadManager manager = get_thread_lock_manager();
    HashCodeType index = hash(key);
    size_t offset = 0;
    bool found;
    ParallelBucket entry_to_delete = ParallelBucket(key, value, index, offset);
    std::pair(index, found) = find_next_index_lock(manager, index, entry_to_delete.key, entry_to_delete.offset); //not sure if this is referenced correct?
    
    if (!found) {
        manager.release_all_locks();
        return false;
    }

    size_t next_index = index++;
    manager.lock(next_index);
    while(buckets_[next_index].offset > 0 ){
        ParallelBucket entry_to_swap = do_atomic_swap(entry_to_delete, next_index); //idk pseduo code says fucking locked entry
        buckets_[index] = entry_to_swap;
        buckets_[index].offset--;
        index = next_index;
        next_index++;
    }

    KeyType empty_key = 0;
    ValueType empty_value = 0;
    HashCodeType empty_hashcode = 0;
    size_t empty_offset = SIZE_MAX;

    ParallelBucket empty_entry = ParallelBucket(empty_key, empty_value, empty_hashcode, empty_offset);
    buckets_[index] = empty_entry;
    manager.release_all_locks();
    return true;
}

std::pair<ValueType, bool>
ParallelRobinHoodHashTable::find(KeyType key)
{
    // TODO
}

std::pair<size_t, bool>
ParallelRobinHoodHashTable::find_next_index_lock(ThreadManager &manager, size_t start_index, KeyType key, size_t &distance_key)
{
    // TODO
}

ThreadManager &
ParallelRobinHoodHashTable::get_thread_lock_manager()
{
    // TODO
}

std::pair<KeyType, ValueType>
ParallelRobinHoodHashTable::compare_and_set_key_val(size_t index, KeyType prev_key, KeyType new_key, ValueType new_val)
{
    // TODO
}

ParallelBucket &
ParallelRobinHoodHashTable::do_atomic_swap(ParallelBucket &swap_entry, size_t index)
{
    // TODO
}

std::pair<bool, bool>
ParallelRobinHoodHashTable::distance_zero_insert(KeyType key, ValueType value, size_t dist_zero_slot)
{
    // TODO
}

bool
ParallelRobinHoodHashTable::locked_insert(ParallelBucket &entry_to_insert, size_t swap_index)
{
    // TODO
    size_t swap_index = entry_to_insert.offset; //? i thnk its redoing it bc of possible contention but its locked so??
    while(true) {
        entry_to_insert = do_atomic_swap(entry_to_insert, swap_index);
        if(entry_to_insert.key.is_empty()) { //idk wtf? not sure this makes sense
            return true;
        }
        swap_index = find_next_index_lock(manager, swap_index, entry_to_insert.key, entry_to_insert.offset);
    }
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