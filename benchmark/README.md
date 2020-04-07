# Benchmark Scripts

## `analysis.R`
R code for plots & analysis.

## `bench.py`
Usage: python3 bench.py <bench_target> <program_path> [skip]

- <bench_target>: benchmark target
- <program_path>: where the benchmark programs are located
- [skip]: files to skip, separated by [,]

Example: python3 bench.py 'multiprocessing' ../examples/multiprocessing/ 'wrappers.py'

**NOTE**: [hyperfine](https://github.com/sharkdp/hyperfine) is required for `bench.py`

## `dummy.py`
This does nothing. It's used by `bench.py` to get the startup overhead of the Python interpreter. The data may then be used as a baseline in analysis.
