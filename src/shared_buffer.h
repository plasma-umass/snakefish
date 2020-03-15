/**
 * \file shared_buffer.h
 */

#ifndef SNAKEFISH_SHARED_BUFFER_H
#define SNAKEFISH_SHARED_BUFFER_H

namespace snakefish {

/**
 * \brief A wrapper around mmap'd shared memory with built-in synchronization
 * and reference counting support.
 *
 * `shared_buffer` is like a stream of bytes. Once a byte is read, it's
 * consumed and no longer available.
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

protected:
  void *shared_mem;              // shared memory, used as a ring buffer
  std::atomic_uint32_t *ref_cnt; // reference counter
  std::atomic_flag *lock;        // "mutex" for the shared memory
  size_t *start;                 // index of first used byte
  size_t *end;                   // index of first unused byte
  bool *full;                    // is the buffer full
  size_t capacity;               // max # of bytes this buffer can hold
};

} // namespace snakefish

#endif // SNAKEFISH_SHARED_BUFFER_H
