#pragma once

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
