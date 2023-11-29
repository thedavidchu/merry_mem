#include "traces.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include "sequential.hpp"
#include "parallel.hpp"

std::unordered_map<KeyType, ValueType> map;

void test_traces_on_unordered_map(const std::vector<Trace>& traces) {

    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = map.emplace({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                    result.first->second = trace.value; // Update the value
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


void test_traces_on_sequential(const std::vector<Trace>& traces) {
    SequentialBucket hashTable(); // Create an instance of your robin hood hash table

    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = hashTable.insert({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                    result.first->second = trace.value; // Update the value
                }
                break;
            }
            case TraceOperator::search:
                if (hashTable.search(trace.key) == hashTable.end()) {
                    std::cout << "Search: Key " << trace.key << " not found." << std::endl;
                } else {
                    std::cout << "Search: Key " << trace.key << " found with value " << hashTable[trace.key] << std::endl;
                }
                break;
            case TraceOperator::remove:
                if (hashTable.remove(trace.key);) {
                    std::cout << "Remove: Key " << trace.key << " removed." << std::endl;
                } else {
                    std::cout << "Remove: Key " << trace.key << " not found, nothing to remove." << std::endl;
                }
                break;
        }
    }
}

void test_traces_on_parallel(const std::vector<Trace>& traces) {
    ParallelBucket hashTable();

    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert: {
                auto result = hashTable.insert({trace.key, trace.value});
                if (result.second) {
                    std::cout << "Insert: Key " << trace.key << " inserted with value " << trace.value << std::endl;
                } else {
                    std::cout << "Insert: Key " << trace.key << " already exists, value updated to " << trace.value << std::endl;
                    result.first->second = trace.value; // Update the value
                }
                break;
            }
            case TraceOperator::search:
                if (hashTable.find(trace.key) == hashTable.end()) {
                    std::cout << "Search: Key " << trace.key << " not found." << std::endl;
                } else {
                    std::cout << "Search: Key " << trace.key << " found with value " << hashTable[trace.key] << std::endl;
                }
                break;
            case TraceOperator::remove:
                if (hashTable.remove(trace.key);) {
                    std::cout << "Remove: Key " << trace.key << " removed." << std::endl;
                } else {
                    std::cout << "Remove: Key " << trace.key << " not found, nothing to remove." << std::endl;
                }
                break;
        }
    }
}






