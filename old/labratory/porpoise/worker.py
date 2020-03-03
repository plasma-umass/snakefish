import cporpoise
import pickle
import tempfile
import os

class Worker(object):
    """ A medium class between porpoise's python and c interfaces"""

    def __init__(self, to_run):
        self.__transfer = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
        self.__fd = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
        self.__shmf = os.fdopen(self.__fd, mode="w+b", buffering=0)
        self.__id = cporpoise.create_worker(to_run, self.__shmf, self.__fd, self.__transfer)

    def collect(self):
        cporpoise.collect_worker(self.__id, self.__fd, self.__transfer)
        self.__shmf.seek(0)
        res = pickle.load(self.__shmf)
        self.__shmf.close()
        return res
