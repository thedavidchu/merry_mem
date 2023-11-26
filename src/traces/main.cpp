#include <iostream>
#include <random>
#include <vector>

#include <time.h>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"

#include "../parallel/parallel.hpp"


#include <pthread.h>
#include <iostream>
#include <optional> // std::optional i think is c++17 



// structure to pass data to threads
struct OperationData {
    enum OperationType {
        INSERT,
        SEARCH,
        REMOVE
    };

    OperationType operation;
    uint64_t key;
};

// Define the structure to pass data to threads
struct ThreadData {
    ParallelRobinHoodHashTable* hashTable;
    OperationData* operations;
    size_t numOperations;
};




void
run_sequential_perf_test()
{
    constexpr size_t num_elem = 1000000;
    SequentialRobinHoodHashTable a;

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.insert(kv, kv);
    }

    for (unsigned i = 0; i < 100; ++i) {
        for (uint64_t kv = 0; kv < num_elem; ++kv) {
            volatile std::optional<ValueType> r = a.search(kv);
        }
    }

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.remove(kv);
    }
}




void* threadFunction(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);

    for (size_t i = 0; i < data->numOperations; ++i) {
        const auto& op = data->operations[i];
        switch (op.operation) {
            case OperationData::INSERT:
                data->hashTable->insert(op.key, op.key);
                break;
            case OperationData::SEARCH:
                volatile std::optional<ValueType> r = data->hashTable->search(op.key);
                break;
            case OperationData::REMOVE:
                data->hashTable->remove(op.key);
                break;
        }
    }

    pthread_exit(nullptr);
}



void run_parallel_perf_test() {
    constexpr size_t num_elem = 1000000;
    constexpr unsigned num_threads = 6; // You can adjust the number of threads

    ParallelRobinHoodHashTable a;

    // Create an array of OperationData for each thread
    OperationData operations[num_elem * 10]; // Assuming an average of 2 operations per element

    // Populate the operations array with a random sequence of operations
    for (size_t i = 0; i < num_elem * 10; ++i) {
        operations[i].operation = static_cast<OperationData::OperationType>(i % 3);
        operations[i].key = i % num_elem;
    }

    // Create thread data and threads
    pthread_t threads[num_threads];
    ThreadData threadData[num_threads];

    size_t operations_per_thread = num_elem * 10 / num_threads;

    // Launch threads
    for (unsigned i = 0; i < num_threads; ++i) {
        threadData[i].hashTable = &a;
        threadData[i].operations = operations + i * operations_per_thread;
        threadData[i].numOperations = operations_per_thread;

        pthread_create(&threads[i], nullptr, threadFunction, &threadData[i]);
    }

    // Wait for threads to finish
    for (unsigned i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }
}



//operation 
//key val
// this array is the trace, now this call funcation that spawns threads that does on the trace

// 1. read trace
// 2. spawn threads
// 3. each thread does the operation

int main() {
    clock_t start_seq, end_seq;
    start_seq = clock();
    run_sequential_perf_test();
    end_seq = clock();
    std::cout << "Sequential: time in sec: " << ((double)(end_seq - start_seq)) / CLOCKS_PER_SEC << std::endl;

    clock_t start_par, end_par;
    start_par = clock();
    run_parallel_perf_test();
    end_par = clock();
    std::cout << "Parallel: time in sec: " << ((double)(end_par - start_par)) / CLOCKS_PER_SEC << std::endl;

    return 0;
}