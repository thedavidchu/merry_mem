#!/usr/bin/python3

from typing import List

import matplotlib.pyplot as plt


def run_performance_test():
    pass


def plot_performance(
    *,
    sequential_time_in_sec: float,
    parallel_num_workers: List[float],
    parallel_time_in_sec: List[float],
    workload_name: str,
    insert_ratio: int,
    search_ratio: int,
    remove_ratio: int,
):
    """
    Plot the results of a performance test.
    """
    title = f"Performance Test for {workload_name}\nwith insert:search:remove ratio {insert_ratio}:{search_ratio}:{remove_ratio}"
    save_title = f"{workload_name}-{insert_ratio}:{search_ratio}:{remove_ratio}"

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
    )


if __name__ == "__main__":
    main()
