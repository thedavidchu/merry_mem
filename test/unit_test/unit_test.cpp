#include "traces.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include "sequential.hpp"
#include "parallel.hpp"

std::unordered_map<KeyType, ValueType> map;
SequentialRobinHoodHashTable sequential_hash_table;
parallelRobinHoodHashTable parallel_hash_table;


void 
test_traces_on_unordered_map(const std::vector<Trace>& traces) 
{
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = map.emplace({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                    //result.first->second = trace.value; // Update the value
                }
                break;
            }
            case TraceOperator::search:
                if (map.find(trace.key) == map.end()) {
                    std::cout << "Search: Key " << trace.key << " not found." << std::endl;
                } else {
                    std::cout << "Search: Key " << trace.key << " found with value " << map[trace.key] << std::endl;
                }
                break;
            case TraceOperator::remove:
                if (map.erase(trace.key)) {
                    std::cout << "Remove: Key " << trace.key << " removed." << std::endl;
                } else {
                    std::cout << "Remove: Key " << trace.key << " not found, nothing to remove." << std::endl;
                }
                break;
        }
    }
}


void 
test_traces_on_sequential(const std::vector<Trace>& traces) 
{
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = sequential_hash_table.insert({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                }
                break;
            }
            case TraceOperator::search:
                if (sequential_hash_table.search(trace.key) == sequential_hash_table.end()) {
                    std::cout << "Search: Key " << trace.key << " not found." << std::endl;
                } else {
                    std::cout << "Search: Key " << trace.key << " found with value " << sequential_hash_table[trace.key] << std::endl;
                }
                break;
            case TraceOperator::remove:
                if (sequential_hash_table.remove(trace.key)) {
                    std::cout << "Remove: Key " << trace.key << " removed." << std::endl;
                } else {
                    std::cout << "Remove: Key " << trace.key << " not found, nothing to remove." << std::endl;
                }
                break;
        }
    }
}


void 
test_traces_on_parallel(const std::vector<Trace>& traces) {
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = parallel_hash_table.insert({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                }
                break;
            }
            case TraceOperator::search:
                if (parallel_hash_table.find(trace.key) == parallel_hash_table.end()) {
                    std::cout << "Search: Key " << trace.key << " not found." << std::endl;
                } else {
                    std::cout << "Search: Key " << trace.key << " found with value " << parallel_hash_table[trace.key] << std::endl;
                }
                break;
            case TraceOperator::remove:
                if (parallel_hash_table.remove(trace.key)) {
                    std::cout << "Remove: Key " << trace.key << " removed." << std::endl;
                } else {
                    std::cout << "Remove: Key " << trace.key << " not found, nothing to remove." << std::endl;
                }
                break;
        }
    }
}



bool
compare_hash_tables(const SequentialRobinHoodHashTable& sequentialTable,
                         const parallelRobinHoodHashTable& parallelTable,
                         const std::unordered_map<KeyType, ValueType>& map) {

    //add to unordered sets as order does not matter in them
    std::unordered_set<ValueType> sequentialSet(sequentialTable.begin(), sequentialTable.end());
    std::unordered_set<ValueType> parallelSet(parallelTable.begin(), parallelTable.end());
    std::unordered_set<ValueType> unorderedMapSet(unorderedMap.begin(), unorderedMap.end());

    //compare the unordered sets to determine if they have the same elements
    bool areEqualSequential = (sequentialSet == unorderedMapSet);
    bool areEqualParallel = (parallelSet == unorderedMapSet);

    //print out results
    if(areEqualSequential && areEqualParallel) {
        std::cout << "Sequential and parallel hash tables are equal to the unordered map" << std::endl;
        return 1;
    } else if(areEqualSequential) {
        std::cout << "Sequential hash table is equal to the unordered map" << std::endl;
        return 0;
    } else if(areEqualParallel) {
        std::cout << "Parallel hash table is equal to the unordered map" << std::endl;
        return 0;
    } else {
        std::cout << "Sequential and parallel hash tables are not equal to the unordered map" << std::endl;
        return 0;
        std::cout << "BOTH UNEQUAL" << std::endl;
    }
}


int main(int argc, char** argv) {

    //numbers can be changed to test different cases
    //possibly change to allow for args to funcaiton call for parameters 
    std::vector<Trace> traces = generate_random_traces(100, 1000); 

    test_traces_on_unordered_map(traces);
    test_traces_on_sequential(traces);
    test_traces_on_parallel(traces);

    bool check = compare_hash_tables(sequential_hash_table, parallel_hash_table, map);

    if(check) {
        std::cout << "**************************" << std::endl;
        std::cout << "********SUCCESS**********" << std::endl;
        std::cout << "Hash tables are equal" << std::endl;
        std::cout << "**************************" << std::endl;
    } else {
        std::cout << "**************************" << std::endl;
        std::cout << "********FAILURE**********" << std::endl;
        std::cout << "Hash tables are not equal" << std::endl;
        std::cout << "**************************" << std::endl;
    }


    return 0;
}


