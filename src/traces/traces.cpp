#include <cassert>
#include <cmath>
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
                       const unsigned insert_ratio,
                       const unsigned search_ratio,
                       const unsigned remove_ratio)
{
    LOG_TRACE("generate_random_traces() with ratio " << insert_ratio << ":" <<
            search_ratio << ":" << remove_ratio);
    std::vector<Trace> traces;
    traces.reserve(trace_length);
    foedus::assorted::ZipfianRandom zrng(max_num_unique_elements, /*theta=*/0.5, /*urnd_seed=*/0);
    foedus::assorted::UniformRandom urng(0);
    const unsigned sum_of_ratios = insert_ratio + search_ratio + remove_ratio;
    if (sum_of_ratios <= 0) {
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

std::vector<Trace>
generate_ordered_traces(const size_t max_num_unique_elements,
                        const size_t goal_trace_length,
                        const double insert_ratio,
                        const double search_ratio,
                        const double remove_ratio)
{
    LOG_INFO("generate_ordered_traces() with ratio " << insert_ratio << ":" <<
            search_ratio << ":" << remove_ratio);
    std::vector<Trace> traces;
    // NOTE Add 1 to the reserved space because if we as for 10 elements but
    //      have ratios {3.5,3.5,3}, then we would have {4,4,3} which adds to 11
    //      elements in total.
    traces.reserve(goal_trace_length + 1);
    foedus::assorted::ZipfianRandom zrng(max_num_unique_elements, /*theta=*/0.5, /*urnd_seed=*/0);
    const double sum_of_ratios = insert_ratio + search_ratio + remove_ratio;
    if (sum_of_ratios < 0) {
        assert(sum_of_ratios > 0 && "should be positive number");
        return traces;
    }
    // NOTE This will not necessarily produce the goal trace length.
    const size_t num_inserts = static_cast<size_t>(std::lround(insert_ratio / sum_of_ratios * static_cast<double>(goal_trace_length)));
    const size_t num_searches = static_cast<size_t>(std::lround(search_ratio / sum_of_ratios * static_cast<double>(goal_trace_length)));
    const size_t num_removes = static_cast<size_t>(std::lround(remove_ratio / sum_of_ratios * static_cast<double>(goal_trace_length)));
    LOG_INFO("Ratio of ops" << num_inserts << ":" << num_searches << ":" << num_removes);

    ValueType value = 0;
    for (size_t i = 0; i < num_inserts; ++i) {
        TraceOperator op = TraceOperator::insert;
        KeyType key = zrng.next();
        traces.emplace_back(op, key, value);
        ++value;
    }
    for (size_t i = 0; i < num_searches; ++i) {
        TraceOperator op = TraceOperator::search;
        KeyType key = zrng.next();
        traces.emplace_back(op, key, value);
        ++value;
    }
    for (size_t i = 0; i < num_removes; ++i) {
        TraceOperator op = TraceOperator::remove;
        KeyType key = zrng.next();
        traces.emplace_back(op, key, value);
        ++value;
    }
    return traces;
}
