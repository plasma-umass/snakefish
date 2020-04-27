# snakefish Dev Doc

Below are some information for snakefish developers.

## Getting Started
1. Since the tests rely on pybind11's [interpreter embedding](https://pybind11.readthedocs.io/en/master/advanced/embedding.html), `CMake` is required for building the tests.
2. Developing in [CLion](https://www.jetbrains.com/clion/) is highly recommended but not required. Since it is centered around `CMake`, importing the project is very easy.

## Directory Structure
```
.
├── benchmark [scripts for benchmarking]
├── examples [Python scripts demonstrating usage]
│   └── multiprocessing [examples reimplemented with multiprocessing]
└── src [C++ source code]
    └── tests [C++ unit tests]
```

## How to View Documentation
1. Install [Doxygen](http://doxygen.nl/) and [Graphviz](https://graphviz.org/).
2. Run `doxygen Doxyfile` in the repo root.
3. Open `doc/html/index.html`.

## How to Build & Run Tests
1. Run `make dev_dependency` in the repo root.
2. Run `cmake -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -B cmake-build-debug` in the repo root.
3. Run `cmake --build cmake-build-debug --target all -- -j 4` in the repo root. You might want to adjust the argument for `-j` to change the level of concurrency.
4. Run `./cmake-build-debug/test` in the repo root.

**NOTE**: For now, tests only build with `clang`. See below in the [Issues/Caveats](#issuescaveats) section for an explanation.

**NOTE 2**: If you want to clean the build directory, run `cmake --build cmake-build-debug --target clean -- -j 4`. That alone doesn't purge CMake's cache, so sometimes you might need `rm -rf cmake-build-debug`.

## Design Decisions
- Shared memory is used for IPC. [Unnamed semaphores](http://man7.org/linux/man-pages/man7/sem_overview.7.html) are used to implement blocking/non-blocking `receive()`. Since unnamed semaphores are not implemented on macOS ([ref 1](https://stackoverflow.com/q/27736618), [ref 2](https://stackoverflow.com/q/1413785)), named semaphores are used there instead.
- Since growing shared memory after `fork()` is difficult ([ref 1](https://stackoverflow.com/q/16423789), [ref 2](https://stackoverflow.com/q/49266193)), a 2 GiB chunk of shared memory is allocated by each `channel` to avoid resizing. The allocation is done through `mmap()` with `MAP_NORESERVE`, so we don't actually use 2 GiB of memory right away.
- Some atomic variables are shared between processes. Such usage should be safe as long as the shared variables are lock-free because lock-free atomics are also address-free ([ref 1](https://stackoverflow.com/a/51463590), [ref 2](https://stackoverflow.com/a/19937333)).
- Regarding the implementation of `get_timestamp_serialized()`, see [ref 1](https://www.felixcloutier.com/x86/rdtsc), [ref 2](https://stackoverflow.com/a/13772771), [ref 3](https://stackoverflow.com/a/12634857), and [ref 4](https://stackoverflow.com/a/28307254).

## Discussion Points
- Better way to IPC objects than `pickle`? Currently, `dumps()` is used to serialize Python objects into `bytes`, which can be converted into byte buffers (`void *`) accessible in C++ ([ref 1](https://docs.python.org/3.8/c-api/memoryview.html), [ref 2](https://docs.python.org/3.8/c-api/buffer.html#buffer-structure)). Deserialization is done in a similar way using `loads()`. This means sending an Python object to another process takes at least 4 copying.
- As mentioned above, snakefish currently over-allocates shared memory to avoid resizing, which is not ideal. If resizing ever needs to be implemented, one possibility is to use `ftruncate()` + `munmap()` + `mmap()` ([ref](https://stackoverflow.com/q/49266193)). We might also want to reference [Boost's implementation](https://github.com/boostorg/interprocess/tree/develop/include/boost/interprocess).

## Issues/Caveats
- Building the tests with `gcc` will produce 2 "undefined reference" errors. This is weird because the symbols are there if you inspect with `objdump`. This is probably due to some issue with `gcc`'s linking order ([ref 1](https://stackoverflow.com/q/16574113), [ref 2](https://stackoverflow.com/q/31286905)).
- `pybind11` will only export instantiated versions of template functions/classes to the produced dynamic library ([ref](https://github.com/pybind/pybind11/issues/199)). This *seems* to affect not just the exposed interface but also internal code. For example, if you define a template function to be called only in your C++ code, a missing symbol error for that function would be generated at load time.

## Roadmap

## Last Updated
2020-04-27 b39b606ae85f964e2d48baa2dd463657e9d2f949
