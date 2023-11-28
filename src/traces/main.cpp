#include <cassert>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include <time.h>

#include "zipfian_random.hpp"

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"
#include "../parallel/parallel.hpp"

enum class TraceOperator {
    insert,
    search,
    remove,
};

struct Trace {
    TraceOperator op;
    KeyType key;
    ValueType value;

    void
    print() const
    {
        switch (this->op) {
        case TraceOperator::insert:
            std::cout << "insert(" << this->key << ", " << this->value << ")\n";
            break;
        case TraceOperator::search:
            std::cout << "search(" << this->key << ", " << this->value << ")\n";
            break;
        case TraceOperator::remove:
            std::cout << "remove(" << this->key << ", " << this->value << ")\n";
            break;
        default:
            assert(0 && "impossible!");
        }
    }
};

std::vector<Trace>
generate_traces(const size_t num_unique_elements, const size_t trace_length)
{
    std::vector<Trace> traces;
    traces.reserve(trace_length);
    foedus::assorted::ZipfianRandom zrng(num_unique_elements, /*theta=*/0.5, /*urnd_seed=*/0);
    foedus::assorted::UniformRandom urng(0);

    TraceOperator op = TraceOperator::insert;
    KeyType key = 0;
    ValueType value = 0;
    for (size_t i = 0; i < trace_length; ++i) {
        uint64_t raw_op = urng.uniform_within(1, 3);
        switch (raw_op) {
        case 1:
            op = TraceOperator::insert;
            break;
        case 2:
            op = TraceOperator::search;
            break;
        case 3:
            op = TraceOperator::remove;
            break;
        default:
            assert(0 && "impossible!");
        }
        key = zrng.next();
        traces.emplace_back(op, key, value);
        ++value;
    }
    return traces;
}

void
run_sequential_performance_test(const std::vector<Trace> &traces)
{
    clock_t start_time, end_time;

    start_time = clock();
    SequentialRobinHoodHashTable hash_table;
    for (auto &t : traces) {
        switch (t.op) {
        case TraceOperator::insert: {
            hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            volatile std::optional<ValueType> r = hash_table.search(t.key);
            break;
        }
        case TraceOperator::remove: {
            hash_table.remove(t.key);
            break;
        }
        default: {
            assert(0 && "impossible!");
        }
        }
    }
    end_time = clock();
    double duration_in_seconds = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    std::cout << "Time in sec: " << duration_in_seconds << std::endl;
}

void
run_parallel_worker(ParallelRobinHoodHashTable &hash_table,
                    const std::vector<Trace> &traces, const size_t t_id,
                    const size_t num_workers)
{
    size_t trace_size = traces.size();

    for (size_t i = t_id; i < trace_size; i += num_workers) {
        Trace &t = traces[i];
        switch (t.op) {
        case TraceOperator::insert: {
            hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            volatile std::optional<ValueType> r = hash_table.search(t.key);
            break;
        }
        case TraceOperator::remove: {
            hash_table.remove(t.key);
            break;
        }
        default: {
            assert(0 && "impossible!");
        }
        }
    }
}

void
run_parallel_performance_test(const std::vector<Trace> &traces, const size_t num_workers)
{
    clock_t start_time, end_time;
    std::vector<std::thread> workers;

    start_time = clock();
    ParallelRobinHoodHashTable hash_table;
    for (size_t i = 0; i < num_workers; ++i) {
        workers.emplace_back(run_parallel_worker, hash_table, traces, i, num_workers);
    }
    for (auto &w : workers) {
        w.join();
    }
    end_time = clock();
    double duration_in_seconds = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    std::cout << "Time in sec: " << duration_in_seconds << std::endl;
}

int main() {
    std::vector<Trace> traces = generate_traces(100000, 100000000);
    run_sequential_performance_test(traces);
    run_parallel_performance_test(traces, 1);
    run_parallel_performance_test(traces, 2);
    run_parallel_performance_test(traces, 4);
    run_parallel_performance_test(traces, 8);
    run_parallel_performance_test(traces, 16);
    run_parallel_performance_test(traces, 32);

    return 0;
}
