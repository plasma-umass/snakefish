import time
from typing import *  # use type hints to make signatures clear

import csnakefish

channel = csnakefish.Channel()  # create a channel


# a function that will be executed on a snakefish thread
def f1() -> None:
    print("thread #1: sending an object...")
    channel.send_pyobj([42])

    print("thread #1: sleep for 500 ms")
    time.sleep(0.5)
    print("thread #1: sending an object...")
    channel.send_pyobj({'1': 1})


# a function that will be executed on a snakefish thread
def f2() -> None:
    print("thread #2: sending an object...")
    channel.send_pyobj((42,))


# a function that will be executed on a snakefish thread
def f3() -> None:
    # blocking receive
    print("thread #3: receiving an object...")
    received = channel.receive_pyobj(True)
    print("thread #3: received", received)

    # blocking receive
    print("thread #3: receiving an object...")
    received = channel.receive_pyobj(True)
    print("thread #3: received", received)

    # non-blocking receive
    print("thread #3: receiving an object...")
    start = time.time()
    while True:
        try:
            received = channel.receive_pyobj(False)
            break
        except IndexError:
            print("thread #3: received nothing, sleep for 100 ms")
            time.sleep(0.1)
    print("thread #3: received", received)
    print("thread #3: receiving took %s sec" % (time.time() - start))


# the function that will extract shared global variables
def extract(_globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {}  # no-op


# the function that will merge shared global variables
def merge(_old_globals: Dict[str, Any], _new_globals: Dict[str, Any]) -> None:
    pass  # no-op


# spawn 3 snakefish threads
channel.fork()
t1 = csnakefish.Thread(f1, extract, merge)

channel.fork()
t2 = csnakefish.Thread(f2, extract, merge)

channel.fork()
t3 = csnakefish.Thread(f3, extract, merge)

t1.start()
t2.start()
t3.start()

# join the threads and check exit status
t1.join()
assert (t1.get_exit_status() == 0)
print("thread #1 exit status:", t1.get_exit_status())

t2.join()
assert (t2.get_exit_status() == 0)
print("thread #2 exit status:", t2.get_exit_status())

t3.join()
assert (t3.get_exit_status() == 0)
print("thread #3 exit status:", t3.get_exit_status())
