#include "sequential.hpp"


int main(void) {
  LOG_TRACE("Enter");
  SequentialRobinHoodHashTable a;
  // Insert
  for (uint32_t i = 0; i < 10; ++i) {
    ErrorType e = a.insert(i, i);
    std::cout << "Insert (" << static_cast<int>(e) << "): <" << i << ", " << i << ">\n";
  }

  // Search
  for (uint32_t i = 0; i < 11; ++i) {
    std::optional<ValueType> value = a.search(i);
    if (value.has_value()) {
      std::cout << "Lookup (" << value.has_value() << ") " << i << ": " << value.value() << "\n";
    } else {
      std::cout << "Lookup (" << value.has_value() << ") " << i << ": ?\n";
    }
  }

  // Remove
  for (uint32_t i = 0; i < 11; ++i) {
    ErrorType e = a.remove(i);
    std::cout << "Remove (" << static_cast<int>(e) << "): " << i << std::endl;
    a.print();
  }
  std::cout << "Done!" << std::endl;
  return 0;
}
