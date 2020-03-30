# SnakeFish Examples

## Usage
1. Run `make` in the repo root.
2. Run `make` here (`/examples`).
3. Run an example Python script (e.g. `python3 fork_tryjoin.py`).

**NOTE**: There's no need to repeat the first 2 steps every time you run an example unless there are new commits.

## Listing
- `channel.py`: Shows how threads can communicate through a channel.
- `fork_join.py`: Shows how to spawn a thread and join it (i.e. blocking join).
- `fork_tryjoin.py`: Shows how to spawn a thread and try-join it (i.e. non-blocking join).
- `generator.py`: Shows how to spawn a generator, how to get the generated values (blocking or non-blocking), and how to try-join it.
- `generator_exception.py`: Show how to handle exceptions thrown by generators.
- `thread_exception.py`: Shows how to handle exceptions thrown by threads.

## Last Updated
2020-03-29 ea73d8e0e15f0ab54c923667bff197f1c1d3cbd8
