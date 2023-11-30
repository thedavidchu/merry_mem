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

////////////////////////////////////////////////////////////////////////////////
/// ARGUMENT PARSER
////////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <iostream>

struct PerformanceTestArguments {
    size_t insert_ratio = 1;
    size_t search_ratio = 1;
    size_t remove_ratio = 1;
    std::string trace_op_mode = "random";
    std::string output_json_path = "output.json";

    void
    print() const
    {
        std::cout << "Mode: '" << this->trace_op_mode << "', Ratios: " <<
                this->insert_ratio << ":" << this->search_ratio << ":" << this->remove_ratio <<
                ", Output: " << this->output_json_path << std::endl;
    }
};

static void
print_help_and_exit(PerformanceTestArguments &args)
{
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "You asked for help!" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "-r, --ratio <num> <num> <num> : the ratio or insert:search:remove operations. [Default " << args.insert_ratio << ":" << args.search_ratio << ":" << args.remove_ratio << "]" << std::endl;
    std::cout << "-m, --mode <mode> : trace generator mode {random,ordered} for the trace operations. [Default '" << args.trace_op_mode << "']" << std::endl;
    std::cout << "                    N.B. The option is just the raw string 'random' or 'ordered' without the quotation marks!" << std::endl;
    std::cout << "-o, --output <output-path> : path for the output JSON file relative to cwd. [Default '" << args.output_json_path << "']" << std::endl;
    std::cout << "-h, --help : print this help message. This overrides all other arguments!" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    exit(1);
}

static bool
matches_argument_flag(char *input_flag, std::string short_flag, std::string long_flag)
{
    return std::strcmp(input_flag, short_flag.c_str()) == 0 ||
            std::strcmp(input_flag, long_flag.c_str()) == 0;
}

PerformanceTestArguments
parse_performance_test_arguments(int argc, char *argv[])
{
    PerformanceTestArguments args;

    // Check for help! Do this first because asking for help will override
    // all other directives.
    for (size_t i = 0; i < static_cast<size_t>(argc); ++i) {
        if (matches_argument_flag(argv[i], "-h", "--help")) {
            print_help_and_exit(args);
        }
    }

    // Check for regular variables
    for (argv = &argv[1]; *argv != nullptr; ++argv) {
        if (matches_argument_flag(*argv, "-r", "--ratio")) {
            // NOTE There is no atoi equivalent for unsigned ints.
            ++argv;
            args.insert_ratio = std::strtoul(*argv, nullptr, 10);
            ++argv;
            args.search_ratio = std::strtoul(*argv, nullptr, 10);
            ++argv;
            args.remove_ratio = std::strtoul(*argv, nullptr, 10);
        } else if (matches_argument_flag(*argv, "-m", "--mode")) {
            ++argv;
            args.trace_op_mode = std::string(*argv);
            assert((args.trace_op_mode == "random" || args.trace_op_mode == "ordered") &&
                    "mode should be {random,ordered}");
        } else if (matches_argument_flag(*argv, "-o", "--output")) {
            ++argv;
            args.output_json_path = std::string(*argv);
        } else {
            // NOTE We create a new default argument structure because we
            //      potentially already modified the other structure.
            PerformanceTestArguments default_args;
            print_help_and_exit(default_args);
        }
    }

    return args;
}

#include <fstream>
#include <iostream>
#include <vector>

void
record_performance_test_times(const PerformanceTestArguments &args,
                              const double seq_time_sec,
                              const std::vector<double> & par_time_sec)
{
    // Open file
    std::ofstream ostrm(args.output_json_path);
    if (!ostrm.is_open()) {
        std::cerr << "Cannot open " << args.output_json_path << std::endl;
        return;
    }

    ostrm << "{";
    ostrm << "\"sequential\": " << seq_time_sec << ",";
    ostrm << "\"parallel\": [";
    for (size_t i = 0; i < par_time_sec.size(); ++i) {
        ostrm << par_time_sec[i];
        // NOTE JSON does not allow trailing commas at the end of arrays, so
        //      skip the last element.
        if (i != par_time_sec.size() - 1) {
            ostrm << ", ";
        }
    }
    ostrm << "]";
    ostrm << "}\n";
    ostrm.close();
}

////////////////////////////////////////////////////////////////////////////////
/// TRACE RUNNER
////////////////////////////////////////////////////////////////////////////////

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
    std::vector<Trace> traces = generate_random_traces(100000, 100000000);
    double seq_time_in_sec = run_sequential_performance_test(traces);
    std::vector<double> parallel_time_in_sec;

    for (size_t w = 1; w <= 32; ++w) {
        double time = run_parallel_performance_test(traces, w);
        parallel_time_in_sec.push_back(time);
    }

    record_performance_test_times(args, seq_time_in_sec, parallel_time_in_sec);

    return 0;
}
