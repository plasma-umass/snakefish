#include "snakefish.h"

PYBIND11_MODULE(snakefish, m) {
  py::class_<snakefish::thread>(m, "Thread")
      .def(py::init<py::function>())
      .def(py::init<py::function, py::function, py::function>())
      .def("start", &snakefish::thread::start)
      .def("join", &snakefish::thread::join)
      .def("try_join", &snakefish::thread::try_join)
      .def("is_alive", &snakefish::thread::is_alive)
      .def("get_exit_status", &snakefish::thread::get_exit_status)
      .def("get_result", &snakefish::thread::get_result)
      .def("dispose", &snakefish::thread::dispose);

  py::class_<snakefish::generator>(m, "Generator")
      .def(py::init<py::function>())
      .def(py::init<py::function, py::function, py::function>())
      .def("start", &snakefish::generator::start)
      .def("next", &snakefish::generator::next)
      .def("join", &snakefish::generator::join)
      .def("try_join", &snakefish::generator::try_join)
      .def("get_exit_status", &snakefish::generator::get_exit_status)
      .def("dispose", &snakefish::generator::dispose);

  py::class_<snakefish::channel>(m, "Channel")
      .def(py::init<>())
      .def(py::init<size_t>())
      .def("send_pyobj", &snakefish::channel::send_pyobj)
      .def("receive_pyobj", &snakefish::channel::receive_pyobj)
      .def("dispose", &snakefish::channel::dispose);

  m.def("get_timestamp", &snakefish::get_timestamp);
  m.def("get_timestamp_serialized", &snakefish::get_timestamp_serialized);
  m.def("map", &snakefish::map, py::arg("f"), py::arg("args"),
        py::arg("concurrency") = 0, py::arg("chunksize") = 0);
  m.def("starmap", &snakefish::starmap, py::arg("f"), py::arg("args"),
        py::arg("concurrency") = 0, py::arg("chunksize") = 0);

  py::register_exception<std::runtime_error>(m, "RuntimeError");
}
