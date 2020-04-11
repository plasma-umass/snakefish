# Benchmarks

## Usage
1. Run `make` in the repo root.
2. Run `make` here (`benchmark/`).
3. Benchmark individual scripts OR use `bench.py` to benchmark multiple scripts.

**NOTE**: There's no need to repeat the first 2 steps every time unless there are new commits.

## Directory Structure
- `benchmarksgame`: Original scripts from [The Benchmarks Game](https://benchmarksgame-team.pages.debian.net/benchmarksgame/). There might be slight modifications to change output locations.
- `multiprocessing`: Scripts parallelized using `multiprocessing` and some wrappers, which can be found at `examples/multiprocessing/wrappers.py`.
- `sequential`: Scripts with no parallelism.
- `snakefish`: Scripts parallelized using `snakefish`.

## Benchmark Scripts
- `binary_tree.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/binarytrees-python3-1.html]() (accessed 2020-04-04).
- `fannkuch.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/fannkuchredux-python3-4.html]() (accessed 2020-04-04).
- `fasta.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/fasta-python3-5.html]() (accessed 2020-04-04).
- `knucleotide.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/knucleotide-python3-3.html]() (accessed 2020-04-04).
- `mandelbrot.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/mandelbrot-python3-7.html]() (accessed 2020-04-04).
- `regexredux.py`: Adapted from [https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/regexredux-python3-1.html]() (accessed 2020-04-04).

## Helper Scripts

### `analysis.R`
R code for plots & analysis.

### `bench.py`
This runs multiple scripts using `hyperfine` and outputs the stats in a JSON file.

Usage: python3 bench.py <bench_target> <program_path> [skip]

- <bench_target>: benchmark target
- <program_path>: where the benchmark programs are located
- [skip]: files to skip, separated by `,`

Example: python3 bench.py 'multiprocessing' ./multiprocessing/ 'wrappers.py'

**NOTE**: [hyperfine](https://github.com/sharkdp/hyperfine) is required for `bench.py`

### `dummy.py`
This does nothing. It's used by `bench.py` to get the startup overhead of the Python interpreter. The data may then be used as a baseline in analysis.

## Last Updated
2020-04-11 ac6ab75c9d30cde9d9e34fc5194d16b978e4676a
