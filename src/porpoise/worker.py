# import cporpoise
import pickle
import tempfile
import os
import random

# class Worker():
#     """ A medium class between porpoise's python and c interfaces.
#         Used for Thread Creation.                                  """
#
#     def __init__(self, to_run):
#         self.__fd = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
#         self.__shmf = os.fdopen(self.__fd, mode="w+b", buffering=0)
#         self.__id = cporpoise.create_worker(to_run, self.__shmf, self.__fd)
#
#     def collect(self):
#         cporpoise.collect_worker(self.__id, self.__fd)
#         self.__shmf.seek(0)
#         res = pickle.load(self.__shmf)
#         self.__shmf.close()
#         return res

def make_connection(bidirectional=False):
    if bidirectional:
        # fd_1 = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
        # fd_2 = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
        fd_1 = os.open("/dev/shm/" + str(random.random()), os.O_CREAT|os.O_RDWR,mode=600)
        fd_2 = os.open("/dev/shm/" + str(random.random()), os.O_CREAT|os.O_RDWR,mode=600)
        read_1, write_1 = os.pipe()
        read_2, write_2 = os.pipe()
        return Conn(fd_1, fd_2, read_1, write_2, read_2, write_1), \
               Conn(fd_2, fd_1, read_2, write_1, read_1, write_2)
    else:
        # fd = os.open("/dev/shm", os.O_TMPFILE|os.O_EXCL|os.O_RDWR,mode=600)
        fd = os.open("/dev/shm/" + str(random.random()), os.O_CREAT|os.O_RDWR,mode=600)
        read, write = os.pipe()
        return Dest(fd, read, write), Source(fd, write, read)

class Conn():
    """ A medium class between porpoise's python and c interface.
        Used for limitless communication (bounded by RAM).
        This class supports bidirectional communications.        """

    def __init__(self, send_fd, recv_fd, read, write, other_1, other_2):
        self.__send_fd = send_fd
        self.__recv_fd = recv_fd
        self.__send_offset = 0
        self.__recv_offset = 0
        self.__w_pipe = write
        self.__r_pipe = read
        self.__other_1 = other_1
        self.__other_2 = other_2

    def close(self):
        os.close(self.__send_fd)
        os.close(self.__recv_fd)
        os.close(self.__other_1)
        os.close(self.__other_2)
        os.close(self.__w_pipe)
        os.close(self.__r_pipe)
        self.__send_offset = 0
        self.__recv_offset =0


    def send(self, data):
        data = pickle.dumps(data)
        os.pwrite(self.__send_fd, data, self.__send_offset)
        self.__send_offset += len(data)
        os.write(self.__w_pipe, len(data).to_bytes(8, 'big'))

    def recv(self):
        length = int.from_bytes(os.read(self.__r_pipe, 8), 'big')
        data = pickle.loads(os.pread(self.__recv_fd, length, self.__recv_offset))
        self.__recv_offset += length
        return data




class Source():
    """ A medium class between porpoise's python and c interface.
        Used for limitless communication (bounded by RAM).
        This class supports only outgoing communications.        """

    def __init__(self, fd, pipe, other):
        self.__fd = fd
        self.__offset = 0
        self.__pipe = pipe
        self.__other = other

    def close(self):
        os.close(self.__fd)
        self.__offset = 0
        os.close(self.__other)
        os.close(self.__pipe)

    def send(self, data):
        # TODO: error check
        data = pickle.dumps(data)
        os.pwrite(self.__fd, data, self.__offset)
        self.__offset += len(data)
        os.write(self.__pipe, len(data).to_bytes(8, 'big'))

    def recv(self):
        raise Exception

class Dest():
    """ A medium class between porpoise's python and c interface.
        Used for limitless communication (bounded by RAM).
        This class supports only incomming communications.       """

    def __init__(self, fd, pipe, other):
        self.__fd = fd
        self.__offset = 0
        self.__pipe = pipe
        self.__other = other

    def close(self):
        os.close(self.__fd)
        self.__offset = 0
        os.close(self.__other)
        os.close(self.__pipe)


    def send(self, data):
        raise Exception

    def recv(self):
        # TODO: error check
        length = int.from_bytes(os.read(self.__pipe, 8), 'big')
        data = pickle.loads(os.pread(self.__fd, length, self.__offset))
        self.__offset += length
        return data
