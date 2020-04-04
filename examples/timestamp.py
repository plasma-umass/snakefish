import random
import time
from typing import *  # use type hints to make signatures clear

import snakefish

channel = snakefish.Channel()  # create a channel


# a function that will be executed on a snakefish thread
def f1() -> None:
    if random.random() < 0.5:
        print("thread #1: sleep for 100 ms")
        time.sleep(0.1)

    ts = snakefish.get_timestamp_serialized()
    print("thread #1: sending an event...")
    channel.send_pyobj((ts, "explosion"))


# a function that will be executed on a snakefish thread
def f2() -> None:
    ts = snakefish.get_timestamp_serialized()
    print("thread #2: sending an event...")
    channel.send_pyobj((ts, "implosion"))


# a function that will be executed on a snakefish thread
def f3() -> None:
    # blocking receive
    print("thread #3: receiving an object...")
    received1 = channel.receive_pyobj(True)

    # blocking receive
    print("thread #3: receiving an object...")
    received2 = channel.receive_pyobj(True)

    print("thread #3: timestamps are %s and %s" % (received1[0], received2[0]))
    if received1[0] <= received2[0]:
        print("thread #3: %s occurred before %s" % (received1[1], received2[1]))
    else:
        print("thread #3: %s occurred before %s" % (received2[1], received1[1]))


# the function that will extract shared global variables
def extract(_globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {}  # no-op


# the function that will merge shared global variables
def merge(_old_globals: Dict[str, Any], _new_globals: Dict[str, Any]) -> None:
    pass  # no-op


# spawn 3 snakefish threads
t1 = snakefish.Thread(f1, extract, merge)
t2 = snakefish.Thread(f2, extract, merge)
t3 = snakefish.Thread(f3, extract, merge)

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
