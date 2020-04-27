/**
 * \file semaphore_t.h
 */

#ifndef SNAKEFISH_SEMAPHORE_T_H
#define SNAKEFISH_SEMAPHORE_T_H

#include <string>

#include <semaphore.h>

namespace snakefish {

/**
 * \brief A wrapper around OS-provided semaphores.
 *
 * Since macOS doesn't implement unnamed semaphores
 * ([ref](https://stackoverflow.com/a/27847103)), this wrapper
 * is provided to make code cleaner.
 *
 * On macOS, named semaphores will be used. Otherwise, POSIX unnamed semaphores
 * will be used.
 *
 * **NOTE**: This wrapper is intended for multi-processing, not multi-threading.
 */
class semaphore_t {
public:
  /**
   * \brief Create a semaphore with initial value 0.
   *
   * \throws std::runtime_error If `sem_init()` or `sem_open()` failed. Check
   * `errno` for details.
   * \throws std::bad_alloc If `mmap()` failed. This will NOT happen on macOS.
   */
  semaphore_t() : semaphore_t(0) {}

  /**
   * Default destructor.
   */
  ~semaphore_t() = default;

  /**
   * \brief Default copy constructor.
   */
  semaphore_t(const semaphore_t &t) = default;

  /**
   * \brief No copy assignment operator.
   */
  semaphore_t &operator=(const semaphore_t &t) = delete;

  /**
   * \brief Default move constructor.
   */
  semaphore_t(semaphore_t &&t) = default;

  /**
   * \brief No move assignment operator.
   */
  semaphore_t &operator=(semaphore_t &&t) = delete;

  /**
   * \brief Create a semaphore with initial value `val`.
   *
   * \throws std::runtime_error If `sem_init()` or `sem_open()` failed. Check
   * `errno` for details.
   * \throws std::bad_alloc If `mmap()` failed. This will NOT happen on macOS.
   */
  explicit semaphore_t(unsigned int val);

  /**
   * Increment the semaphore.
   *
   * @throws std::runtime_error If `sem_post()` failed.
   */
  void post();

  /**
   * Decrement the semaphore.
   *
   * @throws std::runtime_error If `sem_wait()` failed.
   */
  void wait();

  /**
   * Non-blocking version of `wait()`.
   *
   * @return `true` on success. `false` if the call would block.
   *
   * @throws std::runtime_error If `sem_trywait()` failed.
   */
  bool trywait();

  /**
   * Destroy this semaphore and release resources.
   *
   * @throws std::runtime_error If `sem_destroy()`, `sem_close()`,
   * `sem_unlink()`, or `munmap()` failed.
   */
  void destroy();

private:
  sem_t *sem;
#ifdef __APPLE__
  std::string name;
#endif
};

} // namespace snakefish

#endif // SNAKEFISH_SEMAPHORE_T_H
