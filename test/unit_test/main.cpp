#include "trace/trace.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>
#include "sequential/sequential.hpp"
#include "parallel/parallel.hpp"
#include <unordered_set>

#include "common/types.hpp"


std::unordered_map<KeyType, ValueType> map;
SequentialRobinHoodHashTable sequential_hash_table;
ParallelRobinHoodHashTable parallel_hash_table;


//run trace on unordered map for baseline comparison
void 
test_traces_on_unordered_map(const std::vector<Trace>& traces) 
{
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert:
                map[trace.key] = trace.value;
                break;
            case TraceOperator::search:
                map.find(trace.key);
                break;
            case TraceOperator::remove:
                map.erase(trace.key);
                break;
        }
    }
}

//run trace on sequential hash table for comparison 
void 
test_traces_on_sequential(const std::vector<Trace>& traces) 
{
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert:
                sequential_hash_table.insert((trace.key), (trace.value));
                break;
            case TraceOperator::search:
                sequential_hash_table.search(trace.key);
                break;
            case TraceOperator::remove:
                sequential_hash_table.remove(trace.key);
                break;
        }
    }
}

//run trace on parallel hash table for comparison 
void 
test_traces_on_parallel(const std::vector<Trace>& traces) 
{
    for (const auto& trace : traces) {
        switch (trace.op) {
            case TraceOperator::insert:
                parallel_hash_table.insert((trace.key), (trace.value));
                break;
            case TraceOperator::search:
                parallel_hash_table.find(trace.key);
                break;
            case TraceOperator::remove:
                parallel_hash_table.remove(trace.key);
                break;
        }
    }
}


int 
main(int argc, char** argv) 
{

    //generate random traces 
    std::vector<Trace> traces = generate_random_traces(100, 1000); 
    
    //run traces on all three hash tables 
    test_traces_on_unordered_map(traces);
    test_traces_on_sequential(traces);
    test_traces_on_parallel(traces);

    bool equal = true;

    //check if all values in unordered map are in sequential hash table
    for (auto& element: map) 
    {
        auto actual = sequential_hash_table.search(element.first);

        if(actual.value() != element.second) 
        {
            std::cout << "**************************" << std::endl;
            std::cout << "********FAILURE**********" << std::endl;
            std::cout << "********SEQUENTIAL*******" << std::endl;
            std::cout << "Search failed for key " << element.first << std::endl;
            std::cout << "**************************" << std::endl;
            equal = false;
        }

    }

    //check if all values in unordered map are in parallel hash table
    for (auto& element: map) 
    {
        auto actual = parallel_hash_table.find(element.first);

        if(actual.first != element.second) 
        {
            std::cout << "**************************" << std::endl;
            std::cout << "********FAILURE**********" << std::endl;
            std::cout << "********PARALLEL*******" << std::endl;
            std::cout << "Search failed for key " << element.first << std::endl;
            std::cout << "**************************" << std::endl;
            equal = false;
        }

    }
    
    //if all values in unordered map are in both hash tables, then they are equal
    if(equal)
    {
        std::cout << "**************************" << std::endl;
        std::cout << "*********SUCCESS**********" << std::endl;
        std::cout << "****Hash tables equal*****" << std::endl;
        std::cout << "**************************" << std::endl;
    }

    return 0;
}


