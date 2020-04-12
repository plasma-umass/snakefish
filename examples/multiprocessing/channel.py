import multiprocessing
import queue
import time

import wrappers

channel = multiprocessing.Queue()  # create a Queue


# a function that will be executed on a thread
def f1() -> None:
    print("thread #1: sending an object...")
    channel.put([42])

    print("thread #1: sleep for 500 ms")
    time.sleep(0.5)
    print("thread #1: sending an object...")
    channel.put({'1': 1})


# a function that will be executed on a thread
def f2() -> None:
    print("thread #2: sending an object...")
    channel.put((42,))


# a function that will be executed on a thread
def f3() -> None:
    # blocking receive
    print("thread #3: receiving an object...")
    received = channel.get(True)
    print("thread #3: received", received)

    # blocking receive
    print("thread #3: receiving an object...")
    received = channel.get(True)
    print("thread #3: received", received)

    # non-blocking receive
    print("thread #3: receiving an object...")
    start = time.time()
    while True:
        try:
            received = channel.get(False)
            break
        except queue.Empty:
            print("thread #3: received nothing, sleep for 100 ms")
            time.sleep(0.1)
    print("thread #3: received", received)
    print("thread #3: receiving took %s sec" % (time.time() - start))


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
