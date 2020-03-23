#include "snakefish.h"

PYBIND11_MODULE(csnakefish, m) {
  m.doc() = "true parallelism for python";

  py::class_<snakefish::thread>(m, "Thread")
      .def(py::init<py::function, py::function, py::function>())
      .def("start", &snakefish::thread::start)
      .def("join", &snakefish::thread::join)
      .def("try_join", &snakefish::thread::try_join)
      .def("is_alive", &snakefish::thread::is_alive)
      .def("get_exit_status", &snakefish::thread::get_exit_status)
      .def("get_result", &snakefish::thread::get_result);

  py::class_<snakefish::channel>(m, "Channel")
      .def("send_pyobj", &snakefish::channel::send_pyobj)
      .def("receive_pyobj", &snakefish::channel::receive_pyobj)
      .def("try_receive_pyobj", &snakefish::channel::try_receive_pyobj)
      .def("fork", &snakefish::channel::fork);

  m.def("create_channel",
        (std::pair<snakefish::channel, snakefish::channel>(*)()) &
            snakefish::create_channel);
  m.def("create_channel",
        (std::pair<snakefish::channel, snakefish::channel>(*)(size_t)) &
            snakefish::create_channel);

  py::register_exception<std::runtime_error>(m, "RuntimeError");
}
