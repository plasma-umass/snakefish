import queue
import random
import time
from typing import *  # use type hints to make signatures clear

import wrappers

a = 1  # global variable 'a'


# the function that will be executed on a generator
def f() -> float:
    global a
    a = [42]

    while True:
        print("f() will sleep for 100 ms")
        time.sleep(0.1)
        yield random.random()


# the function that will extract shared global variables
def extract(globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {'a': globals_dict['a']}  # extract a


# the function that will merge shared global variables
def merge(old_globals: Dict[str, Any], new_globals: Dict[str, Any]) -> None:
    old_globals.update(new_globals)  # simply overwrite the old globals


# 'a' before update
print("global 'a' was", a)
print()

# spawn a generator
g = wrappers.Generator(target=f, extract=extract, merge=merge, shared_vars=globals())
g.start()

# blocking receive
for i in range(3):
    print("next :", g.next(True))
print()

# non-blocking receive
for i in range(3):
    while True:
        try:
            print("next:", g.next(False))
            break
        except queue.Empty:
            print("received nothing, sleep for 100 ms")
            time.sleep(0.1)
print()

# try to join the generator
while True:
    g.join(0)
    if g.get_exit_status() is not None:
        print("join() successful")
        break
    else:
        print("join() unsuccessful, sleep for 100ms")
        time.sleep(0.1)
print()

# check exit status
assert (g.get_exit_status() == 0)
print("generator exit status:", g.get_exit_status())
print("global 'a' is", a)  # 'a' after update
