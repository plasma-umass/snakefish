import multiprocessing
import inspect
# import porpals
import worker

class ThreadError(Exception):
    """Base Class for exceptions in this module"""
    pass

class CreationError(ThreadError):
    """Raised when an error occurs at thread creation"""
    pass

class InputError(CreationError):
    """Raised from wrong input to the Thread constructor"""
    def __init__(self, message):
        self.message = message

class JoinError(ThreadError):
    """Raised when there's a problem at one of the join operations"""
    def __init__(self, message, thread):
        self.message = message
        self.thread = thread

class ReturnedExcpetion(ThreadError):
    """Used by Porpoise internals for exception handling"""
    def __init__(self, exception):
        self.exception = exception

class RunTimeError(ThreadError):
    """Raised when an error occurs during thread run"""
    def __init__(self, message, thread, exception):
        self.message = message
        self.thread = thread
        self.exception = exception


def normal(fun, conn, e_handler):
    try:
        ret = fun()
        # for x in porpals.porpals_list:
        #     conn.send(porpals.strip(x))
        conn.send(ret)

    except Exception as e:
        conn.send(ReturnedExcpetion(e_handler(e)))


def default_except(exception):
    return exception

def gen(fun, conn, e_handler):
    try:
        g = fun()
        ret = next(g)
        conn.send(ret)
        while(True):
            conn.recv()
            ret = next(g)
            conn.send(ret)
    except Exception as e:
        conn.send(ReturnedExcpetion(e_handler(e)))


class Thread(object):
    """The Thread class"""

    __everyone = set()

    def __init__(self, to_run, *args, e_handler=default_except, **kwargs):

        if not inspect.isfunction(e_handler):
            raise InputError("Invalid Input. Please follow Thread(function, args, exception_handler = 'a function' , kwargs)")

        self.__func = to_run
        self.__args = args
        self.__kwargs = kwargs
        self.__e_handler = e_handler
        self.__simple = lambda: to_run(*args, **kwargs)

        ## Subject to change, don't error check for now

        if inspect.isgeneratorfunction(to_run):
             self.__globals__ = []
             # self.__parent_conn, self.__child_conn = multiprocessing.Pipe(True)
             self.__parent_conn, self.__child_conn = worker.make_connection(True)
             self.__thread = multiprocessing.Process(target=lambda: gen(self.__simple, self.__child_conn, self.__e_handler))
             self.__gen = True
        else:
            # self.__globals__ = porpals.copy()
            # self.__parent_conn, self.__child_conn = multiprocessing.Pipe()
            self.__parent_conn, self.__child_conn = worker.make_connection()
            self.__thread = multiprocessing.Process(target=lambda: normal(self.__simple, self.__child_conn, self.__e_handler))
            self.__gen = False

        self.__thread.start()
        self.__alive = True
        self.__everyone.add(self)

    def join(self):
        if not self.__alive:
            raise JoinError("Cannot join with dead thread", self)

        ## Subject to change, don't error check for now
        if self.__gen:
            ## Make this better
            self.__parent_conn.send(1)
        else:
            self.__everyone.remove(self)
            self.__thread.join()
            self.__alive = False

        # for i in range(len(self.__globals__)):
        #     porpals.porpals_list[i].porpal_merge(self.__globals__[i], self.__parent_conn.recv())

        ret = self.__parent_conn.recv()

        if not self.__alive:
            self.__parent_conn.close()
            # self.__child_conn.close()

        if isinstance(ret, ReturnedExcpetion):
            if self.__gen:
                self.__everyone.remove(self)
                self.__thread.join()
                self.__alive = False

            raise RunTimeError("Error at runtime.", self, ret.exception)
        else:
            return ret

    def joinable(self):
        return self.__alive

    def recreate(self):
        return Thread(self.__func, *self.__args, e_handler=self.__e_handler, **self.__kwargs)

    def terminate(self):
        if not self.__alive:
            self.__thread.terminate()
            self.__alive = False
            self.__parent_conn.close()
            # self.__child_conn.close()
            self.__everyone.remove(self)
            self.__globals__.clear()


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
