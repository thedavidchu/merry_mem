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

void test_traces_on_parallel(const std::vector<Trace>& traces) {
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




void compare_hash_tables(const ParallelBucket& hashTable, const std::vector<Trace>& traces) {
    // Implementation for comparing the final state of the hash table
}






int main(int argc, char** argv) {

    //numbers can be changed to test different cases
    //possibly change to allow for args to funcaiton call for parameters 
    std::vector<Trace> traces = generate_random_traces(100, 1000); 

    test_traces_on_unordered_map(traces);
    test_traces_on_sequential(traces);
    test_traces_on_parallel(traces);

    return 0;
}


