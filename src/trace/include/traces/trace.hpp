#pragma once

#include <vector>

#include "common/logger.hpp"
#include "common/status.hpp"
#include "common/types.hpp"
#include "utilities/utilities.hpp"

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

    constexpr bool operator==(const Trace& rhs) const = default;
};

/// @brief  Create a trace of elements following the Zipfian distribution with
///         a uniformly random distribution of insert, search, and remove
///         operations.
std::vector<Trace>
generate_random_traces(const size_t max_num_unique_elements,
                       const size_t trace_length,
                       const unsigned insert_ratio = 33,
                       const unsigned search_ratio = 33,
                       const unsigned remove_ratio = 33);

/// @brief  Create a trace of elements following the Zipfian distribution with
///         the operations being ordered: insert, search, remove.
/// @param  goal_trace_length: size_t
///             This is the attempted trace length. However, if the ratios do
///             not sum nicely, we not necessarily have that exact trace length.
/// @param  {insert,search,remove}_ratio: double = 33.0.
///             This is the relative ratio of the {} operation.
std::vector<Trace>
generate_ordered_traces(const size_t max_num_unique_elements,
                        const size_t goal_trace_length,
                        const double insert_ratio = 33.0,
                        const double search_ratio = 33.0,
                        const double remove_ratio = 33.0);
