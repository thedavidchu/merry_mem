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
generate_random_traces(const size_t max_num_unique_elements,
                       const size_t trace_length,
                       const double insert_ratio,
                       const double search_ratio,
                       const double remove_ratio)
{
    LOG_TRACE("generate_random_traces() with ratio " << insert_ratio << ":" <<
            search_ratio << ":" << remove_ratio);
    std::vector<Trace> traces;
    traces.reserve(trace_length);
    foedus::assorted::ZipfianRandom zrng(num_unique_elements, /*theta=*/0.5, /*urnd_seed=*/0);
    foedus::assorted::UniformRandom urng(0);
    const unsigned sum_of_ratios = insert_ratio + search_ratio + remove_ratio;
    if (sum_of_ratios < 0) {
        assert(sum_of_ratios > 0 && "should be positive number");
        return traces;
    }

    TraceOperator op = TraceOperator::insert;
    KeyType key = 0;
    ValueType value = 0;
    for (size_t i = 0; i < trace_length; ++i) {
        uint64_t op_prob = urng.uniform_within(1, sum_of_ratios);
        assert(1 <= op_prob && op_prob <= sum_of_ratios &&
                "the op_prob should be in interval [1, sum_of_ratios]");
        if (op_prob <= insert_ratio) {
            op = TraceOperator::insert;
        } else if (op_prob <= insert_ratio + search_ratio) {
            op = TraceOperator::search;
        } else {
            op = TraceOperator::remove;
        }
        key = zrng.next();
        traces.emplace_back(op, key, value);
        ++value;
    }
    return traces;
}
