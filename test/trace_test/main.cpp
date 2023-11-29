#include <array>
#include <cassert>
#include <iostream>
#include <vector>

#include "../../src/traces/traces.hpp"

/// Print out two traces
int main() {
    constexpr std::size_t trace_count = 10;
    std::vector<Trace> traces;
    std::array<Trace, trace_count> random_oracle = {
        {TraceOperator::insert, 2, 0},
        {TraceOperator::insert, 0, 1},
        {TraceOperator::remove, 2, 2},
        {TraceOperator::remove, 2, 3},
        {TraceOperator::search, 2, 4},
        {TraceOperator::insert, 0, 5},
        {TraceOperator::remove, 1, 6},
        {TraceOperator::insert, 1, 7},
        {TraceOperator::remove, 0, 8},
        {TraceOperator::remove, 1, 9},
    };
    std::array<Trace, trace_count> ordered_oracle = {
        {TraceOperator::insert, 2, 0},
        {TraceOperator::insert, 0, 1},
        {TraceOperator::insert, 2, 2},
        {TraceOperator::search, 2, 3},
        {TraceOperator::search, 2, 4},
        {TraceOperator::search, 0, 5},
        {TraceOperator::search, 1, 6},
        {TraceOperator::search, 1, 7},
        {TraceOperator::remove, 0, 8},
        {TraceOperator::remove, 1, 9},
        {TraceOperator::remove, 0, 10},
    };

    std::cout << "=== Start trace test ===\n";
    std::cout << "--- Random test ---\n";
    traces = generate_random_traces(3, 10);
    assert(traces.size() == random_oracle.size() && "sizes should match");
    for (size_t i = 0; i < traces.size(); ++i) {
        assert(traces[i] == random_oracle[i] && "traces should match");
    }
    std::cout << "\t--- SUCCESS ---\n";
    std::cout << "--- Ordered test ---\n";
    traces = generate_ordered_traces(3, 10, 25.0, 50.0, 25.0);
    assert(traces.size() == ordered_oracle.size() && "sizes should match");
    for (size_t i = 0; i < traces.size(); ++i) {
        assert(traces[i] == ordered_oracle[i] && "traces should match");
    }
    std::cout << "\t--- SUCCESS ---\n";
}
