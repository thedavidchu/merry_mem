#include <iostream>
#include <optional>

#include "parallel/parallel.hpp"


int main() {
  LOG_TRACE("Enter");
  ParallelRobinHoodHashTable a;
  // Insert
  for (uint64_t i = 0; i < 10; ++i) {
    ErrorType e = a.insert(i, i);
    std::cout << "Insert (" << static_cast<int>(e) << "): <" << i << ", " << i << ">\n";
  }

  // Search
  for (uint64_t i = 0; i < 11; ++i) {
    std::optional<ValueType> value = a.search(i);
    if (value.has_value()) {
      std::cout << "Lookup (" << value.has_value() << ") " << i << ": " << value.value() << "\n";
    } else {
      std::cout << "Lookup (" << value.has_value() << ") " << i << ": ?\n";
    }
  }

  // Remove
  for (uint64_t i = 0; i < 11; ++i) {
    ErrorType e = a.remove(i);
    std::cout << "Remove (" << static_cast<int>(e) << "): " << i << std::endl;
    a.print();
  }
  std::cout << "Done!" << std::endl;
  return 0;
}
