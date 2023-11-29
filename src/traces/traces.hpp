#pragma once

#include <vector>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

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
    print() const;
};

std::vector<Trace>
generate_random_traces(const size_t num_unique_elements, const size_t trace_length);
