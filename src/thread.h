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
 * \brief A "thread" class for executing Python functions with true parallelism.
 */
class thread {
public:
  /**
   * \brief No default constructor.
   */
  thread() = delete;

  /**
   * \brief Destructor.
   */
  ~thread();

  /**
   * \brief No copy constructor.
   */
  thread(const thread &t) = delete;

  /**
   * \brief No copy assignment operator.
   */
  thread &operator=(const thread &t) = delete;

  /**
   * \brief No move constructor.
   */
  thread(thread &&t) = delete;

  /**
   * \brief No move assignment operator.
   */
  thread &operator=(thread &&t) = delete;

  /**
   * \brief Create a new snakefish thread.
   *
   * \param f The Python function this thread will execute.
   *
   * \param extract The globals extraction function this thread will execute.
   * Its signature should be `(dict) -> dict`. It should take the current
   * `globals()` and return a dict of globals that must be kept. The
   * returned dict will be passed to `merge` as the second parameter. Note that
   * anything contained in the returned dict must be [picklable]
   * (https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
   *
   * \param merge The merge function this thread will execute. Its signature
   * should be `(dict, dict) -> nil`. The first dict is the current `globals()`,
   * and the second dict is the new `globals()`. The merge function must merge
   * the two by updating the first dict.
   */
  thread(py::function f, py::function extract, py::function merge);

  /**
   * \brief Start executing this thread. In other words, start executing the
   * underlying function.
   */
  void start();

  /**
   * \brief Join this thread.
   *
   * This will block the calling thread until this thread terminates. If the
   * thread hasn't been started yet, this function will throw an exception.
   */
  void join();

  /**
   * \brief Try to join this thread.
   *
   * This is the non-blocking version of `join()`.
   *
   * \returns `true` if joined. `false` otherwise.
   */
  bool try_join();

  /**
   * \brief Get the status of the thread.
   *
   * \returns `true` if this thread has been started and has not yet terminated;
   * `false` otherwise.
   */
  bool is_alive() { return alive; }

  /**
   * \brief Get the exit status of the thread.
   *
   * \returns -1 if the thread hasn't been started yet. -2 if the thread hasn't
   * exited yet. -3 if the thread exited abnormally. Otherwise, the exit status
   * given by the thread is returned.
   *
   * Note that a snakefish thread is really a process. Hence the "exit status"
   * terminology.
   */
  int get_exit_status();

  /**
   * \brief Get the output of the thread (i.e. the output of the underlying
   * function).
   */
  py::object get_result();

private:
  /**
   * \brief Run the underlying function and return the result to the parent.
   */
  void run();

  bool is_parent;
  pid_t child_pid;
  bool started;
  bool alive;
  int child_status;
  py::function func;
  py::function extract_func;
  py::function merge_func;
  py::object ret_val;
  py::object globals;
  channel _channel;
};

/**
 * \brief A vector is used to track all threads so that their destructors
 * may be called at exit.
 */
extern std::vector<thread *> all_threads;

/**
 * \brief A set is used to track all disposed threads so that they won't be
 * destructed multiple times.
 */
extern std::set<thread *> disposed_threads;

} // namespace snakefish

#endif // SNAKEFISH_THREAD_H
