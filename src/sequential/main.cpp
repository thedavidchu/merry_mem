#include "sequential.hpp"


int main(int argc, char *argv[]) {
  LOG_TRACE("Enter");
  SequentialRobinHoodHashTable a = SequentialRobinHoodHashTable();
  // Insert
  for (int32_t i = 0; i < 10; ++i) {
    int r = a.insert(i, i);
    std::cout << "Insert (" << r << "): <" << i << ", " << i << ">\n";
  }

  // Search
  for (int32_t i = 0; i < 11; ++i) {
    int32_t value = -1;
    int r = a.search(i, &value);
    std::cout << "Lookup (" << r << ") " << i << ": " << value << "\n";
  }

  // Remove
  for (int32_t i = 0; i < 11; ++i) {
    int r = a.remove(i);
    std::cout << "Remove (" << r << "): " << i << std::endl;
    a.print();
  }
  std::cout << "Done!" << std::endl;
  return 0;
}
