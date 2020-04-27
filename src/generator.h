/**
 * \file generator.h
 */

#ifndef SNAKEFISH_GENERATOR_H
#define SNAKEFISH_GENERATOR_H

#include <sys/wait.h>
#include <unistd.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"

namespace snakefish {

/**
 * \brief An enum type used to make generator IPC cleaner.
 */
enum generator_cmd { NEXT, STOP };

/**
 * \brief A "generator" class for executing Python generators with true
 * parallelism.
 *
 * **IMPORTANT**: The `dispose()` function must be called when a generator is
 * no longer needed to release resources.
 */
class generator {
public:
  /**
   * \brief No default constructor.
   */
  generator() = delete;

  /**
   * \brief Default destructor.
   */
  ~generator() = default;

  /**
   * \brief No copy constructor.
   */
  generator(const generator &t) = delete;

  /**
   * \brief No copy assignment operator.
   */
  generator &operator=(const generator &t) = delete;

  /**
   * \brief No move constructor.
   */
  generator(generator &&t) = delete;

  /**
   * \brief No move assignment operator.
   */
  generator &operator=(generator &&t) = delete;

  /**
   * \brief Create a new snakefish generator.
   *
   * \param f The Python function this generator will execute. It must be a
   * [generator function](https://wiki.python.org/moin/Generators).
   *
   * \param extract The globals extraction function this generator will execute.
   * Its signature should be `(dict) -> dict`. It should take the child's
   * `globals()` and return a dict of globals that must be kept. The
   * returned dict will be passed to `merge` as the second parameter. Note that
   * anything contained in the returned dict must be [picklable]
   * (https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
   *
   * \param merge The merge function this generator will execute. Its signature
   * should be `(dict, dict) -> nil`. The first dict is the parent's
   * `globals()`, and the second dict is the one returned by `extract()`. The
   * merge function must merge the two by updating the first dict.
   *
   * Note that `extract()` is executed by the child process, and `merge` is
   * executed by the parent process. The return value of `extract()` is sent
   * to the parent through IPC.
   *
   * \throws std::runtime_error If `f` is not a generator function.
   */
  generator(const py::function &f, py::function extract, py::function merge);

  /**
   * \brief Start executing this generator.
   *
   * \throws std::runtime_error If this generator has already been started OR
   * if `fork()` failed.
   */
  void start();

  /**
   * \brief Get the next output of the generator.
   *
   * \param block Should this function block?
   *
   * \throws std::out_of_range If the next output isn't ready yet (only applies
   * when `block` is `false`).
   * \throws e `next()` will rethrow any exception thrown by the generator.
   */
  py::object next(bool block);

  /**
   * \brief Join this generator.
   *
   * This will block the caller until this generator terminates.
   *
   * \throws std::runtime_error If this generator hasn't been started yet OR if
   * `waitpid()` failed.
   */
  void join();

  /**
   * \brief Try to join this generator.
   *
   * This is the non-blocking version of `join()`.
   *
   * \returns `true` if joined. `false` otherwise.
   *
   * \throws std::runtime_error If this generator hasn't been started yet OR if
   * `waitpid()` failed.
   */
  bool try_join();

  /**
   * \brief Get the exit status of the generator.
   *
   * \returns The exit status of the generator. If the generator was terminated
   * by signal `N`, `-N` would be returned.
   *
   * Note that a snakefish generator is really a process. Hence the
   * "exit status" terminology.
   *
   * \throws std::runtime_error If the generator hasn't been started yet OR if
   * the generator hasn't been joined yet.
   */
  int get_exit_status();

  /**
   * \brief Release resources held by this generator.
   */
  void dispose();

private:
  /**
   * \brief Run the underlying generator on demand.
   */
  void run();

  /**
   * \brief Send a command to the child.
   */
  void send_cmd(generator_cmd cmd);

  /**
   * \brief Receive a command from the parent.
   */
  generator_cmd receive_cmd();

  bool is_parent;
  pid_t child_pid;
  bool started;
  bool joined;
  int child_status;
  py::object gen;   // the generator object
  py::object _next; // _next() => gen.__next__()
  py::function extract_func;
  py::function merge_func;
  py::object globals;
  channel _channel;    // channel used to send data
  channel cmd_channel; // channel used to send commands
  bool next_sent;      // has command NEXT been sent?
  bool stop_sent;      // has command STOP been sent?
};

} // namespace snakefish

#endif // SNAKEFISH_GENERATOR_H
