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
 * \brief An enum type to make generator IPC cleaner.
 */
enum generator_cmd { NEXT, STOP };

/**
 * \brief A "generator" class for executing Python generators with true
 * parallelism.
 */
class generator {
public:
  /**
   * \brief No default constructor.
   */
  generator() = delete;

  /**
   * \brief Destructor.
   */
  ~generator();

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
   * Its signature should be `(dict) -> dict`. It should take the current
   * `globals()` and return a dict of globals that must be kept. The
   * returned dict will be passed to `merge` as the second parameter. Note that
   * anything contained in the returned dict must be [picklable]
   * (https://docs.python.org/3/library/pickle.html#what-can-be-pickled-and-unpickled).
   *
   * \param merge The merge function this generator will execute. Its signature
   * should be `(dict, dict) -> nil`. The first dict is the current `globals()`,
   * and the second dict is the new `globals()`. The merge function must merge
   * the two by updating the first dict.
   *
   * \throws std::runtime_error If `f` is not a generator function.
   */
  generator(const py::function &f, py::function extract, py::function merge);

  /**
   * \brief Start executing this generator.
   */
  void start();

  /**
   * \brief Get the next output of the generator.
   *
   * \param block Should this function block?
   *
   * \throws std::out_of_range If the next output isn't ready yet (only applies
   * when `block` is `false`).
   */
  py::object next(bool block);

  /**
   * \brief Join this generator.
   *
   * This will block the calling thread until this generator terminates. If the
   * generator hasn't been started yet, this function will throw an exception.
   */
  void join();

  /**
   * \brief Try to join this generator.
   *
   * This is the non-blocking version of `join()`.
   *
   * \returns `true` if joined. `false` otherwise.
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
   * the thread hasn't been joined yet.
   */
  int get_exit_status();

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

/**
 * \brief A set is used to track all generators so that their destructors
 * may be called at exit.
 */
extern std::set<generator *> all_generators;

} // namespace snakefish

#endif // SNAKEFISH_GENERATOR_H
