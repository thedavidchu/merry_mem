#include <iostream>
#include <random>
#include <vector>

#include <time.h>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"

#include "../parallel/parallel.hpp"

#include <iostream>
#include <optional> 

#include <thread>


#define NUM_OPS 2000000 //2 mill ops per trace element 

#define NUM_ELEM 1000000 

// structure to pass data to threads
struct OperationData {
    enum OperationType { insert, search, remove };
    OperationType operation;
    KeyType key;
    ValueType value;
};

// Define the structure to pass data to threads
struct ThreadData {
    ParallelRobinHoodHashTable *hashTable;
    OperationData *operations;
    size_t numOperations;
    uint64_t threadID;
};


// Function to convert OperationType to string for better output
const char* operationToString(OperationData::OperationType op) {
    switch (op) {
        case OperationData::insert: {
            return "insert";
        }
        case OperationData::search: {
            return "search";
        }
        case OperationData::remove: {
            return "remove";
        }
    }
    return "unknown";
}

/*
void
run_sequential_perf_test(std::vector<OperationData> &trace)
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
*/

void 
run_sequential_perf_test(const std::vector<OperationData>& trace) 
{
    SequentialRobinHoodHashTable a;

    // Assuming that you need to insert, search, and remove operations for each element in the trace
    for (const auto& op : trace) {
        switch (op.operation) {
            case OperationData::OperationType::insert:{
                a.insert(op.key, op.value);
                break;
            }
            case OperationData::OperationType::search: {
                // Perform search operation
                volatile std::optional<ValueType> r = a.search(op.key);
                break;
            }
            case OperationData::OperationType::remove: {
                a.remove(op.key);
                break;
            }
            default: {
                std::cerr << "Unknown operation: " << operationToString(op.operation) << std::endl;
                break;
            }
        }
    }
}


void *
threadFunction(void *arg)
{
    ThreadData *data = static_cast<ThreadData *>(arg);

    for (size_t i = 0; i < data->numOperations; ++i) {
        const auto &op = data->operations[i];
        switch (op.operation) {
        case OperationData::insert: {
            data->hashTable->insert(op.key, op.value);
            break;
        }
        case OperationData::search: {
            std::pair<ValueType, bool> res = data->hashTable->find(op.key);
            break;
        }
        case OperationData::remove: {
            data->hashTable->remove(op.key, op.value);
            break;
        }
        default: {
            std::cerr << "Unknown operation: " << operationToString(op.operation) << std::endl;
            break;
        }
        }
    }
    pthread_exit(nullptr);
}


void 
generate_trace(std::vector<OperationData> &trace)
{
    // Populate the operations array with a random sequence of operations
    for (size_t i = 0; i < NUM_OPS; ++i) {
        trace[i].operation = static_cast<OperationData::OperationType>(i % 3);
        trace[i].key = i % NUM_ELEM;
        trace[i].value = i % NUM_ELEM + 1; //just so different
    }
}


void
run_parallel_perf_test(std::vector<OperationData> &trace, uint64_t num_threads)
{
    ParallelRobinHoodHashTable a;

    // Create thread data and threads
    std::vector<std::thread> threads;
    std::vector<ThreadData> threadData(num_threads);

    size_t operations_per_thread = trace.size() / num_threads;

    // Launch threads
    for (uint64_t i = 0; i < num_threads; ++i) {
        threadData[i].hashTable = &a;
        threadData[i].operations = &trace[i * operations_per_thread];
        threadData[i].numOperations = operations_per_thread;
        threadData[i].threadID = i;
        threads.emplace_back(threadFunction, &threadData[i]);
    }

    // Wait for threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}


int
main() 
{

    std::vector<OperationData> trace(NUM_ELEM); //not sure if this is num elemtns * num ops or just num ops
    generate_trace(trace);

    clock_t start, end;
    start = clock();
    run_sequential_perf_test(trace);
    end = clock();
    std::cout << "Sequential: time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC
              << std::endl;

    start = clock();
    run_parallel_perf_test(trace, 1);
    end = clock();
    std::cout << "Parallel 1: time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC
              << std::endl;


    start = clock();
    run_parallel_perf_test(trace, 8);
    end = clock();
    std::cout << "Parallel 8: time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC
              << std::endl;


    start = clock();
    run_parallel_perf_test(trace, 16);
    end = clock();
    std::cout << "Parallel 16: time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC
              << std::endl;

    return 0;
}
