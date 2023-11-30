[![cpp-linter](https://github.com/cpp-linter/cpp-linter-action/actions/workflows/cpp-linter.yml/badge.svg)](https://github.com/cpp-linter/cpp-linter-action/actions/workflows/cpp-linter.yml)

# merry_mem
This project includes a sequential and parallel version of the Robin Hood hash
table.

## Project Structure

The project is structured as follows:

```
|--src/
|  |--common/           : Common utilities (types and a logger)
|  |--parallel/         : Parallel implementation (library and simple sanity
|  |                      check executable)
|  |--sequential/       : Sequential implementation (library and simple sanity
|  |                      check executable)
|  `--traces/           : Code for generating traces to test our implementations
`--test/
   |--performance_test/ : Benchmark the sequential vs the parallel parallel
   |                      implementations with different traces and different
   |                      numbers of workers
   |--trace_test/       : Test the trace generating library
   `--unit_test/        : Unit tests for our sequential and parallel
                          implementations
```

## Build

We use the CMake buildsystem (version 3.18.4) that is available on the UG
machines. After ensuring you have CMake installed, build the CMake project:

```bash
# In the project's root directory
mkdir -p build
cd build
cmake -S .. -B .
```

This builds the required `Makefile` that allows you to build the project.

```bash
# In the build directory
make
```

To see the individual targets that can be made, run:

```bash
# In the build directory
make help
```

In general, executables are named `*_exe`.

## Test

After building the project, there are three types of tests:

1. Performance tests
2. Trace tests
3. Unit tests

Performance tests measure the timing of the sequential and parallel
implementations when running various traces.

```bash
#In the build directory
./test/performance_test/performance_test_exe
```

Trace tests ensure the trace generator produces traces in the expected format.

```bash
# In the build directory
./test/trace_test/trace_test_exe
```

Unit tests ensure that the sequential and parallel implementations match the
results of the same trace being run on `std::unordered_map`. To run the unit
tests, run:

```bash
# In the build directory
./test/unit_test/unit_test_exe
```

## Performance

After building the performance test, run it:

```bash
#In the build directory
./test/performance_test/performance_test_exe
```
