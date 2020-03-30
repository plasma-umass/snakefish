/**
 * \file channel.h
 */

#ifndef SNAKEFISH_CHANNEL_H
#define SNAKEFISH_CHANNEL_H

#include <atomic>
#include <set>

#include <semaphore.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "buffer.h"
#include "semaphore_t.h"

namespace snakefish {

const unsigned PICKLE_PROTOCOL = 4;
const size_t DEFAULT_CHANNEL_SIZE = 2l * 1024l * 1024l * 1024l; // 2 GiB

/**
 * \brief An IPC channel with built-in synchronization and (semi-automatic)
 * reference counting support.
 *
 * The support for reference counting is "semi-automatic" in the sense that
 * the `channel::fork()` function must be called right before calling
 * the system `fork()`.
 *
 * Characteristics of the functions:
 * - `send_bytes()`: won't block; can throw
 * - `send_pyobj()`: won't block; can throw
 * - `receive_bytes()`: may or may not block^; can throw
 * - `receive_pyobj()`: may or may not block^; can throw
 *
 * ^: the client must specify whether the function should block when there's no
 * incoming messages to receive
 *
 * **NOTE**: When doing shared memory IO, a lock must be acquired for
 * synchronization purposes. As such, all functions mentioned above
 * can technically block on the said lock. The characteristics described
 * apply when there's no contention.
 */
class channel {
public:
  /**
   * \brief Create a channel with buffer size `DEFAULT_CHANNEL_SIZE`.
   */
  channel() : channel(DEFAULT_CHANNEL_SIZE) {}

  /**
   * \brief Destructor implementing reference counting.
   */
  ~channel();

  /**
   * \brief Copy constructor implementing reference counting.
   */
  channel(const channel &t);

  /**
   * \brief No copy assignment operator.
   */
  channel &operator=(const channel &t) = delete;

  /**
   * \brief Move constructor implementing reference counting.
   */
  channel(channel &&t) noexcept;

  /**
   * \brief No move assignment operator.
   */
  channel &operator=(channel &&t) = delete;

  /**
   * \brief Create a channel with buffer size `size`.
   *
   * \param size The size of the underlying shared memory buffer.
   */
  explicit channel(size_t size);

  /**
   * \brief Send some bytes.
   *
   * \param bytes Pointer to the start of the bytes.
   * \param len Number of bytes to send.
   *
   * \throws std::overflow_error If the underlying buffer does not have enough
   * space to accommodate the request.
   * \throws std::runtime_error If some semaphore error occurred.
   */
  void send_bytes(void *bytes, size_t len);

  /**
   * \brief Send a python object.
   *
   * This function will serialize `obj` using `pickle` and send the binary
   * output.
   *
   * \throws std::overflow_error If the underlying buffer does not have enough
   * space to accommodate the request.
   * \throws std::runtime_error If some semaphore error occurred.
   */
  void send_pyobj(const py::object &obj);

  /**
   * \brief Receive some bytes.
   *
   * \param block Should this function block?
   *
   * \returns The received bytes wrapped in a `buffer`.
   *
   * \throws std::out_of_range If the underlying buffer does not have enough
   * content to accommodate the request (this only applies when `block` is
   * `false`).
   * \throws std::runtime_error If some semaphore error occurred.
   * \throws std::bad_alloc If `malloc()` failed.
   */
  buffer receive_bytes(bool block);

  /**
   * \brief Receive a python object.
   *
   * This function will receive some bytes and deserialize them using `pickle`.
   *
   * \param block Should this function block?
   *
   * \throws std::out_of_range If the underlying buffer does not have enough
   * content to accommodate the request (this only applies when `block` is
   * `false`).
   * \throws std::runtime_error If some semaphore error occurred.
   * \throws std::bad_alloc If `malloc()` failed.
   */
  py::object receive_pyobj(bool block);

  /**
   * Called by the client to indicate that this `channel` is about to be
   * shared with another process.
   */
  void fork();

protected:
  /**
   * \brief The buffer used to hold messages.
   */
  void *shared_mem;

  /**
   * \brief Global/interprocess reference counter.
   */
  std::atomic_uint32_t *ref_cnt;

  /**
   * \brief Process local reference counter.
   */
  std::atomic_uint32_t *local_ref_cnt;

  /**
   * \brief "Mutex" for the shared memory.
   */
  std::atomic_flag *lock;

  /**
   * \brief Index of first used byte.
   */
  size_t *start;

  /**
   * \brief Index of first unused byte.
   */
  size_t *end;

  /**
   * \brief A flag indicating whether this buffer is full.
   */
  bool *full;

  /**
   * \brief Number of unread messages.
   */
  semaphore_t n_unread;

  /**
   * \brief Number of bytes this buffer can hold.
   */
  size_t capacity;

private:
  /**
   * \brief Acquire `lock`.
   */
  void acquire_lock() {
    while (lock->test_and_set())
      ;
  }

  /**
   * \brief Release `lock`.
   */
  void release_lock() { lock->clear(); }

  /**
   * \brief `pickle.dumps()`
   */
  py::object dumps;

  /**
   * \brief `pickle.loads()`
   */
  py::object loads;
};

/**
 * \brief A vector is used to track all channels so that their destructors
 * may be called at exit.
 */
extern std::vector<channel *> all_channels;

/**
 * \brief A set is used to track all disposed channels so that they won't be
 * destructed multiple times.
 */
extern std::set<channel *> disposed_channels;

} // namespace snakefish

#endif // SNAKEFISH_CHANNEL_H
