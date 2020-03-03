#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>


#include <interpose.hh>
#include <vector>
#include <unordered_map>

#define ERROR(X, S) if (X) {perror(S); exit(1);}
#define MIN(X,Y) X > Y ? Y : X
#define MAX(X,Y) X > Y ? X : Y

#define FD_SIZE 0x1000 // set in accoradance to the kernel limit
#define TRANSFER_SIZE 0x10*0x1000

extern "C" int open64(const char *pathname, int flags, mode_t mode);
typedef int (*main_fn_t)(int argc, char **argv, char **env);
typedef int (*close_fn_t)(int);
typedef ssize_t (*write_fn_t)(int fd, const void *buf, size_t count);
typedef ssize_t (*read_fn_t)(int fd, void *buf, size_t count);
typedef off64_t (*lseek_fn_t)(int fd, off64_t offset, int whence);
static main_fn_t real_main;
static close_fn_t real_close;
static write_fn_t real_write;
static read_fn_t real_read;
static lseek_fn_t real_lseek64;



#define WRITELOG 0x01
#define TRUNCLOG 0x02

// struct used for the log
struct file_log {
	int type; // the type of log
	off64_t offset; // length of truncate or the offset of write
	size_t size; // the size of the write log
	char *buf; // the buffer that holds what was written
};

#define PORP_COMMIT 0x01
#define PORP_RETURN 0x02

struct log {
	// Mainly to prevent corruption and ensure fd was opened by Porpoise
	int fd;
	// Current offset of the file
	off64_t offset;
	// 0x01: If the creating thread is not a worker
	// 0x02: Indicates that this file is used for returning and should not be logged
	uint64_t flags;
	// Owners of this specific file. File can only be commited if owners is 1
	// and it is commitable
	size_t owners;
	// A log of pending commits to files in order of commits
	std::vector<file_log> to_commit;
};


// Set to false initially because the main program is not a worker.
// For any thread after that, it should be set to true.
static bool worker = false;

// Information about specific fds, located in their corresponding index
static log fd_map[FD_SIZE];

// Set to 0 initially because the main thread has no transfer fd.
// The rest of the files have a transfter that lets them transfter their
// information to the parent thread at termination point
static int transfer_fd = 0;
static off64_t transfer_size = 0;
static off64_t transfer_max = 0;

// Commits to file by calling the commits on the to_commit vector.
// Only happens if fd is commitable in its current state
// NOTE: This function should be only and only called if (fd_map[fd].owners == 1)
// TODO: Do I stick in a safety check here?
// TODO: deal with the return values
int commit(int fd) {
	if (fd_map[fd].flags & PORP_COMMIT) {
		for (file_log i : fd_map[fd].to_commit) {
			switch (i.type) {
			case WRITELOG:
				pwrite(fd, i.buf, i.size, i.offset);
				break;
			case TRUNCLOG:
				ftruncate(fd, i.offset);
				break;
			default:
				break;
			}
		}
		fd_map[fd].to_commit.clear();
		real_lseek64(fd, fd_map[fd].offset, SEEK_SET);
	}
}

// for now, just free l->buf, since free ignores nullptr
int delete_log(file_log *l) {
	free(l->buf);
	l->type = -1;
	l->offset = 0;
	l->size = 0;
	l->buf = nullptr;
}

int set_log(file_log *l, int type, off64_t offset, size_t size, char *buf) {
	l->type = type;
	l->offset = offset;
	if (type == WRITELOG) {
		// if it is a write log
		l->buf = (char*) malloc(size * sizeof(char));
		if (l->buf == nullptr) {
			// TODO: something
		}
		memcpy(l->buf, buf, size);
		l->size = size;
	} else {
		// if it is another type of log
		l->buf = nullptr;
		l->size = 0;
	}
}

// Inline function to clear out an entry in the fd_map
inline void clear(int fd) {
	fd_map[fd].fd = -1;
	fd_map[fd].offset = 0;
	fd_map[fd].flags = 0;
	fd_map[fd].owners = 0;
	for (file_log i : fd_map[fd].to_commit) {
		delete_log(&i);
	}
	fd_map[fd].to_commit.empty();
}

// Inline function to decrement the number of owners on an fd, will commit if it
// falls to 1
inline void dec_owner(int fd) {
	if (fd_map[fd].fd == fd) {
		fd_map[fd].owners--;
		if (fd_map[fd].owners == 1) {
			// TODO: do error handling
			// commit(fd);
		}
	}
}


inline void increase_transfer() {
	ERROR(ftruncate(transfer_fd, transfer_max + TRANSFER_SIZE) == -1, "ftruncate failed");
	transfer_max += TRANSFER_SIZE;
}

// Transfers ownership to parent
// Written data per fd has the following shape
// START_OFFSET : fd integer
//            + : offset of file
//            + : number of logs
//            + : log
//              | + : log type
//              | + : log offset
//              | + : size of buffer  (if WRITELOG)
//              | + : buffer          (if WRITELOG)
static void transfer_file(int fd) {
	if (transfer_max <= transfer_size + sizeof(int) + sizeof(off64_t) + sizeof(size_t)) {
		increase_transfer();
	}
	size_t s = fd_map[fd].to_commit.size();
	ERROR(pwrite(transfer_fd, &fd, sizeof(int), transfer_size) != sizeof(int), "pwrite");
	transfer_size += sizeof(int);
	ERROR(pwrite(transfer_fd, &fd_map[fd].offset, sizeof(off64_t), transfer_size) != sizeof(off64_t), "pwrite");
	transfer_size += sizeof(off64_t);
	ERROR(pwrite(transfer_fd, &s, sizeof(size_t), transfer_size) != sizeof(size_t), "pwrite");
	transfer_size += sizeof(size_t);
	for (file_log l : fd_map[fd].to_commit) {
		if (transfer_max <= transfer_size + l.size + sizeof(size_t) + sizeof(off64_t) + sizeof(int)) {
			increase_transfer();
		}
		ERROR(pwrite(transfer_fd, &l.type, sizeof(int), transfer_size) != sizeof(int), "pwrite");
		transfer_size += sizeof(int);
		ERROR(pwrite(transfer_fd, &l.offset, sizeof(off64_t), transfer_size) != sizeof(off64_t), "pwrite");
		transfer_size += sizeof(off64_t);
		if (l.type == WRITELOG) {
			ERROR(pwrite(transfer_fd, &l.size, sizeof(size_t), transfer_size) != sizeof(size_t), "pwrite");
			transfer_size += sizeof(size_t);

			ERROR(pwrite(transfer_fd, l.buf, l.size, transfer_size) != l.size, "pwrite");
			transfer_size += l.size;
		}
		delete_log(&l);
	}
}

// Do the sign up for both child and parent
// Includes increasing the owner size
// Writing the fd of each shared file descriptor to the transfer
// Writing FD_SIZE + 1 to the transfer
__attribute__ ((noinline)) void sign_up(int transfer, off64_t *t_size) {
	ERROR(ftruncate(transfer, TRANSFER_SIZE) == -1, "ftruncate failed");
	*t_size = 0;
	for (size_t i = 0; i < FD_SIZE; ++i) {
		// Only increment the ones that we know
		if (fd_map[i].fd == i) {
			fd_map[i].owners++;
			ERROR(pwrite(transfer, &(fd_map[i].fd), sizeof(int), *t_size) != sizeof(int), "pwrite");
			*t_size += sizeof(int);
		}
	}
	int fd = FD_SIZE + 1;
	ERROR(pwrite(transfer, &fd, sizeof(int), *t_size) != sizeof(int), "pwrite");
	*t_size += sizeof(int);
}

__attribute__ ((noinline)) void sign_out(int transfer) {
	ERROR(real_lseek64(transfer, 0, SEEK_SET) == (off64_t) -1, "lseek64");
	char buf[256];
	// read the list of shared files
	int fd = -1;
	off64_t offset = 0;
	ERROR(pread(transfer, &fd, sizeof(int), offset) != sizeof(int), "pread");
	offset += sizeof(int);
	while(fd != FD_SIZE + 1 && fd != -1) {
		fd_map[fd].owners--;
		if (fd_map[fd].owners == 1) {
			commit(fd);
		}
		ERROR(pread(transfer, &fd, sizeof(int), offset) != sizeof(int), "pread");
		offset += sizeof(int);
	}

	// read the file logs
	fd = -1;
	size_t count = 0;
	ERROR(pread(transfer, &fd, sizeof(int), offset) != sizeof(int), "pread");
	offset += sizeof(int);
	while(fd != FD_SIZE + 1 && fd != -1) {
		// set the other file offset as current offset
		ERROR(pread(transfer, &fd_map[fd].offset, sizeof(off64_t), offset) != sizeof(off64_t), "pread");
		offset += sizeof(off64_t);
		ERROR(pread(transfer, &count, sizeof(size_t), offset) != sizeof(size_t), "pread");
		offset += sizeof(size_t);
		file_log l;
		for (size_t i = 0; i < count; ++i) {
			// read the logs
			ERROR(pread(transfer, &l.type, sizeof(int), offset) != sizeof(int), "pread");
			offset += sizeof(int);
			ERROR(pread(transfer, &l.offset, sizeof(off64_t), offset) != sizeof(off64_t), "pread");
			offset += sizeof(off64_t);
			if (l.type == TRUNCLOG) {
				ERROR(ftruncate(fd, l.offset), "ftruncate by user");
			} else if (l.type == WRITELOG) {
				ERROR(pread(transfer, &l.size, sizeof(size_t), offset) != sizeof(size_t), "pread");
				offset += sizeof(size_t);
				l.buf = (char*) malloc(l.size * sizeof(char));
				if (l.buf == nullptr) {
					// TODO: something
				}
				ERROR(pread(transfer, l.buf, l.size, offset) != l.size, "pread this");
				offset += l.size;
				if (fd_map[fd].owners == 1) {
					ERROR(pwrite(fd, l.buf, l.size, l.offset) != l.size, "pwrite");
					free(l.buf);
				} else {
					fd_map[fd].to_commit.push_back(l);
				}
			}
		}
		ERROR(pread(transfer, &fd, sizeof(int), offset) != sizeof(int), "pread");
		offset += sizeof(int);
	}
}
// Do the log in for the child
// Currently this includes setting ret_fd and transfer_fd to allow writes
__attribute__ ((noinline)) void log_in(int ret_fd, int transfer, off64_t t_size) {
	fd_map[ret_fd].flags |= PORP_RETURN;
	fd_map[transfer].flags |= PORP_RETURN;
	transfer_fd = transfer;
	transfer_max = TRANSFER_SIZE;
	transfer_size = t_size;
	worker = true;
}


// Do the log out for the child.
// Does the transfer per affected fd
// Ends the transfer file with an fd of value FD_SIZE + 1
__attribute__ ((noinline)) void log_out() {
	// read the list of shared files
	int fd = -1;
	off64_t offset = 0;
	ERROR(pread(transfer_fd, &fd, sizeof(int), offset) != sizeof(int), "pread");
	offset += sizeof(int);
	while(fd != FD_SIZE + 1 && fd != -1) {
		if (fd_map[fd].fd != -1) {
			transfer_file(fd);
		}
		ERROR(pread(transfer_fd, &fd, sizeof(int), offset) != sizeof(int), "pread");
		offset += sizeof(int);
	}
	if (transfer_max <= transfer_size + sizeof(int)) {
		increase_transfer();
	}
	fd = FD_SIZE + 1;
	pwrite(transfer_fd, &fd, sizeof(int), transfer_size);
}

INTERPOSE(open64)(const char *pathname, int flags, mode_t mode) {
	if (worker) {
		// if this is a worker, set premission to just read for now
		// TODO: fix this

	}
	int fd = real::open64(pathname, flags, mode);
	// TODO: prevent truncation in children
	if (fd != -1) {
		// Only save information if opening the file succeeded
		off64_t offset = real_lseek64(fd, 0, SEEK_CUR);
		if (offset != (off64_t) -1) {
			// If seek succeeded
			fd_map[fd].fd = fd;
			fd_map[fd].offset = offset;
			// Automatically set to not ret file and derive commitability from
			// process status.
			// Return files should not be written to until the end of thread's
			// life. Porpoise creates these files before a thread's creation
			// and then immedietly sets this true via the sign up function.
			// Therefore, it is safe to set it to false initially for all files.
			// Use the worker global to find out if it's commitable or not.
			// Files opened in children are not commitable, others are.
			// so no commitables
			fd_map[fd].flags = worker? 0: PORP_COMMIT;
			// Someone opened it, and persumably files don't get opened during
			// fork, so initall owners are 1.
			fd_map[fd].owners = 1;
		} else {
			// Should not happen if this is an actual file, but one may never know
			// Nothing was written, don't need to check for the return value of
			// close
			real_close(fd);
			fd = -1;
		}
	}
	return fd;
}

INTERPOSE(close)(int fd) {
	if (fd_map[fd].fd == fd) {
		// This file was opened by Porpoise
		clear(fd);
	}
	return real::close(fd);
}

// extract data from offset offset with size count using iterators
ssize_t iter_read(int fd, char* buf, off64_t offset, size_t size,
                  std::vector<file_log>::reverse_iterator rbegin,
                  std::vector<file_log>::const_reverse_iterator crend) {
	if (size == 0) {
		return 0;
	}
	ssize_t ret = 0;
	while(rbegin != crend) {
		// check if log is within range
		if (offset <= rbegin->offset && rbegin->offset < offset + size) {
			// case 0: log buffer starts at mid point in requested read
			size_t diff = rbegin->offset - offset;
			// copy shared range
			ret = MIN(size - diff, rbegin->size);
			memcpy(buf + diff, rbegin->buf, ret);
			// if the requested size was not entirely found
			size_t passed = diff + ret;
			// find the region after offset + size if it passed the log's range
			// size - passed will be zero if we covered everything and
			// should just return 0
			ret += iter_read(fd, buf + passed, offset + passed, size - passed,
			                 rbegin + 1, crend);
			// find the region before rbegin->offset
			ret += iter_read(fd, buf, offset, diff, rbegin + 1, crend);
			break;
		} else if (rbegin->offset <= offset && offset < rbegin->offset + rbegin->size) {
			// case 1: read request is in the middle of the log buffer
			size_t diff = offset - rbegin->offset;
			// copy shared range
			ret = MIN(size, rbegin->size - diff);
			memcpy(buf, rbegin->buf + diff, ret);
			// if the requested size was not entirely found
			// look for regions after this
			ret += iter_read(fd, buf + ret, offset + ret, size - ret, rbegin + 1, crend);
			break;
		} else {
			// case 3: log buffer and requested read don't share any regions, proceed to next
			rbegin++;
		}
	}
	// base case: if requested read is not in the buffers, read directly
	// from the file
	if (rbegin == crend) {
		ret += pread(fd, buf, size, offset);
	}
	return ret;
}



INTERPOSE(read)(int fd, void *buf, size_t count) {
	//  1. File descriptor is not being tracked, aka fd == -1
	//  2. File descriptor is a return file ret_file == true
	//  3. Flie descriptor is not shared with anyone and it is the main thread
	if (fd_map[fd].to_commit.empty()) {
		return real::read(fd, buf, count);
	} else {
		// iterate the vector and extract the data from the log
		iter_read(fd, (char*) buf, fd_map[fd].offset, count,
		          fd_map[fd].to_commit.rbegin(), fd_map[fd].to_commit.crend());
	}
}

// The write warpper
INTERPOSE(write)(int fd, void *buf, size_t count) {
	// write to file in 3 cases:
	//  1. File descriptor is not being tracked, aka fd == -1
	//  2. File descriptor is a return file ret_file == true
	//  3. Flie descriptor is not shared with anyone and it is the main thread
	if (fd_map[fd].fd == -1 || fd_map[fd].flags & PORP_RETURN ||
	    (fd_map[fd].owners == 1 && fd_map[fd].flags & PORP_COMMIT)) {
		return real_write(fd, buf, count);
	} else {
		file_log log;
		set_log(&log, WRITELOG, fd_map[fd].offset, count, (char*) buf);
		fd_map[fd].to_commit.push_back(log);
		fd_map[fd].offset += count;
		return count;
	}
}

INTERPOSE(lseek64)(int fd, off64_t offset, int whence) {
	// Log seeks not in 3 cases
	//  1. File descriptor is not being tracked, aka fd == -1
	//  2. File descriptor is a return file ret_file == true
	//  3. Flie descriptor is not shared with anyone and it is the main thread
	if (fd_map[fd].fd == -1 || fd_map[fd].flags & PORP_RETURN ||
	    (fd_map[fd].owners == 1 && fd_map[fd].flags & PORP_COMMIT)) {
		return real_lseek64(fd, offset, whence);
	} else {
		if (whence == SEEK_SET) {
			fd_map[fd].offset = offset;
		} else if (whence == SEEK_CUR) {
			fd_map[fd].offset += offset;
		} else {
			fprintf(stderr, "AHHHH, WELP, DON'T SUPPORT THIS SEEK YET\n");
			real_lseek64(fd, fd_map[fd].offset, SEEK_SET);
			fd_map[fd].offset += real_lseek64(fd, offset, whence);
		}
		return fd_map[fd].offset;
	}
}



int wrapped_main(int argc, char **argv, char **env) {
	for (size_t i = 0; i < FD_SIZE; ++i) {
		clear(i);
	}
	return real_main(argc, argv, env);
}



// Intercepts the program and captures the actual overwritten functions
extern "C" int __libc_start_main(main_fn_t main_fn, int argc, char **argv,
	                             void (*init)(), void (*fini)(),
	                             void (*rtld_fini)(), void *stack_end) {
	auto real_libc_start_main = reinterpret_cast<decltype(__libc_start_main) *>(
	                            dlsym(RTLD_NEXT, "__libc_start_main"));
	real_main = main_fn;
	real_close = reinterpret_cast<decltype(close) *>(dlsym(RTLD_NEXT, "close"));
	real_read = reinterpret_cast<decltype(read) *>(dlsym(RTLD_NEXT, "read"));
	real_write = reinterpret_cast<decltype(write) *>(dlsym(RTLD_NEXT, "write"));
	real_lseek64 = reinterpret_cast<decltype(lseek64) *>(dlsym(RTLD_NEXT, "lseek64"));
	return real_libc_start_main(wrapped_main, argc, argv, init, fini,
	                                  rtld_fini, stack_end);
}
