import inspect
import multiprocessing
import traceback
from typing import *  # use type hints to make signatures clear

# this is needed for merging to work
if multiprocessing.get_start_method(True) != 'fork':
    multiprocessing.set_start_method('fork')


# the default function for extracting shared global variables
def default_extract(_globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {}  # no-op


# the default function for merging shared global variables
def default_merge(_old_globals: Dict[str, Any], _new_globals: Dict[str, Any]) -> None:
    pass  # no-op


# wrapper around multiprocessing.Process to (mostly) align its behavior with snakefish.Thread
class Thread(multiprocessing.Process):
    def __init__(self, target=None, name=None, args=(), kwargs={},
                 extract=default_extract, merge=default_merge,
                 shared_vars=None, *, daemon=None):
        self.sender, self.receiver = multiprocessing.Pipe()
        self.merge = merge
        self.shared_vars = shared_vars
        self.ret_val = None
        self.traceback = None

        def wrapped_target(args=(), kwargs={}):
            # ref: https://stackoverflow.com/a/33599967
            try:
                self.sender.send(target(*args, **kwargs))
            except Exception as e:
                self.sender.send(e)
                self.sender.send(traceback.format_exc())

            if self.shared_vars is not None:  # has shared global variables to merge
                self.sender.send(extract(self.shared_vars))

        super().__init__(None, wrapped_target, name, args, kwargs, daemon=daemon)

    def join(self, timeout=None):
        super().join(timeout)

        if self.exitcode is not None:  # if joined
            self.ret_val = self.receiver.recv()

            if isinstance(self.ret_val, Exception):  # exception in child
                self.traceback = self.receiver.recv()

            if self.shared_vars is not None:  # has shared global variables to merge
                self.merge(self.shared_vars, self.receiver.recv())

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
                 extract=default_extract, merge=default_merge,
                 shared_vars=None, *, daemon=None):
        self.channel = multiprocessing.Queue()
        self.cmd_sender, self.cmd_receiver = multiprocessing.Pipe()
        self.merge = merge
        self.shared_vars = shared_vars
        self.next_sent = False
        self.stop_sent = False

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

            if self.shared_vars is not None:  # has shared global variables to merge
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

        if self.exitcode is not None:  # if joined
            if self.shared_vars is not None:  # has shared global variables to merge
                self.merge(self.shared_vars, self.channel.get())

    def get_exit_status(self):
        return self.exitcode
