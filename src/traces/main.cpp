#include <iostream>
#include <random>
#include <vector>

#include <time.h>

#include "../common/logger.hpp"
#include "../common/status.hpp"
#include "../common/types.hpp"

#include "../sequential/sequential.hpp"

void
run_sequential_perf_test()
{
    constexpr size_t num_elem = 1000000;
    SequentialRobinHoodHashTable a;

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.insert(kv, kv);
    }

    for (unsigned i = 0; i < 100; ++i) {
        for (uint64_t kv = 0; kv < num_elem; ++kv) {
            volatile std::optional<ValueType> r = a.search(kv);
        }
    }

    for (uint64_t kv = 0; kv < num_elem; ++kv) {
        a.remove(kv);
    }
}

int main() {
    clock_t start, end;
    start = clock();
    run_sequential_perf_test();
    end = clock();
    std::cout << "Time in sec: " << ((double)(end - start)) / CLOCKS_PER_SEC << std::endl;

    return 0;
}