# SnakeFish

## Description
SnakeFish is an [opinionated](https://stackoverflow.com/questions/802050/what-is-opinionated-software) Python module providing primitives for true parallelism.

### Etymology
[Snake fish](https://en.wikipedia.org/wiki/Snakehead_%28fish%29) have "GILs" but can breathe air.

### Details
At a high level, SnakeFish avoids the Python interpreter's global interpreter lock (GIL) using processes, but it works differently than the official [multiprocessing](https://docs.python.org/3/library/multiprocessing.html) module.

For fork-join/map-reduce parallelism, SnakeFish relies on *merge functions* to synchronize processes upon exits. This simplifies concurrent programming in various ways:
- no deadlocks, race conditions, and false sharing
- updating shared states is possible
- reduced IPC costs, as the only copying happens during merging

Finally, since SnakeFish works independently of the Python interpreter, you may use JIT compilers like PyPy to obtain further speedups.

## How to Build
1. Clone the repo.
2. Run `make` in the repo root. This will install dependencies and build SnakeFish.
3. The built `.so` file can be found in `src`.

**NOTE**: Once the dependencies are installed, you can simply run `make` in `src` to rebuild. There's no need to reinstall the dependencies for every build.

## How to Use
1. Place the built `.so` file in a directory that's on your `PYTHONPATH`.
2. `import snakefish` in your Python code to import the extension module.

**NOTE**: You can find some examples [here](examples).

## How to View Documentation
1. Install [Doxygen](http://doxygen.nl/) and [Graphviz](https://graphviz.org/).
2. Run `doxygen Doxyfile` in the repo root.
3. Open `doc/html/index.html`.

**NOTE**: Doc is still WIP at the moment.

## Caveats
- [fork(2)](http://man7.org/linux/man-pages/man2/fork.2.html): "After a `fork()` in a multithreaded program, the child can safely call only async-signal-safe functions (see [signal-safety(7)](http://man7.org/linux/man-pages/man7/signal-safety.7.html)) until such time as it calls execve(2)." As such, users must ensure that their code, including its imported modules, either doesn't create threads or doesn't call non-async-signal-safe functions (e.g. `malloc()` and `printf()`).

## Development
See the [development documentation](dev_doc.md).

## Last Updated
2020-04-04 5ad2e495b0433a8b9ac28aae364ace987f806bf8
