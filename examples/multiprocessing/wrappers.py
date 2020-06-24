import inspect
import math
import multiprocessing
import os
import traceback

# this is needed for merging to work
if multiprocessing.get_start_method(True) != 'fork':
    multiprocessing.set_start_method('fork')


def map(f, args, extract=None, merge=None, concurrency=0, chunksize=0, star=False):
    args = list(args)

    # use default concurrency (i.e. # of physical cores)?
    if concurrency == 0:
        concurrency = os.cpu_count()

    # use default chunk size?
    if chunksize == 0:
        chunksize = math.ceil(len(args) / concurrency)

    # calculate # of batches
    n_batch = math.ceil(len(args) / (concurrency * chunksize))

    # run jobs
    threads = []
    results = []

    for i in range(n_batch):
        for j in range(concurrency):
            # split args
            start = (i * concurrency + j) * chunksize
            thread_args = args[start:(start + chunksize)]

            # if thread_args is empty, then args has been exhausted
            if len(thread_args) == 0:
                break

            # spawn thread
            if star:
                t = Thread(target=lambda: [f(*arg) for arg in thread_args], extract=extract, merge=merge)
            else:
                t = Thread(target=lambda: [f(arg) for arg in thread_args], extract=extract, merge=merge)
            t.start()
            threads.append(t)

        # join threads and get results
        for t in threads:
            t.join()
            results.extend(t.get_result())

        # reset
        threads.clear()

    return results


def starmap(f, args, extract=None, merge=None, concurrency=0, chunksize=0):
    return map(f, args, extract, merge, concurrency, chunksize, True)


# wrapper around multiprocessing.Process to (mostly) align its behavior with snakefish.Thread
class Thread(multiprocessing.Process):
    def __init__(self, target=None, name=None, args=(), kwargs={},
                 extract=None, merge=None, shared_vars=None, *, daemon=None):
        self.sender, self.receiver = multiprocessing.Pipe()
        self.merge = merge
        self.shared_vars = shared_vars
        self.ret_val = None
        self.traceback = None
        self.merging = (extract is not None) and (merge is not None) and (shared_vars is not None)

        def wrapped_target():
            # ref: https://stackoverflow.com/a/33599967
            try:
                self.sender.send(target(*args, **kwargs))
            except Exception as e:
                self.sender.send(e)
                self.sender.send(traceback.format_exc())

            if self.merging:
                self.sender.send(extract(self.shared_vars))

        super().__init__(None, wrapped_target, name, daemon=daemon)

    def join(self, timeout=None):
        super().join(timeout)

        if self.exitcode is None:  # not joined
            return False
        else:  # joined
            self.ret_val = self.receiver.recv()

            if isinstance(self.ret_val, Exception):  # exception in child
                self.traceback = self.receiver.recv()

            if self.merging:
                self.merge(self.shared_vars, self.receiver.recv())

            return True

    def get_exit_status(self):
        return self.exitcode

    def get_result(self):
        if isinstance(self.ret_val, Exception):
            print(self.traceback)
            raise self.ret_val
        else:
            return self.ret_val


# wrapper around multiprocessing.Process to (mostly) align its behavior with snakefish.Generator
class Generator(multiprocessing.Process):
    def __init__(self, target=None, name=None, args=(), kwargs={},
                 extract=None, merge=None, shared_vars=None, *, daemon=None):
        self.channel = multiprocessing.Queue()
        self.cmd_sender, self.cmd_receiver = multiprocessing.Pipe()
        self.merge = merge
        self.shared_vars = shared_vars
        self.next_sent = False
        self.stop_sent = False
        self.merging = (extract is not None) and (merge is not None) and (shared_vars is not None)

        if inspect.isgeneratorfunction(target):
            self.gen = target(*args, **kwargs)
        else:
            raise RuntimeError("target is not a generator function")

        def wrapped_target():
            while True:
                cmd = self.cmd_receiver.recv()
                if cmd == 'STOP':
                    break
                elif cmd == 'NEXT':
                    try:
                        self.channel.put(next(self.gen))
                    except Exception as e:
                        self.channel.put(e)
                        self.channel.put(traceback.format_exc())
                else:
                    raise RuntimeError("unknown command %s!" % cmd)

            if self.merging:
                self.channel.put(extract(self.shared_vars))

        super().__init__(None, wrapped_target, name, daemon=daemon)

    def next(self, block=True, timeout=None):
        if not self.next_sent:
            self.cmd_sender.send("NEXT")
            self.next_sent = True

        val = self.channel.get(block, timeout)
        self.next_sent = False

        if isinstance(val, Exception):
            tb = self.channel.get()
            if not isinstance(val, StopIteration):
                print(tb)
            raise val
        else:
            return val

    def join(self, timeout=None):
        if not self.stop_sent:
            self.cmd_sender.send("STOP")
            self.stop_sent = True

        super().join(timeout)

        if self.exitcode is None:  # not joined
            return False
        else:  # joined
            if self.merging:
                self.merge(self.shared_vars, self.channel.get())
            return True

    def get_exit_status(self):
        return self.exitcode
