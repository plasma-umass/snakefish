# SnakeFish Dev Doc

Below are some information for SnakeFish developers.

## Getting Started
1. Since the tests rely on pybind11's [interpreter embedding](https://pybind11.readthedocs.io/en/master/advanced/embedding.html), `CMake` is required for building the tests.
2. Developing in [CLion](https://www.jetbrains.com/clion/) is highly recommended but not required. Since it is centered around `CMake`, importing the project is very easy.

## Directory Structure
```
.
├── benchmark [scripts for benchmarking]
├── examples [Python scripts explaining usage]
    └── multiprocessing [examples reimplemented with multiprocessing]
└── src [C++ source code]
    └── tests [C++ unit tests]
```

## How to Build & Run Tests
1. Run `make dev_dependency` in the repo root.
2. Run `cmake -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -B cmake-build-debug` in the repo root.
3. Run `cmake --build cmake-build-debug --target all -- -j 4` in the repo root. You might want to adjust the argument for `-j` to change the level of concurrency.
4. Run `./cmake-build-debug/test` in the repo root.

**NOTE**: For now, tests only build with `clang`. See below in the [Issues/Caveats](#issuescaveats) section for an explanation.

**NOTE 2**: If you want to clean the build directory, run `cmake --build cmake-build-debug --target clean -- -j 4`. That alone doesn't purge CMake's cache, so sometimes you might need `rm -rf cmake-build-debug`

## Design Decisions
- Shared memory is used for IPC. [Unnamed semaphores](http://man7.org/linux/man-pages/man7/sem_overview.7.html) are used to implement blocking/non-blocking `receive()`. Since unnamed semaphores are not implemented on macOS ([ref 1](https://stackoverflow.com/q/27736618), [ref 2](https://stackoverflow.com/q/1413785)), named semaphores are used there instead.
- Some atomic variables are shared between processes. Such usage should be safe as long as the shared variables are lock-free because lock-free atomics are also address-free ([ref 1](https://stackoverflow.com/a/51463590), [ref 2](https://stackoverflow.com/a/19937333)).
- Regarding the implementation of `get_timestamp_serialized()`, see [ref 1](https://www.felixcloutier.com/x86/rdtsc), [ref 2](https://stackoverflow.com/a/13772771), [ref 3](https://stackoverflow.com/a/12634857), and [ref 4](https://stackoverflow.com/a/28307254).
- Since manual memory management is not needed in Python, it would be strange if SnakeFish needs it. The solution is to make memory management automatic on the C++ side, but it's not straightforward given that processes and shared memory are involved. The current implementation is as follows. For classes that use shared memory (e.g. `channel`), reference counting is performed in c'tors/d'tors. This only gives semi-automatic RC because users of these classes must call a `fork()` member function on each object before calling the system `fork()`. Wrappers around system `fork()` and `exit()`--`snakefish_fork()` and `snakefish_exit()`--are provided to make things fully automatic. These wrappers, together with the relevant c'tors/d'tors, keep track of the instantiated objects and call `fork()` or d'tor on them. Note that d'tors won't be called when calling `exit()`, so we have to manually invoke them in such cases.

## Discussion Points
- Better way to IPC objects than `pickle`? Currently, `dumps()` is used to serialize Python objects into `bytes`, which can be converted into byte buffers (`void *`) accessible in C++ ([ref 1](https://docs.python.org/3.8/c-api/memoryview.html), [ref 2](https://docs.python.org/3.8/c-api/buffer.html#buffer-structure)). Deserialization is done in a similar way using `loads()`. This means sending an Python object to another process takes at least 4 copying.
- The RC implementation described above is conservative. Since we don't know whether certain objects will be used by certain processes, we'll have to assume that they will. This can be an issue for applications that spawn SnakeFish threads in a loop (e.g. a web server). In such cases, resources will not be released until all threads have exited, and that might be never.

## Issues/Caveats
- Building the tests with `gcc` will produce 2 "undefined reference" errors. This is weird because the symbols are there if you inspect with `objdump`. This is probably due to some issue with `gcc`'s linking order ([ref 1](https://stackoverflow.com/q/16574113), [ref 2](https://stackoverflow.com/q/31286905)).
- `pybind11` will only export instantiated versions of template functions/classes to the produced dynamic library ([ref](https://github.com/pybind/pybind11/issues/199)). This *seems* to affect not just the exposed interface but also internal code. For example, if you define a template function to be called only in your C++ code, a missing symbol error for that function would be generated at load time.

## Roadmap
- benchmarks & performance measurements

## Last Updated
2020-04-06 b2f6442d8a542ab6c0faffb6cd796548d2ee2b8f
