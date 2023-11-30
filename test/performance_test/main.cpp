#include <cassert>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include <ctime>

#include "common/logger.hpp"
#include "common/status.hpp"
#include "common/types.hpp"
#include "trace/trace.hpp"

#include "sequential/sequential.hpp"
#include "parallel/parallel.hpp"

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
    return ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
}

void
run_parallel_worker(ParallelRobinHoodHashTable &hash_table,
                    const std::vector<Trace> &traces, const size_t t_id,
                    const size_t num_workers)
{
    size_t trace_size = traces.size();

    for (size_t i = t_id; i < trace_size; i += num_workers) {
        const Trace &t = traces[i];
        switch (t.op) {
        case TraceOperator::insert: {
            hash_table.insert(t.key, t.value);
            break;
        }
        case TraceOperator::search: {
            // NOTE Marking this as volatile means the compiler will not
            //      optimize this call out.
            volatile std::pair<long unsigned int, bool> r = hash_table.find(t.key);
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

void
run_parallel_performance_test(const std::vector<Trace> &traces, const size_t num_workers)
{
    clock_t start_time, end_time;
    std::vector<std::thread> workers;

    start_time = clock();
    ParallelRobinHoodHashTable hash_table;
    for (size_t i = 0; i < num_workers; ++i) {
        workers.emplace_back(run_parallel_worker, std::ref(hash_table), std::ref(traces), i, num_workers);
    }
    for (auto &w : workers) {
        w.join();
    }
    end_time = clock();
    double duration_in_seconds = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    std::cout << "Time in sec: " << duration_in_seconds << std::endl;
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

    std::vector<double> parallel_time_in_sec;
    for (size_t w = 1; w <= 32; ++w) {
        double time = run_parallel_performance_test(traces, w);
        parallel_time_in_sec.push_back(time);
    }

    record_performance_test_times(args, seq_time_in_sec, parallel_time_in_sec);

    return 0;
}
