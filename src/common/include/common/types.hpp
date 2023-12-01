#pragma once
#include <cstdint>  // uint64_t
#include <cstddef>
#include <limits>

////////////////////////////////////////////////////////////////////////////////
/// TYPE DEFINITIONS (for easy refactor)
////////////////////////////////////////////////////////////////////////////////

/// N.B.  I use uint64_t for the key and value so that they can hold a general
///       pointer value.
/// N.B.  I do not use *_t because this is a reserved name in POSIX.
/// N.B.  I also named them something unique so it's easy to find/replace.
using KeyType = uint32_t;
using ValueType = uint32_t;
/// N.B.  I use 'HashCode' because I want to distinguish 'hash' (verb) and
///       'hash' (noun). Thus, I use 'hash code' to denote the latter.
using HashCodeType = uint32_t;
using OffsetType = uint32_t;

constexpr OffsetType offset_invalid = std::numeric_limits<OffsetType>::max();
