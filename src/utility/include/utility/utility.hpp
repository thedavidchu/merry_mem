#pragma once
#include "common/types.hpp"
#include "common/logger.hpp"

////////////////////////////////////////////////////////////////////////////////
/// UTILITIES (static helper functions common to sequential and parallel implementations)
////////////////////////////////////////////////////////////////////////////////

HashCodeType
hash(const KeyType key);

size_t
get_home(const HashCodeType hashcode, const size_t capacity);
