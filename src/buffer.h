#ifndef SNAKEFISH_BUFFER_H
#define SNAKEFISH_BUFFER_H

#include <sys/mman.h>

namespace snakefish {

/*
 * An enum describing how the buffer was allocated.
 */
enum buffer_type { MALLOC, MMAP };

/*
 * A wrapper around dynamically allocated memory buffers.
 *
 * This makes memory management easier.
 */
class buffer {
public:
  /*
   * No default constructor.
   */
  buffer() = delete;

  /*
   * No copy constructor.
   */
  buffer(const buffer &t) = delete;

  /*
   * No copy assignment operator.
   */
  buffer &operator=(const buffer &t) = delete;

  /*
   * Default move constructor.
   */
  buffer(buffer &&t) = default;

  /*
   * No move assignment operator.
   */
  buffer &operator=(buffer &&t) = delete;

  /*
   * Create a new buffer.
   *
   * If type is `MALLOC`, the underlying memory buffer will be allocated
   * using the default `malloc()`.
   *
   * If type is `MMAP`, the underlying memory buffer will be allocated using
   * `mmap()` with `PROT_READ | PROT_WRITE` and `MAP_PRIVATE | MAP_ANONYMOUS`.
   */
  buffer(size_t len, buffer_type type);

  /*
   * Destructor.
   */
  ~buffer();

  /*
   * Return a pointer to the start of the underlying memory buffer.
   */
  void *get_ptr() { return buf; }

  /*
   * Return the length of the underlying memory buffer.
   */
  size_t get_len() { return len; }

  /*
   * Return the type of the underlying memory buffer.
   */
  buffer_type get_type() { return type; }

private:
  void *buf;
  size_t len;
  buffer_type type;
};

} // namespace snakefish

#endif // SNAKEFISH_BUFFER_H
