#ifndef SNAKEFISH_TEST_UTIL_H
#define SNAKEFISH_TEST_UTIL_H

#include <random>

#include "buffer.h"
using namespace snakefish;

static char get_random_byte() {
  static std::random_device rd;
  static std::mt19937 rng(rd());
  static std::uniform_int_distribution<std::mt19937::result_type> dist(
      CHAR_MIN, CHAR_MAX);
  return dist(rng);
}

static buffer get_random_bytes(const size_t len) {
  buffer buf = buffer(len, buffer_type::MALLOC);
  char *bytes = static_cast<char *>(buf.get_ptr());

  for (size_t i = 0; i < len; i++) {
    bytes[i] = get_random_byte();
  }
  return buf;
}

static buffer duplicate_bytes(const void *bytes, const size_t len) {
  buffer buf = buffer(len, buffer_type::MALLOC);
  const char *src = static_cast<const char *>(bytes);
  char *copy = static_cast<char *>(buf.get_ptr());

  for (size_t i = 0; i < len; i++) {
    copy[i] = src[i];
  }
  return buf;
}

#endif // SNAKEFISH_TEST_UTIL_H
