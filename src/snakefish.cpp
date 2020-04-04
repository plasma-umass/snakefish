#include <x86intrin.h>

#include "snakefish.h"

static inline uint64_t get_timestamp() { return __rdtsc(); }

static inline uint64_t get_timestamp_serialized() {
  asm volatile("lfence" ::: "memory");
  return __rdtsc();
}

PYBIND11_MODULE(snakefish, m) {
  m.doc() = "true parallelism for python";

  py::class_<snakefish::thread>(m, "Thread")
      .def(py::init<py::function, py::function, py::function>())
      .def("start", &snakefish::thread::start)
      .def("join", &snakefish::thread::join)
      .def("try_join", &snakefish::thread::try_join)
      .def("is_alive", &snakefish::thread::is_alive)
      .def("get_exit_status", &snakefish::thread::get_exit_status)
      .def("get_result", &snakefish::thread::get_result);

  py::class_<snakefish::generator>(m, "Generator")
      .def(py::init<py::function, py::function, py::function>())
      .def("start", &snakefish::generator::start)
      .def("next", &snakefish::generator::next)
      .def("join", &snakefish::generator::join)
      .def("try_join", &snakefish::generator::try_join)
      .def("get_exit_status", &snakefish::generator::get_exit_status);

  py::class_<snakefish::channel>(m, "Channel")
      .def(py::init<>())
      .def(py::init<size_t>())
      .def("send_pyobj", &snakefish::channel::send_pyobj)
      .def("receive_pyobj", &snakefish::channel::receive_pyobj);

  m.def("get_timestamp", &get_timestamp);
  m.def("get_timestamp_serialized", &get_timestamp_serialized);

  py::register_exception<std::runtime_error>(m, "RuntimeError");
}
