import random
import time
from typing import *  # use type hints to make signatures clear

import snakefish

a = 1  # global variable 'a'


# the function that will be executed on a snakefish generator
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

# spawn a snakefish generator
g = snakefish.Generator(f, extract, merge)
print("generator alive?", g.is_alive())
g.start()
print("generator alive?", g.is_alive())
print()

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
        except IndexError:
            print("received nothing, sleep for 100 ms")
            time.sleep(0.1)
print()

# try to join the generator
while True:
    if g.try_join():
        print("try_join() successful")
        break
    else:
        print("try_join() unsuccessful, sleep for 100ms")
        time.sleep(0.1)
print("generator alive?", g.is_alive())
print()

# check exit status
assert (g.get_exit_status() == 0)
print("generator exit status:", g.get_exit_status())
print("global 'a' is", a)  # 'a' after update
