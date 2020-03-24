#include <cerrno>
#include <stdexcept>

#include <unistd.h>

#include "semaphore_t.h"
#include "util.h"

namespace snakefish {

#ifdef __APPLE__
semaphore_t::semaphore_t(unsigned int val) : name("/snakefish-") {
  name.append(std::to_string(getpid()));
  name.append("-");
  name.append(std::to_string(util::get_random_uint()));

  sem = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, val);
  if (sem == SEM_FAILED) {
    perror("sem_open() failed");
    throw std::runtime_error("sem_open() failed");
  }
}
#else
semaphore_t::semaphore_t(unsigned int val) {
  sem = static_cast<sem_t *>(util::get_shared_mem(sizeof(sem_t), true));

  if (sem_init(sem, 1, val)) {
    perror("sem_init() failed");
    throw std::runtime_error("sem_init() failed");
  }
}
#endif

void semaphore_t::post() {
  if (sem_post(sem)) {
    perror("sem_post() failed");
    throw std::runtime_error("sem_post() failed");
  }
}

void semaphore_t::wait() {
  if (sem_wait(sem)) {
    perror("sem_wait() failed");
    throw std::runtime_error("sem_wait() failed");
  }
}

bool semaphore_t::trywait() {
  if (sem_trywait(sem)) {
    if (errno == EAGAIN) {
      return false;
    } else {
      perror("sem_trywait() failed");
      throw std::runtime_error("sem_trywait() failed");
    }
  }
  return true;
}

#if __APPLE__
void semaphore_t::destroy() {
  if (sem_close(sem)) {
    perror("sem_close() failed");
    throw std::runtime_error("sem_close() failed");
  }
  if (sem_unlink(name.c_str())) {
    perror("sem_unlink() failed");
    throw std::runtime_error("sem_unlink() failed");
  }
}
#else
void semaphore_t::destroy() {
  if (sem_destroy(sem)) {
    perror("sem_destroy() failed");
    throw std::runtime_error("sem_destroy() failed");
  }
  if (munmap(sem, sizeof(sem_t))) {
    perror("munmap() failed");
    throw std::runtime_error("munmap() failed");
  }
}
#endif

} // namespace snakefish
