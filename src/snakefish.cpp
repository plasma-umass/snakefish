#include "snakefish.h"

PYBIND11_MODULE(csnakefish, m) {
  m.doc() = "true parallelism for python";

  py::class_<snakefish::thread>(m, "Thread")
      .def(py::init<py::function>())
      .def("start", &snakefish::thread::start)
      .def("join", &snakefish::thread::join)
      .def("try_join", &snakefish::thread::try_join)
      .def("is_alive", &snakefish::thread::is_alive)
      .def("get_exit_status", &snakefish::thread::get_exit_status);

  py::register_exception<std::runtime_error>(m, "RuntimeError");
}
