# snakefish

## Description
snakefish is an [opinionated](https://stackoverflow.com/questions/802050/what-is-opinionated-software) Python module providing primitives for true parallelism.

### Etymology
[Snake fish](https://en.wikipedia.org/wiki/Snakehead_%28fish%29) have "GILs" but can breathe air.

### Details
At a high level, snakefish avoids the Python interpreter's global interpreter lock (GIL) using processes, but it works differently than the official [multiprocessing](https://docs.python.org/3/library/multiprocessing.html) module.

With multiprocessing, data may be shared between processes through either [IPC channels](https://docs.python.org/3/library/multiprocessing.html#exchanging-objects-between-processes) or [shared states](https://docs.python.org/3/library/multiprocessing.html#sharing-state-between-processes). This makes shared states explicit, and is very different from multithreading.

On the other hand, snakefish tries to provide a threading semantics. Users simply need to define *extraction functions* and *merge functions*, and snakefish will handle the rest. The former specify which global variables are shared, and the latter specify how different versions of global variables should be merged.

Since *merge functions* are used to synchronize processes upon exits, concurrent programming is simplified in various ways:
- no deadlocks, race conditions, and false sharing
- updating shared states is possible
- reduced IPC costs, as the only copying happens during merging

This scheme works best with fork-join parallelism, but it should work in many other use cases as well, such as pipeline-based parallelism.

snakefish doesn't provide things like synchronization primitives, but that shouldn't be a problem because one can intermix multiprocessing and snakefish, as long as the [`fork` start method](https://docs.python.org/3/library/multiprocessing.html#contexts-and-start-methods) is used.

Finally, since snakefish works independently of the Python interpreter, you may use JIT compilers like PyPy to obtain further speedups.

## How to Build
1. Clone the repo.
2. Run `make` in the repo root. This will download dependencies and build snakefish.
3. The built `.so` file can be found in `src`.

**NOTE**: Once the dependencies are downloaded, you can simply run `make` in `src` to rebuild. There's no need to re-download the dependencies for every build.

## How to Use
1. Place the built `.so` file in a directory that's on your `PYTHONPATH`.
2. `import snakefish` in your Python code to import the extension module.

**NOTE**: You can find some examples [here](examples).

## API

### `Channel`
An IPC channel with built-in synchronization support.

All messages are exchanged through the same buffer, so the communication is not multiplexed. As such, `channel` should only be used for uni-directional communication in most cases. Having multiple senders and/or multiple receivers may or may not work, depending on the use cases.

**IMPORTANT**: The `dispose()` function must be called when a channel is no longer needed to release resources.

#### `Channel() -> obj`
Create a channel with default buffer size.

#### `Channel(size: int) -> obj`
Create a channel with buffer size `size`. The buffer is allocated with `MAP_NORESERVE`.

#### `send_pyobj(obj) -> None`
Send a Python object. This function will serialize `obj` using `pickle` and send the binary output.

Throws:
- `OverflowError`: If the underlying buffer does not have enough space to accommodate the request.
- `RuntimeError`: If some semaphore error occurred.

#### `receive_pyobj(block: bool) -> obj`
Receive a Python object. This function will receive some bytes and deserialize them using `pickle`. This function may or may not block, depending on the value of `block`.

Throws
- `IndexError`: If the underlying buffer does not have enough content to accommodate the request (this only applies when `block` is `false`).
- `RuntimeError`: If some semaphore error occurred.
- `MemoryError`: If `malloc()` failed.

#### `dispose() -> None`
Release resources held by this channel.

### `Generator`
A class for executing Python generators with true parallelism.

**IMPORTANT**: The `dispose()` function must be called when a generator is no longer needed to release resources.

#### `Generator(f) -> obj`
Create a new snakefish generator with no global variable merging.

Params:
- `f`: The Python function this generator will execute. It must be a [generator function](https://wiki.python.org/moin/Generators).

Throws:
- `RuntimeError`: If `f` is not a generator function.

#### `Generator(f, extract, merge) -> obj`
Create a new snakefish generator with global variable merging.

Params:
- `f`: The Python function this generator will execute. It must be a [generator function](https://wiki.python.org/moin/Generators).
- `extract`: The globals extraction function this generator will execute. Its signature should be `(dict) -> dict`. It should take the child's `globals()` and return a dict of globals that must be kept. The returned dict will be passed to `merge` as the second parameter. Note that anything contained in the returned dict must be [picklable](https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
- `merge`: The merge function this generator will execute. Its signature should be `(dict, dict) -> nil`. The first dict is the parent's `globals()`, and the second dict is the one returned by `extract()`. The merge function must merge the two by updating the first dict.

Note that `extract()` is executed by the child process, and `merge` is executed by the parent process. The return value of `extract()` is sent to the parent through IPC.

Throws:
- `RuntimeError`: If `f` is not a generator function.

#### `start() -> None`
Start executing this generator.

Throws:
- `RuntimeError`: If this generator has already been started OR if `fork()` failed.

#### `next(block: bool) -> obj`
Get the next output of the generator. This function may or may not block, depending on the value of `block`.

Throws:
- `IndexError`: If the next output isn't ready yet (only applies when `block` is `false`).
- `next()` will rethrow any exception thrown by the generator.

#### `join() -> None`
Join this generator. This will block the caller until this generator terminates.

Throws:
- `RuntimeError`: If this generator hasn't been started yet OR if `waitpid()` failed.

#### `try_join() -> bool`
Try to join this generator. This is the non-blocking version of `join()`. Returns `true` if joined and `false` otherwise.

Throws:
- `RuntimeError`: If this generator hasn't been started yet OR if `waitpid()` failed.

#### `get_exit_status() -> int`
Get the exit status of the generator. If the generator was terminated by signal `N`, `-N` would be returned.

Throws:
- `RuntimeError`: If the generator hasn't been started yet OR if the generator hasn't been joined yet.

#### `dispose() -> None`
Release resources held by this generator.

### `Thread`
A class for executing Python functions with true parallelism.

**IMPORTANT**: The `dispose()` function must be called when a thread is no longer needed to release resources.

#### `Thread(f) -> obj`
Create a new snakefish thread with no global variable merging.

Params:
- `f`: The Python function this thread will execute.

#### `Thread(f, extract, merge) -> obj`
Create a new snakefish thread with global variable merging.

Params:
- `f`: The Python function this thread will execute.
- `extract`: The globals extraction function this thread will execute. Its signature should be `(dict) -> dict`. It should take the child's `globals()` and return a dict of globals that must be kept. The returned dict will be passed to `merge` as the second parameter. Note that anything contained in the returned dict must be [picklable](https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
- `merge`: The merge function this thread will execute. Its signature should be `(dict, dict) -> nil`. The first dict is the parent's `globals()`, and the second dict is the one returned by `extract()`. The merge function must merge the two by updating the first dict.

Note that `extract()` is executed by the child process, and `merge` is executed by the parent process. The return value of `extract()` is sent to the parent through IPC.

#### `start() -> None`
Start executing this thread. In other words, start executing the underlying function.

Throws:
- `RuntimeError`: If this thread has already been started OR if `fork()` failed.

#### `join() -> None`
Join this thread. This will block the caller until this thread terminates.

Throws:
- `RuntimeError`: If this thread hasn't been started yet OR if `waitpid()` failed.

#### `try_join() -> bool`
Try to join this thread. This is the non-blocking version of `join()`. Returns `true` if joined and `false` otherwise.

Throws:
- `RuntimeError`: If this thread hasn't been started yet OR if `waitpid()` failed.

#### `is_alive() -> bool`
Get the status of the thread. Returns `true` if this thread has been started and has not yet terminated; `false` otherwise.

#### `get_exit_status() -> int`
Get the exit status of the thread. If the thread was terminated by signal `N`, `-N` would be returned.

Throws:
- `RuntimeError`: If the thread hasn't been started yet OR if the thread hasn't been joined yet.

#### `get_result() -> obj`
Get the output of the thread (i.e. the output of the underlying function).

Throws:
- `RuntimeError`: If the thread hasn't been started yet OR if the thread hasn't been joined yet.
- `get_result()` will rethrow any exception thrown by the thread.

#### `dispose() -> None`
Release resources held by this thread.

### Standalone Functions

#### `get_timestamp() -> int`
Get a high resolution timestamp obtained from [`rdtsc`](https://www.felixcloutier.com/x86/rdtsc). This may be used to establish an ordering of IPC messages.

#### `get_timestamp_serialized() -> int`
Like `get_timestamp()`, but with `lfence` and compiler fence applied. For most use cases, this is probably not needed, and `get_timestamp()` would be sufficient.

#### `map(f, args, concurrency=0, chunksize=0) -> list`
`map(f, args)` executed in parallel, with no global variable merging. Results are returned in a list.

Params
- `f`: The Python function that should be applied to each argument.
- `args`: The arguments as a Python iterable.
- `concurrency`: The level of concurrency. If not supplied, this is set to the number of cores in the system.
- `chunksize`: The size of each process' job. If not supplied, `args` are handed out evenly to each process.

#### `map(f, args, extract, merge, concurrency=0, chunksize=0) -> list`
`map(f, args)` executed in parallel, with global variable merging. Results are returned in a list.

Params
- `f`: The Python function that should be applied to each argument.
- `args`: The arguments as a Python iterable.
- `extract`: See `Thread` constructor.
- `merge`: See `Thread` constructor.
- `concurrency`: The level of concurrency. If not supplied, this is set to the number of cores in the system.
- `chunksize`: The size of each process' job. If not supplied, `args` are handed out evenly to each process.

#### `starmap(f, args, concurrency=0, chunksize=0) -> list`
`starmap(f, args)` executed in parallel, with no global variable merging. Results are returned in a list.

Params
- `f`: The Python function that should be applied to each argument (after unpacking).
- `args`: The arguments as a Python iterable.
- `concurrency`: The level of concurrency. If not supplied, this is set to the number of cores in the system.
- `chunksize`: The size of each process' job. If not supplied, `args` are handed out evenly to each process.

#### `starmap(f, args, extract, merge, concurrency=0, chunksize=0) -> list`
`starmap(f, args)` executed in parallel, with global variable merging. Results are returned in a list.

Params
- `f`: The Python function that should be applied to each argument (after unpacking).
- `args`: The arguments as a Python iterable.
- `extract`: See `Thread` constructor.
- `merge`: See `Thread` constructor.
- `concurrency`: The level of concurrency. If not supplied, this is set to the number of cores in the system.
- `chunksize`: The size of each process' job. If not supplied, `args` are handed out evenly to each process.

## Caveats
- [fork(2)](http://man7.org/linux/man-pages/man2/fork.2.html): "After a `fork()` in a multithreaded program, the child can safely call only async-signal-safe functions (see [signal-safety(7)](http://man7.org/linux/man-pages/man7/signal-safety.7.html)) until such time as it calls execve(2)." As such, users must ensure that their code, including its imported modules, either doesn't create threads or doesn't call non-async-signal-safe functions (e.g. `malloc()` and `printf()`).

## Development
See the [development documentation](dev_doc.md).

## Last Updated
2020-04-30 84491cf3d72e2b9a9fd0b6e5c416006f405df357
