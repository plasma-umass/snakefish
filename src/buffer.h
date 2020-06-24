/**
 * \file buffer.h
 */

#ifndef SNAKEFISH_BUFFER_H
#define SNAKEFISH_BUFFER_H

#include <sys/mman.h>

namespace snakefish {

/**
 * \brief An enum indicating the allocation function of `buffer`.
 */
enum buffer_type { MALLOC, MMAP };

/**
 * \brief A wrapper around dynamically allocated memory.
 *
 * This makes memory management easier.
 */
class buffer {
public:
  /**
   * \brief No default constructor.
   */
  buffer() = delete;

  /**
   * \brief No copy constructor.
   */
  buffer(const buffer &t) = delete;

  /**
   * \brief No copy assignment operator.
   */
  buffer &operator=(const buffer &t) = delete;

  /**
   * \brief Default move constructor.
   */
  buffer(buffer &&t) = default;

  /**
   * \brief No move assignment operator.
   */
  buffer &operator=(buffer &&t) = delete;

  /**
   * \brief Create a new buffer.
   *
   * If type is `MALLOC`, the underlying memory buffer will be allocated
   * using the default `malloc()`.
   *
   * If type is `MMAP`, the underlying memory buffer will be allocated using
   * `mmap()` with `PROT_READ | PROT_WRITE` and `MAP_PRIVATE | MAP_ANONYMOUS`.
   *
   * \throws std::bad_alloc If `malloc()` or `mmap()` failed.
   */
  buffer(size_t len, buffer_type type);

  /**
   * \brief Destructor.
   */
  ~buffer();

  /**
   * \brief Get a pointer to the start of the underlying memory buffer.
   */
  void *get_ptr() { return buf; }

  /**
   * \brief Get the length (in bytes) of the underlying memory buffer.
   */
  size_t get_len() { return len; }

  /**
   * \brief Get the type of the underlying memory buffer.
   */
  buffer_type get_type() { return type; }

private:
  void *buf;
  size_t len;
  buffer_type type;
};

} // namespace snakefish

#endif // SNAKEFISH_BUFFER_H
