import worker

class ThreadError(Exception):
    """Base Class for exceptions in this module"""
    pass

class CreationError(ThreadError):
    """Raised when an error occurs at thread creation"""
    pass

class RunTimeError(ThreadError):
    """Raised when an error occurs during thread run"""

    def __init__(self, message, thread, exception):
        self.message = message
        self.thread = thread
        self.exception = exception

class JoinError(ThreadError):
    """Raised when there's a problem at one of the join operations"""

    def __init__(self, message, thread):
        self.message = message
        self.thread = thread

class Thread(object):
    """The Thread class"""

    __everyone = set()

    def __init__(self, to_run, *args, **kwargs):
        self.__func = to_run
        self.__args = args
        self.__kwargs = kwargs
        self.__simple = lambda: to_run(*args, **kwargs)
        self.__joined = False
        self.__everyone.add(self)
        self.__worker = worker.Worker(self.__simple)

    def join(self):
        if self.__joined:
            raise JoinError("Cannot join with already joined thread", self)
        self.__everyone.remove(self)
        self.__joined = True
        return self.__worker.collect()

    def joinable(self):
        return not self.__joined

    def recreate(self):
        return Thread(self.__func, *self.__args, **self.__kwargs)

    everyone = __everyone

def joinAll(pool=None):
    if pool is None:
        pool = Thread.everyone.copy()
    results = []
    for t in pool:
        results.append(t.join())
    return results

def joinAny(pool):
    if len(pool) is 0:
        return None, pool
    for t in pool:
        if not t.joinable():
            raise JoinError("The provided pool contains joined thread", t)
    res = pool[0].join()
    pool.pop(0)
    return res, pool
