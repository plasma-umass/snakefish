#include "thread.h"

namespace snakefish {

void thread::start() {
  if (started) {
    throw std::runtime_error("this thread has already been started");
  }

  channel.fork();
  pid_t pid = fork();
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
    alive = false;
    ret_val = channel.receive_pyobj(true);
    globals = channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
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
    alive = false;
    ret_val = channel.receive_pyobj(true);
    globals = channel.receive_pyobj(true);
    merge_func(py::globals(), globals);
    return true;
  }
}

int thread::get_exit_status() {
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

void thread::run() {
  if (is_parent) {
    fprintf(stderr, "run() called by parent!\n");
    abort();
  }
  if (!started) {
    fprintf(stderr, "run() called but thread hasn't started yet!\n");
    abort();
  }
  if (!alive) {
    fprintf(stderr, "run() called but thread is no longer alive!\n");
    abort();
  }

  ret_val = func();
  channel.send_pyobj(ret_val);
  globals = extract_func(py::globals());
  channel.send_pyobj(globals);
  exit(0);
}

} // namespace snakefish
