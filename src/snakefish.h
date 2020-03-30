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
  for (auto t : snakefish::all_threads) {
    if (disposed_threads.find(t) == disposed_threads.end()) {
      t->~thread();
    }
  }
  for (auto g : snakefish::all_generators) {
    if (disposed_generators.find(g) == disposed_generators.end()) {
      g->~generator();
    }
  }
  for (auto c : snakefish::all_channels) {
    if (disposed_channels.find(c) == disposed_channels.end()) {
      c->~channel();
    }
  }
  std::exit(status);
}

} // namespace snakefish

#endif // SNAKEFISH_H
