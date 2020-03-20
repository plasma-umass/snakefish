/**
 * \file shared_buffer.h
 */

#ifndef SNAKEFISH_SHARED_BUFFER_H
#define SNAKEFISH_SHARED_BUFFER_H

namespace snakefish {

/**
 * \brief A wrapper around mmap'd shared memory with built-in synchronization
 * and (semi-automatic) reference counting support.
 *
 * `shared_buffer` is like a stream of bytes. Once a byte is read, it's
 * consumed and no longer available.
 *
 * The support for reference counting is "semi-automatic" in the sense that
 * the `shared_buffer::fork()` function must be called right before calling
 * the system `fork()`.
 */
class shared_buffer {
public:
  /**
   * \brief No default constructor.
   */
  shared_buffer() = delete;

  /**
   * \brief Destructor implementing reference counting.
   */
  ~shared_buffer();

  /**
   * \brief Copy constructor implementing reference counting.
   */
  shared_buffer(const shared_buffer &t);

  /**
   * \brief No copy assignment operator.
   */
  shared_buffer &operator=(const shared_buffer &t) = delete;

  /**
   * \brief Move constructor implementing reference counting.
   */
  shared_buffer(shared_buffer &&t) noexcept;

  /**
   * \brief No move assignment operator.
   */
  shared_buffer &operator=(shared_buffer &&t) = delete;

  /**
   * \brief Create a new shared buffer.
   */
  explicit shared_buffer(size_t len);

  /**
   * \brief Copy `len` bytes from the underlying buffer into `buf`.
   */
  void read(void *buf, size_t n);

  /**
   * \brief Copy `n` bytes from `buf` into the underlying buffer.
   */
  void write(const void *buf, size_t n);

  /**
   * Called by the client to indicate that this `shared_buffer` is about to be
   * shared with another process.
   */
  void fork();

protected:
  /**
   * \brief Shared memory, used as a ring buffer.
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
   * \brief Number of bytes this buffer can hold.
   */
  size_t capacity;
};

} // namespace snakefish

#endif // SNAKEFISH_SHARED_BUFFER_H
