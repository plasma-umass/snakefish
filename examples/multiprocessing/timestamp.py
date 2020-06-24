import multiprocessing
import random
import time

import wrappers

channel = multiprocessing.Queue()  # create a Queue


# a function that will be executed on a thread
def f1() -> None:
    if random.random() < 0.5:
        print("thread #1: sleep for 100 ms")
        time.sleep(0.1)

    ts = time.perf_counter_ns()
    print("thread #1: sending an event...")
    channel.put((ts, "explosion"))


# a function that will be executed on a thread
def f2() -> None:
    ts = time.perf_counter_ns()
    print("thread #2: sending an event...")
    channel.put((ts, "implosion"))


# a function that will be executed on a thread
def f3() -> None:
    # blocking receive
    print("thread #3: receiving an object...")
    received1 = channel.get()

    # blocking receive
    print("thread #3: receiving an object...")
    received2 = channel.get()

    print("thread #3: timestamps are %s and %s" % (received1[0], received2[0]))
    if received1[0] <= received2[0]:
        print("thread #3: %s occurred before %s" % (received1[1], received2[1]))
    else:
        print("thread #3: %s occurred before %s" % (received2[1], received1[1]))


# spawn 3 threads
t1 = wrappers.Thread(target=f1)
t2 = wrappers.Thread(target=f2)
t3 = wrappers.Thread(target=f3)

t1.start()
t2.start()
t3.start()

# join the threads and check exit status
t1.join()
assert (t1.get_exit_status() == 0)
print("thread #1 exit status:", t1.get_exit_status())
print("result of f1():", t1.get_result())

t2.join()
assert (t2.get_exit_status() == 0)
print("thread #2 exit status:", t2.get_exit_status())
print("result of f2():", t2.get_result())

t3.join()
assert (t3.get_exit_status() == 0)
print("thread #3 exit status:", t3.get_exit_status())
print("result of f3():", t3.get_result())
