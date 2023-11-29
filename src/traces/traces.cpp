#include <cassert>
#include <iostream>
#include <vector>

#include "uniform_random.hpp"
#include "zipfian_random.hpp"

#include "traces.hpp"

void
Trace::print() const
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

std::vector<Trace>
generate_random_traces(const size_t num_unique_elements, const size_t trace_length)
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
