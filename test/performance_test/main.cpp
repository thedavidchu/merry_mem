#include <cassert>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>

#include "common/logger.hpp"
#include "common/status.hpp"
#include "common/types.hpp"
#include "trace/trace.hpp"

#include "sequential/sequential.hpp"
#include "parallel/parallel.hpp"
#include "naive_parallel/naive_parallel.hpp"

#include "argument_parser.hpp"
#include "recorder.hpp"

double
run_sequential_performance_test(const std::vector<Trace> &traces)
{
    clock_t start_time, end_time;

    start_time = clock();
    SequentialRobinHoodHashTable hash_table;
    for (auto &t : traces) {
        switch (t.op) {
        case TraceOperator::insert: {
            hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            volatile std::optional<ValueType> r = hash_table.search(t.key);
            break;
        }
        case TraceOperator::remove: {
            hash_table.remove(t.key);
            break;
        }
        default: {
            assert(0 && "impossible!");
        }
        }
    }
    end_time = clock();
    double duration_in_seconds = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    std::cout << "Time in sec: " << duration_in_seconds << std::endl;
    return duration_in_seconds;
}

template<typename HashTable>
void
run_parallel_worker(HashTable &hash_table,
                    const std::vector<Trace> &traces, const size_t t_id,
                    const size_t num_workers)
{
    size_t trace_size = traces.size();

    size_t traces_per_thread = trace_size / num_workers;
    size_t traces_remaining = trace_size % num_workers;
    size_t start_index = t_id * traces_per_thread;
    size_t end_index = start_index + traces_per_thread;

    if (t_id == num_workers - 1)
    {
        end_index += traces_remaining;
    }

    for (size_t i = start_index; i < end_index; ++i) {
        const Trace &t = traces[i];
        switch (t.op) {
        case TraceOperator::insert: {
            hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            // NOTE Marking this as volatile means the compiler will not
            //      optimize this call out.
            volatile auto r = hash_table.search(t.key);
            break;
        }
        case TraceOperator::remove: {
            hash_table.remove(t.key);
            break;
        }
        default: {
            assert(false && "impossible!");
        }
        }
    }
}

template<typename HashTable>
double
run_parallel_performance_test(const std::vector<Trace> &traces, const size_t num_workers)
{
    std::vector<std::thread> workers;

    const auto start_time = std::chrono::steady_clock::now();
    HashTable hash_table;
    for (size_t i = 0; i < num_workers; ++i) {
        workers.emplace_back(run_parallel_worker<HashTable>, std::ref(hash_table), std::ref(traces), i, num_workers);
    }
    for (auto &w : workers) {
        w.join();
    }
    const auto end_time = std::chrono::steady_clock::now();
    double duration_in_seconds = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "Time in sec: " << duration_in_seconds << std::endl;
    return duration_in_seconds;
}

int main(int argc, char *argv[]) {
    PerformanceTestArguments args = parse_performance_test_arguments(argc, argv);
    args.print();

    std::vector<Trace> traces;
    if (args.trace_op_mode == "random") {
        traces = generate_random_traces(args.max_num_keys, args.goal_trace_length,
                args.insert_ratio, args.search_ratio, args.remove_ratio);
    } else if (args.trace_op_mode == "ordered") {
        traces = generate_ordered_traces(args.max_num_keys, args.goal_trace_length,
                args.insert_ratio, args.search_ratio, args.remove_ratio);
    }
    LOG_INFO("Finished generating traces");

    double seq_time_in_sec = run_sequential_performance_test(traces);
    LOG_INFO("Finished sequential test");

    std::vector<double> naive_parallel_time_in_sec;
    for (size_t w = 1; w <= 32; ++w) {
        double time = run_parallel_performance_test<NaiveParallelRobinHoodHashTable>(traces, w);
        naive_parallel_time_in_sec.push_back(time);
    }

    std::vector<double> parallel_time_in_sec;
    for (size_t w = 1; w <= 32; ++w) {
        double time = run_parallel_performance_test<ParallelRobinHoodHashTable>(traces, w);
        parallel_time_in_sec.push_back(time);
    }

    record_performance_test_times(args, seq_time_in_sec, naive_parallel_time_in_sec, parallel_time_in_sec);

    return 0;
}
