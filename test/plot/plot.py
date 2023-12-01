#!/usr/bin/python3

import itertools
import json
import subprocess
from typing import List, Tuple

import matplotlib.pyplot as plt


def run_performance_tests(
    modes: List[str] = ["random", "ordered"],
    ratios: List[Tuple[int, int, int]] = [(10, 80, 10), (33, 33, 33), (50, 0, 50)],
    max_num_keys: int = 10000,
    goal_trace_length: int = 100000,
    version: int = 0,               # TODO Change this if you have multiple runs
):
    for m, r in itertools.product(modes, ratios):
        print(f"Running '{m}' mode with ratios {r} keys {max_num_keys} length {goal_trace_length}")
        output_file = f"{m}-{r[0]}:{r[1]}:{r[2]}-n{max_num_keys}-t{goal_trace_length}-v{version}.json"
        # Assumes we are in the root of the project
        # NOTE We cannot just pass this list in as the commands because for some
        #      reason, it is not read correctly so the specified values are not
        #      used. It might be because the strings with the flags and arguments
        #      do not match just the flags?
        cmd = " ".join([
            "./build/test/performance_test/performance_test_exe",
            f"--ratio {r[0]} {r[1]} {r[2]}",
            f"--num-keys  {max_num_keys}",
            f"--trace-length {goal_trace_length}",
            f" --mode {m}",
            f"--output {output_file}",
        ])
        print(f"Running '{cmd}'")
        subprocess.run(cmd, shell=True)

    for m, r in itertools.product(modes, ratios):
        print(f"Plotting '{m}' mode with ratios {r} keys {max_num_keys} length {goal_trace_length}")
        output_file = f"{m}-{r[0]}:{r[1]}:{r[2]}-n{max_num_keys}-t{goal_trace_length}-v{version}.json"
        with open(output_file) as f:
            j = json.load(f)
        sequential_time = j["sequential"]
        parallel_times = j["parallel"]
        plot_performance(
            sequential_time_in_sec=sequential_time,
            parallel_num_workers=[x for x in range(1, 32 + 1)],
            parallel_time_in_sec=parallel_times,
            workload_name=f"{m} operators",
            insert_ratio=r[0],
            search_ratio=r[1],
            remove_ratio=r[2],
            max_num_keys=max_num_keys,
            goal_trace_length=goal_trace_length,
        )


def plot_performance(
    *,
    sequential_time_in_sec: float,
    parallel_num_workers: List[float],
    parallel_time_in_sec: List[float],
    workload_name: str,
    insert_ratio: int,
    search_ratio: int,
    remove_ratio: int,
    max_num_keys: int,
    goal_trace_length: int,
):
    """
    Plot the results of a performance test.
    """
    title = "\n".join([
        f"Performance Test for {workload_name}",
        f"with insert:search:remove ratio {insert_ratio}:{search_ratio}:{remove_ratio}",
        f"with {max_num_keys} keys and {goal_trace_length} operations",
    ])
    save_title = f"{workload_name}-{insert_ratio}:{search_ratio}:{remove_ratio}-n{max_num_keys}-t{goal_trace_length}"

    # Set up the plot
    plt.figure()
    plt.title(title)
    plt.xlabel("Number of Workers")
    plt.ylabel("Total Compuation Time [s]")

    # Plot data
    plt.axhline(y=sequential_time_in_sec, label="Sequential", color="tab:blue", linestyle="dashed")
    plt.plot(parallel_num_workers, parallel_time_in_sec, label="Parallel", c="tab:red", linestyle="solid")

    # Finish up plot and save
    plt.legend()
    plt.savefig(save_title)


def main():
    plot_performance(
        sequential_time_in_sec=10.0,
        parallel_num_workers=[x for x in range(1, 32 + 1)],
        parallel_time_in_sec=[x for x in range(1, 32 + 1)],
        workload_name="Ficticious Example",
        insert_ratio=1,
        search_ratio=1,
        remove_ratio=1,
        max_num_keys=0,
        goal_trace_length=0,
    )
    run_performance_tests()


if __name__ == "__main__":
    main()
