#include <pybind11/pybind11.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace pybind11::literals;
namespace py = pybind11;

int create_worker(py::function f,  py::object file, py::int_ ret_fd) {
	pid_t pid = fork();
	if (pid == 0) {
		py::object result = f();
		py::object dumper = py::module::import("pickle").attr("dump");
		dumper("obj"_a=result, "file"_a=file, "protocol"_a=4);
		exit(0);
	} else if (pid > 0) {
		return pid;
	} else {
		perror("fork failed");
		exit(1);
	}
}

void collect_worker(py::int_ pid, py::int_ fd) {
	if (waitpid(pid, NULL, 0) == -1) {
		perror("waitpid failed");
		exit(1);
	}
}

PYBIND11_MODULE(cporpoise, m) {
	m.doc() = "porpoise python c extensions";
	// optional module docstring
	m.def("create_worker", &create_worker, "A function that runs a python function object in c");
	m.def("collect_worker", &collect_worker, "A function that collects a worker's results");
}
