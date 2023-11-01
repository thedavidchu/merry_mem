#include "sequential.hpp"


int main(int argc, char *argv[]) {
  LOG_TRACE("Enter");
  SequentialRobinHoodHashTable a = SequentialRobinHoodHashTable();
  for (int32_t i = 0; i < 10; ++i) {
    std::cout << "Insert <" << i << ", " << i << ">\n";
    a.insert(i, i);
    a.print();
  }
  std::cout << "Done!" << std::endl;
  return 0;
}
