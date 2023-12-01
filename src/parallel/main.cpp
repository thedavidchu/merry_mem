#include "parallel/parallel.hpp"

int
main()
{
    LOG_TRACE("Enter");
    ParallelRobinHoodHashTable a;
    for (uint32_t i = 0; i < 10; ++i) {
        auto e = a.insert(i, i);
        std::cout << "Insert (" << static_cast<int>(e) << "): <" << i << ", " << i << ">\n";
    }
    return 0;
}
