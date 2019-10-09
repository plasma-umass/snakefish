#include <pybind11/pybind11.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <interpose.hh>

using namespace pybind11::literals;
namespace py = pybind11;

__attribute__ ((noinline)) void sign_up(int transfer, off64_t *t_size) {
	return;
}

__attribute__ ((noinline)) void sign_out(int transfer) {
	return;
}

__attribute__ ((noinline)) void log_in(int ret_fd, int transfer, off64_t t_size) {
	return;
}

__attribute__ ((noinline)) void log_out() {
	return;
}

__attribute__ ((noinline)) void espeicaly_return_file(int fd) {
	fprintf(stderr, "fd is %d\n", fd);
	return;
}



int create_worker(py::function f,  py::object file, py::int_ ret_fd, py::int_ transfer_fd) {
	off64_t t_size = 0;
	sign_up(transfer_fd, &t_size);
	pid_t pid = fork();
	if (pid == 0) {
		log_in(ret_fd, transfer_fd, t_size);
		py::object result = f();
		py::object isinstance = py::module::import("builtins").attr("isinstance");
		py::object io_base = py::module::import("io").attr("IOBase");
		if (isinstance(result, io_base).cast<bool>()) {
			// TODO: needs more error checking. And is awfully not robust
			py::object result_fd = result.attr("fileno")();
			espeicaly_return_file(result_fd.cast<int>());
		} else {
			py::object dumper = py::module::import("pickle").attr("dump");
			dumper("obj"_a=result, "file"_a=file, "protocol"_a=4);
		}
		log_out();
		exit(0);
	} else if (pid > 0) {
		return pid;
	} else {
		perror("fork failed");
		exit(1);
	}
}

void collect_worker(py::int_ pid, py::int_ fd, py::int_ transfer_fd) {
	if (waitpid(pid, NULL, 0) == -1) {
		perror("waitpid failed");
		exit(1);
	}
	sign_out(transfer_fd);
}

PYBIND11_MODULE(cporpoise, m) {
	m.doc() = "porpoise python c extensions";
	// optional module docstring
	m.def("create_worker", &create_worker, "A function that runs a python function object in c");
	m.def("collect_worker", &collect_worker, "A function that collects a worker's results");
}
