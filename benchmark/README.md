# Benchmark Scripts

## `bench.py`
Usage: python3 bench.py <bench_target> <program_path> [skip]

- <bench_target>: benchmark target
- <program_path>: where the benchmark programs are located
- [skip]: files to skip, separated by [,]

Example: python3 bench.py 'multiprocessing' ../examples/multiprocessing/ 'wrappers.py'

**NOTE**: [hyperfine](https://github.com/sharkdp/hyperfine) is required for `bench.py`
