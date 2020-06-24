/**
 * \file thread.h
 */

#ifndef SNAKEFISH_THREAD_H
#define SNAKEFISH_THREAD_H

#include <sys/wait.h>
#include <unistd.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"

namespace snakefish {

/**
 * \brief A class for executing Python functions with true parallelism.
 *
 * **IMPORTANT**: The `dispose()` function must be called when a thread is no
 * longer needed to release resources.
 */
class thread {
public:
  /**
   * \brief No default constructor.
   */
  thread() = delete;

  /**
   * \brief Default destructor.
   */
  ~thread() = default;

  /**
   * \brief No copy constructor.
   */
  thread(const thread &t) = delete;

  /**
   * \brief No copy assignment operator.
   */
  thread &operator=(const thread &t) = delete;

  /**
   * \brief Default move constructor.
   */
  thread(thread &&t) = default;

  /**
   * \brief No move assignment operator.
   */
  thread &operator=(thread &&t) = delete;

  /**
   * \brief Create a new snakefish thread with no global variable merging.
   *
   * \param f The Python function this thread will execute.
   */
  explicit thread(py::function f);

  /**
   * \brief Create a new snakefish thread with global variable merging.
   *
   * \param f The Python function this thread will execute.
   *
   * \param extract The globals extraction function this thread will execute.
   * Its signature should be `(dict) -> dict`. It should take the child's
   * `globals()` and return a dict of globals that must be kept. The
   * returned dict will be passed to `merge` as the second parameter. Note that
   * anything contained in the returned dict must be [picklable]
   * (https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
   *
   * \param merge The merge function this thread will execute. Its signature
   * should be `(dict, dict) -> nil`. The first dict is the parent's
   * `globals()`, and the second dict is the one returned by `extract()`. The
   * merge function must merge the two by updating the first dict.
   *
   * Note that `extract()` is executed by the child process, and `merge` is
   * executed by the parent process. The return value of `extract()` is sent
   * to the parent through IPC.
   */
  thread(py::function f, py::function extract, py::function merge);

  /**
   * \brief Start executing this thread. In other words, start executing the
   * underlying function.
   *
   * \throws std::runtime_error If this thread has already been started OR
   * if `fork()` failed.
   */
  void start();

  /**
   * \brief Join this thread.
   *
   * This will block the caller until this thread terminates.
   *
   * \throws std::runtime_error If this thread hasn't been started yet OR if
   * `waitpid()` failed.
   */
  void join();

  /**
   * \brief Try to join this thread.
   *
   * This is the non-blocking version of `join()`.
   *
   * \returns `true` if joined. `false` otherwise.
   *
   * \throws std::runtime_error If this thread hasn't been started yet OR if
   * `waitpid()` failed.
   */
  bool try_join();

  /**
   * \brief Get the status of the thread.
   *
   * \returns `true` if this thread has been started and has not yet terminated;
   * `false` otherwise.
   */
  bool is_alive() { return alive->load(); }

  /**
   * \brief Get the exit status of the thread.
   *
   * \returns The exit status of the thread. If the thread was terminated by
   * signal `N`, `-N` would be returned.
   *
   * Note that a snakefish thread is really a process. Hence the "exit status"
   * terminology.
   *
   * \throws std::runtime_error If the thread hasn't been started yet OR if the
   * thread hasn't been joined yet.
   */
  int get_exit_status();

  /**
   * \brief Get the output of the thread (i.e. the output of the underlying
   * function).
   *
   * \throws std::runtime_error If the thread hasn't been started yet OR if the
   * thread hasn't been joined yet.
   * \throws e `get_result()` will rethrow any exception thrown by the thread.
   */
  py::object get_result();

  /**
   * \brief Release resources held by this thread.
   */
  void dispose();

private:
  /**
   * \brief Run the underlying function and return the result to the parent.
   */
  void run();

  bool is_parent;
  pid_t child_pid;
  bool started;
  std::atomic_bool *alive;
  bool joined;
  int child_status;
  py::function func;
  py::function extract_func;
  py::function merge_func;
  py::object ret_val;
  py::object globals;
  py::object exc_type;
  py::object exc_traceback;
  channel _channel;
  bool merging; // should globals be merged?
};

} // namespace snakefish

#endif // SNAKEFISH_THREAD_H
