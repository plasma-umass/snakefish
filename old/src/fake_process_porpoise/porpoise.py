import multiprocessing
import porpals

class ThreadError(Exception):
    """Base Class for exceptions in this module"""
    pass

class CreationError(ThreadError):
    """Raised when an error occurs at thread cration"""
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

def transfer(fun, conn):
    ret = fun()
    porpals.pause()
    for i in range(len(porpals.porpals_list)):
        porpals.porpals_list[i] = porpals.strip(porpals.porpals_list[i])
    conn.send(porpals.porpals_list)
    conn.send(ret)

class Thread(object):
    """The Thread class"""

    __everyone = set()

    def __init__(self, to_run, args=[], kwargs={}):
        porpals.record()
        self.__copy = porpals.copy()
        self.__parent_conn, self.__child_conn = multiprocessing.Pipe()
        self.__func = to_run
        self.__args = args
        self.__kwargs = kwargs
        self.__simple = lambda: to_run(*args, **kwargs)
        self.__thread = multiprocessing.Process(target=lambda: transfer(self.__simple, self.__child_conn))
        self.__thread.start()
        self.__joined = False
        self.__everyone.add(self)

    def join(self):
        if self.__joined:
            raise JoinError("Cannot join with already joined thread", self)
        self.__everyone.remove(self)
        self.__joined = True
        self.__thread.join()
        if len(self.__everyone) is 0:
            porpals.pause()
        joined_list = self.__parent_conn.recv()
        porpals.merge(self.__copy, joined_list)
        self.__copy.clear()
        return self.__parent_conn.recv()

    def joinable(self):
        return not self.__joined

    def recreate(self):
        porpals.record()
        self.__copy = porpals.copy()
        return Thread(self.__func, self.__args, self.__kwargs)

    @classmethod
    def join_all(cls, pool=None):
        if pool is None:
            pool = cls.__everyone.copy()
        results = []
        for t in pool:
            results.append(t.join())
        return results

    @classmethod
    def join_any(cls, pool):
        if len(pool) is 0:
            return None, pool
        for t in pool:
            if not t.joinable():
                raise JoinError("The provided pool contains joined thread", t)
        res = pool[0].join()
        pool.pop(0)
        return res, pool
