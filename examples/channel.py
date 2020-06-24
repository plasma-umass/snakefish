import time

import snakefish

channel = snakefish.Channel()  # create a channel


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


# spawn 3 snakefish threads
t1 = snakefish.Thread(f1)
t2 = snakefish.Thread(f2)
t3 = snakefish.Thread(f3)

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

# release resources
channel.dispose()
t1.dispose()
t2.dispose()
t3.dispose()
