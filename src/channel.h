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

/**
 * \brief The default [pickle protocol]
 * (https://docs.python.org/3.8/library/pickle.html#data-stream-format).
 */
const unsigned PICKLE_PROTOCOL = 4;

/**
 * \brief The default `channel` buffer size.
 *
 * Note that the buffer will be allocated using `mmap()` with flag
 * `MAP_NORESERVE`, so the actual memory consumption is much lower
 * in general.
 */
const size_t DEFAULT_CHANNEL_SIZE = 2l * 1024l * 1024l * 1024l; // 2 GiB

/**
 * \brief An IPC channel with built-in synchronization support.
 *
 * All messages are exchanged through the same buffer, so the communication is
 * not multiplexed. As such, `channel` should only be used for uni-directional
 * communication in most cases. Having multiple senders and/or multiple
 * receivers may or may not work, depending on the use cases.
 *
 * **IMPORTANT**: The `dispose()` function must be called when a channel is no
 * longer needed to release resources.
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
   * \brief Default destructor.
   */
  ~channel() = default;

  /**
   * \brief Default copy constructor.
   */
  channel(const channel &t) = default;

  /**
   * \brief No copy assignment operator.
   */
  channel &operator=(const channel &t) = delete;

  /**
   * \brief Default move constructor.
   */
  channel(channel &&t) = default;

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
   * \brief Send a Python object.
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
   * \brief Receive a Python object.
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
   * \brief Release resources held by this channel.
   */
  void dispose();

protected:
  /**
   * \brief The buffer used to hold messages.
   */
  void *shared_mem;

  /**
   * \brief "Mutex" for the shared memory.
   */
  semaphore_t lock;

  /**
   * \brief Index of first used byte.
   */
  std::atomic_size_t *start;

  /**
   * \brief Index of first unused byte.
   */
  std::atomic_size_t *end;

  /**
   * \brief A flag indicating whether this buffer is full.
   */
  std::atomic_bool *full;

  /**
   * \brief A semaphore representing the number of unread messages.
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
  void acquire_lock() { lock.wait(); }

  /**
   * \brief Release `lock`.
   */
  void release_lock() { lock.post(); }

  /**
   * \brief `pickle.dumps()`
   */
  py::object dumps;

  /**
   * \brief `pickle.loads()`
   */
  py::object loads;
};

} // namespace snakefish

#endif // SNAKEFISH_CHANNEL_H
