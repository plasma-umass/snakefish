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
- `generator_exception.py`: Shows how to handle exceptions thrown by generators.
- `map.py`: Shows how to use `map()` and `starmap()`.
- `thread_exception.py`: Shows how to handle exceptions thrown by threads.
- `timestamp.py`: Shows how to timestamp messages to establish an ordering of events.

## Last Updated
2020-04-30 9f243fa3c88b955dbc5e3fb41e97937751b4e09e
