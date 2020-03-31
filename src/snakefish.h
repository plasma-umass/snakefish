#ifndef SNAKEFISH_H
#define SNAKEFISH_H

#include "channel.h"
#include "generator.h"
#include "thread.h"

namespace snakefish {

static inline pid_t snakefish_fork() {
  for (auto c : snakefish::all_channels) {
    c->fork();
  }
  return fork();
}

// d'tors won't be called when calling exit()
// so call them manually
static inline void snakefish_exit(const int status) {
  // make copies to avoid modifications during iterations
  std::set<thread *> threads(snakefish::all_threads);
  for (auto &t : threads) {
    t->~thread();
  }

  std::set<generator *> generators(snakefish::all_generators);
  for (auto &g : generators) {
    g->~generator();
  }

  std::set<channel *> channels(snakefish::all_channels);
  for (auto &c : channels) {
    c->~channel();
  }

  std::exit(status);
}

} // namespace snakefish

#endif // SNAKEFISH_H
