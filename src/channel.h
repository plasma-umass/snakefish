/**
 * \file channel.h
 */

#ifndef SNAKEFISH_CHANNEL_H
#define SNAKEFISH_CHANNEL_H

#include <atomic>
#include <tuple>
#include <utility>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "buffer.h"
#include "shared_buffer.h"

namespace snakefish {

static const unsigned PICKLE_PROTOCOL = 4;
static const size_t MAX_SOCK_MSG_SIZE = 1024;                          // bytes
static const size_t DEFAULT_CHANNEL_SIZE = 2l * 1024l * 1024l * 1024l; // 2 GiB

class receiver; // forward declaration to solve circular dependency

/**
 * \brief The sending part of an IPC channel, with (semi-automatic) reference
 * counting support.
 *
 * Note that the `send_*()` functions do not protect against concurrent
 * accesses.
 *
 * The support for reference counting is "semi-automatic" in the sense that
 * the `sender::fork()` function must be called right before calling
 * the system `fork()`.
 */
class sender {
public:
  /**
   * \brief No default constructor.
   */
  sender() = delete;

  /**
   * \brief Destructor implementing reference counting.
   */
  ~sender();

  /**
   * \brief Copy constructor implementing reference counting.
   */
  sender(const sender &t);

  /**
   * \brief No copy assignment operator.
   */
  sender &operator=(const sender &t) = delete;

  /**
   * \brief Move constructor implementing reference counting.
   */
  sender(sender &&t) noexcept;

  /**
   * \brief No move assignment operator.
   */
  sender &operator=(sender &&t) = delete;

  /**
   * \brief Create a synchronous channel.
   *
   * Here, synchronous means that the sender will block if the channel is full,
   * and the receiver will block if there's no incoming messages.
   *
   * \returns Two pairs of `(sender, receiver)`. One for each communicating
   * party.
   */
  friend std::tuple<sender, receiver, sender, receiver>
  sync_channel(size_t buffer_size);

  /**
   * \brief Send some bytes.
   *
   * \param bytes Pointer to the start of the bytes.
   * \param len Number of bytes to send.
   */
  void send_bytes(const void *bytes, size_t len);

  /**
   * \brief Send a python object.
   *
   * This function will serialize `obj` using `pickle` and send the binary
   * output.
   */
  void send_pyobj(const py::object &obj);

  /**
   * Called by the client to indicate that this `sender` is about to be
   * shared with another process.
   */
  void fork();

private:
  /**
   * \brief Private constructor to be used by the friend functions.
   */
  sender(int socket_fd, shared_buffer shared_mem, std::atomic_uint32_t *ref_cnt,
         std::atomic_uint32_t *local_ref_cnt)
      : socket_fd(socket_fd), shared_mem(std::move(shared_mem)),
        ref_cnt(ref_cnt), local_ref_cnt(local_ref_cnt) {}

  int socket_fd;
  shared_buffer shared_mem;            // buffer used to hold large messages
  std::atomic_uint32_t *ref_cnt;       // global reference counter
  std::atomic_uint32_t *local_ref_cnt; // process local reference counter
};

/**
 * \brief The receiving part of an IPC channel, with (semi-automatic) reference
 * counting support.
 *
 * Note that the `receive_*()` functions do not protect against concurrent
 * accesses.
 *
 * The support for reference counting is "semi-automatic" in the sense that
 * the `receiver::fork()` function must be called right before calling
 * the system `fork()`.
 */
class receiver {
public:
  /**
   * \brief No default constructor.
   */
  receiver() = delete;

  /**
   * \brief Destructor implementing reference counting.
   */
  ~receiver();

  /**
   * \brief Copy constructor implementing reference counting.
   */
  receiver(const receiver &t);

  /**
   * \brief No copy assignment operator.
   */
  receiver &operator=(const receiver &t) = delete;

  /**
   * \brief Move constructor implementing reference counting.
   */
  receiver(receiver &&t) noexcept;

  /**
   * \brief No move assignment operator.
   */
  receiver &operator=(receiver &&t) = delete;

  /**
   * \brief Create a synchronous channel.
   *
   * Here, synchronous means that the sender will block if the channel is full,
   * and the receiver will block if there's no incoming messages.
   *
   * \returns Two pairs of `(sender, receiver)`. One for each communicating
   * party.
   */
  friend std::tuple<sender, receiver, sender, receiver>
  sync_channel(size_t buffer_size);

  /**
   * \brief Receive some bytes.
   *
   * \param len Number of bytes to receive.
   *
   * \returns The received bytes wrapped in a `buffer`.
   */
  buffer receive_bytes(size_t len);

  /**
   * \brief Receive a python object.
   *
   * This function will receive some bytes and deserialize them using `pickle`.
   */
  py::object receive_pyobj();

  /**
   * Called by the client to indicate that this `receiver` is about to be
   * shared with another process.
   */
  void fork();

private:
  /**
   * \brief Private constructor to be used by the friend functions.
   */
  receiver(int socket_fd, shared_buffer shared_mem,
           std::atomic_uint32_t *ref_cnt, std::atomic_uint32_t *local_ref_cnt)
      : socket_fd(socket_fd), shared_mem(std::move(shared_mem)),
        ref_cnt(ref_cnt), local_ref_cnt(local_ref_cnt) {}

  int socket_fd;
  shared_buffer shared_mem;            // buffer used to hold large messages
  std::atomic_uint32_t *ref_cnt;       // global reference counter
  std::atomic_uint32_t *local_ref_cnt; // process local reference counter
};

/**
 * \brief Create a synchronous channel with buffer size `DEFAULT_CHANNEL_SIZE`.
 *
 * Here, synchronous means that the sender will block if the channel is full,
 * and the receiver will block if there's no incoming messages.
 *
 * \returns Two pairs of `(sender, receiver)`. One for each communicating
 * party.
 */
std::tuple<sender, receiver, sender, receiver> sync_channel();

/**
 * \brief Create a synchronous channel.
 *
 * Here, synchronous means that the sender will block if the channel is full,
 * and the receiver will block if there's no incoming messages.
 *
 * \param buffer_size The size of the channel buffer, which is allocated as
 * shared memory.
 *
 * \returns Two pairs of `(sender, receiver)`. One for each communicating
 * party.
 */
std::tuple<sender, receiver, sender, receiver> sync_channel(size_t buffer_size);

} // namespace snakefish

#endif // SNAKEFISH_CHANNEL_H
