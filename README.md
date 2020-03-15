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
2. `import csnakefish` in your Python code to import the extension module.

## How to View Documentation
1. Install [Doxygen](http://doxygen.nl/) and [Graphviz](https://graphviz.org/).
2. Run `doxygen Doxyfile` in the repo root.
3. Open `doc/html/index.html`.

**NOTE**: Doc is still WIP at the moment.

## Development
See the [development documentation](dev_doc.md).

## Last Updated
2020-03-14 ff5b3b2ce0aa4d224fc1fae5747c3d4b9a53e602
