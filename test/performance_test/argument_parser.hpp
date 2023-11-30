#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
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
