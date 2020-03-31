#include "generator.h"
#include "snakefish.h"

namespace snakefish {

std::set<generator *> all_generators;

generator::~generator() { all_generators.erase(this); }

generator::generator(const py::function &f, py::function extract,
                     py::function merge)
    : is_parent(false), child_pid(0), started(false), alive(false),
      child_status(0), extract_func(std::move(extract)),
      merge_func(std::move(merge)), _channel(), cmd_channel(1024),
      next_sent(false), stop_sent(false) {

  py::object is_gen_func =
      py::module::import("inspect").attr("isgeneratorfunction");

  if (is_gen_func(f).equal(py::bool_(true)))
    gen = f();
  else
    throw std::runtime_error("f is not a generator function");

  _next = py::getattr(gen, "__next__");

  all_generators.insert(this);
}

void generator::start() {
  if (started) {
    throw std::runtime_error("this generator has already been started");
  }

  pid_t pid = snakefish_fork();
  if (pid > 0) {
    is_parent = true;
    child_pid = pid;
    started = true;
    alive = true;
  } else if (pid == 0) {
    is_parent = false;
    child_pid = 0;
    started = true;
    alive = true;
    run();
  } else {
    perror("fork() failed");
    throw std::runtime_error("fork() failed");
  }
}

py::object generator::next(bool block) {
  if (!next_sent) {
    send_cmd(generator_cmd::NEXT);
    next_sent = true;
  }

  py::object val = _channel.receive_pyobj(block);
  next_sent = false;

  if (py::isinstance(val, PyExc_Exception)) {
    // raise exceptions
    py::object type = _channel.receive_pyobj(true);
    PyErr_SetObject(type.ptr(), val.ptr());
    throw py::error_already_set();
  } else {
    return val;
  }
}

void generator::join() {
  if (!started) {
    throw std::runtime_error("this generator has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "join() called from child!\n");
    abort();
  }

  if (!stop_sent) {
    send_cmd(generator_cmd::STOP);
    stop_sent = true;
  }

  int result = waitpid(child_pid, &child_status, 0);
  if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    alive = false;
    globals = _channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
  }
}

bool generator::try_join() {
  if (!started) {
    throw std::runtime_error("this generator has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "try_join() called from child!\n");
    abort();
  }

  if (!stop_sent) {
    send_cmd(generator_cmd::STOP);
    stop_sent = true;
  }

  int result = waitpid(child_pid, &child_status, WNOHANG);
  if (result == 0) {
    return false;
  } else if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    alive = false;
    globals = _channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
    return true;
  }
}

int generator::get_exit_status() {
  if (!started) {
    return -1;
  } else if (alive) {
    return -2;
  } else if (WIFEXITED(child_status)) {
    return WEXITSTATUS(child_status);
  } else {
    return -3;
  }
}

void generator::run() {
  if (is_parent) {
    fprintf(stderr, "run() called by parent!\n");
    abort();
  }
  if (!started) {
    fprintf(stderr, "run() called but generator hasn't started yet!\n");
    abort();
  }
  if (!alive) {
    fprintf(stderr, "run() called but generator is no longer alive!\n");
    abort();
  }

  while (true) {
    generator_cmd cmd = receive_cmd();

    if (cmd == generator_cmd::STOP) {
      break;
    } else if (cmd == generator_cmd::NEXT) {
      try {
        _channel.send_pyobj(_next());
      } catch (py::error_already_set &e) {
        // send exceptions to parent
        _channel.send_pyobj(e.value());
        _channel.send_pyobj(e.type());

        // print stacktrace
        e.restore();
        PyErr_PrintEx(0);
      }
    } else {
      fprintf(stderr, "unknown command: %d!\n", cmd);
      abort();
    }
  }

  globals = extract_func(py::globals());
  _channel.send_pyobj(globals);
  snakefish_exit(0);
}

void generator::send_cmd(generator_cmd cmd) {
  if (!is_parent) {
    fprintf(stderr, "send_cmd() called by child!\n");
    abort();
  }

  cmd_channel.send_bytes(&cmd, sizeof(generator_cmd));
}

generator_cmd generator::receive_cmd() {
  if (is_parent) {
    fprintf(stderr, "receive_cmd() called by parent!\n");
    abort();
  }

  buffer bytes = cmd_channel.receive_bytes(true);
  return *static_cast<generator_cmd *>(bytes.get_ptr());
}

} // namespace snakefish
