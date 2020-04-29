#include "thread.h"
#include "util.h"

namespace snakefish {

thread::thread(py::function f)
    : is_parent(false), child_pid(0), started(false), joined(false),
      child_status(0), func(std::move(f)), extract_func(), merge_func(),
      _channel(), merging(false) {

  // create shared memory
  alive = static_cast<std::atomic_bool *>(
      util::get_shared_mem(sizeof(std::atomic_bool), true));
  alive->store(false);
}

thread::thread(py::function f, py::function extract, py::function merge)
    : is_parent(false), child_pid(0), started(false), joined(false),
      child_status(0), func(std::move(f)), extract_func(std::move(extract)),
      merge_func(std::move(merge)), _channel(), merging(true) {

  // create shared memory
  alive = static_cast<std::atomic_bool *>(
      util::get_shared_mem(sizeof(std::atomic_bool), true));
  alive->store(false);
}

void thread::start() {
  if (started) {
    throw std::runtime_error("this thread has already been started");
  }

  pid_t pid = fork();
  if (pid > 0) {
    is_parent = true;
    child_pid = pid;
    started = true;
    alive->store(true);
  } else if (pid == 0) {
    is_parent = false;
    child_pid = 0;
    started = true;
    run();
  } else {
    perror("fork() failed");
    throw std::runtime_error("fork() failed");
  }
}

void thread::join() {
  if (!started) {
    throw std::runtime_error("this thread has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "join() called from child!\n");
    abort();
  }

  int result = waitpid(child_pid, &child_status, 0);
  if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    joined = true;
    if (merging) {
      globals = _channel.receive_pyobj(true);
      ret_val = _channel.receive_pyobj(true);
      merge_func(py::globals(), globals);
    } else {
      ret_val = _channel.receive_pyobj(true);
    }
  }
}

bool thread::try_join() {
  if (!started) {
    throw std::runtime_error("this thread has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "try_join() called from child!\n");
    abort();
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
    joined = true;
    if (merging) {
      globals = _channel.receive_pyobj(true);
      ret_val = _channel.receive_pyobj(true);
      merge_func(py::globals(), globals);
    } else {
      ret_val = _channel.receive_pyobj(true);
    }
    return true;
  }
}

int thread::get_exit_status() {
  if (!started || !joined) {
    throw std::runtime_error("exit status is not yet available");
  } else if (WIFEXITED(child_status)) {
    return WEXITSTATUS(child_status);
  } else if (WIFSIGNALED(child_status)) {
    return -WTERMSIG(child_status);
  } else {
    fprintf(stderr, "thread have neither exited nor received a signal!\n");
    abort();
  }
}

void thread::run() {
  if (is_parent) {
    fprintf(stderr, "run() called by parent!\n");
    abort();
  }
  if (!started) {
    fprintf(stderr, "run() called but thread hasn't started yet!\n");
    abort();
  }

  try {
    ret_val = func();
    if (merging) {
      globals = extract_func(py::globals());
      _channel.send_pyobj(globals);
    }
    _channel.send_pyobj(ret_val);
  } catch (py::error_already_set &e) {
    // send globals
    if (merging) {
      globals = extract_func(py::globals());
      _channel.send_pyobj(globals);
    }

    // send exceptions to parent
    _channel.send_pyobj(e.value());
    _channel.send_pyobj(e.type());

    // send traceback
    if (e.trace()) {
      _channel.send_pyobj(
          py::module::import("traceback")
              .attr("format_exception")(e.type(), e.value(), e.trace()));
    } else {
      _channel.send_pyobj(
          py::module::import("traceback")
              .attr("format_exception_only")(e.type(), e.value()));
    }
  }

  alive->store(false);
  std::exit(0);
}

py::object thread::get_result() {
  if ((!started) || (!joined)) {
    throw std::runtime_error("result is not yet available");
  }

  if (py::isinstance(ret_val, PyExc_Exception)) {
    // handle exceptions
    static bool exc_received = false;
    if (!exc_received) {
      exc_type = _channel.receive_pyobj(true);
      exc_traceback = _channel.receive_pyobj(true);
      exc_traceback = py::str("").attr("join")(exc_traceback);
      exc_received = true;
    }

    py::print(exc_traceback);
    PyErr_SetObject(exc_type.ptr(), ret_val.ptr());
    throw py::error_already_set();
  } else {
    return ret_val;
  }
}

void thread::dispose() {
  if (munmap(alive, sizeof(std::atomic_bool))) {
    perror("munmap() failed");
    abort();
  }
  _channel.dispose();
}

} // namespace snakefish
