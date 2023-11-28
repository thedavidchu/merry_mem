#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include <time.h>

#include "zipfian_random.hpp"

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"

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
    print()
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
generate_traces(size_t num_elem, size_t trace_length)
{
    std::vector<Trace> traces;
    traces.reserve(num_elem);
    foedus::assorted::ZipfianRandom zrng(num_elem, /*theta=*/0.5, /*urnd_seed=*/0);
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
run_sequential_performance_test(std::vector<Trace> &traces)
{
    clock_t start_time, end_time;

    start_time = clock();
    SequentialRobinHoodHashTable seq_hash_table;
    for (auto &t : traces) {
        switch (t.op) {
        case TraceOperator::insert: {
            seq_hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            volatile std::optional<ValueType> r = seq_hash_table.search(t.key);
            break;
        }
        case TraceOperator::remove: {
            seq_hash_table.remove(t.key);
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

int main() {
    std::vector<Trace> traces = generate_traces(100000, 100000000);
    run_sequential_performance_test(traces);

    return 0;
}
