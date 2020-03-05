#include "thread.h"

namespace snakefish {

void thread::start() {
  if (started) {
    throw std::runtime_error("this thread has already been started");
  }

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

    // TODO move this
    ret_val = func();
    exit(0);
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
  }
}

void thread::try_join() {
  if (!started) {
    throw std::runtime_error("this thread has not been started yet");
  }
  if (!is_parent) {
    fprintf(stderr, "try_join() called from child!\n");
    abort();
  }

  int result = waitpid(child_pid, &child_status, WNOHANG);
  if (result == -1) {
    perror("waitpid() failed");
    throw std::runtime_error("waitpid() failed");
  } else if (result != child_pid) {
    fprintf(stderr, "joined pid = %d, child pid = %d!\n", result, child_pid);
    abort();
  } else {
    alive = false;
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

} // namespace snakefish
