#ifndef SNAKEFISH_CHANNEL_TESTS_H
#define SNAKEFISH_CHANNEL_TESTS_H

#include <gtest/gtest.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"
#include "test_util.h"
using namespace snakefish;

static const size_t TEST_CAPACITY = 1024;

class channel_test : public channel {
public:
  using channel::shared_mem;
  using channel::lock;
  using channel::start;
  using channel::end;
  using channel::full;
  using channel::n_unread;
  using channel::capacity;
  using channel::channel;
};

TEST(ChannelTest, ReadWriteNoWrapping) {
  size_t capacity = TEST_CAPACITY + sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY - 1);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY - 1);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY - 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), capacity - 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY - 1);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY - 1), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), capacity - 1);
  ASSERT_EQ((channel.end)->load(), capacity - 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.dispose();
}

TEST(ChannelTest, ReadWriteWithWrapping) {
  size_t capacity = TEST_CAPACITY + sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), true);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.dispose();
}

TEST(ChannelTest, ReadWriteAfterWrapping) {
  size_t capacity = TEST_CAPACITY + 2 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY / 2);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), capacity / 2);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY / 2);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY / 2), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), capacity / 2);
  ASSERT_EQ((channel.end)->load(), capacity / 2);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes2 = get_random_bytes(capacity - sizeof(size_t));
  buffer copy2 = duplicate_bytes(bytes2.get_ptr(), capacity - sizeof(size_t));
  channel.send_bytes(bytes2.get_ptr(), capacity - sizeof(size_t));
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), capacity / 2);
  ASSERT_EQ((channel.end)->load(), capacity / 2);
  ASSERT_EQ((channel.full)->load(), true);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes2 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes2.get_len(), capacity - sizeof(size_t));
  ASSERT_EQ(
      memcmp(copy2.get_ptr(), read_bytes2.get_ptr(), capacity - sizeof(size_t)),
      0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), capacity / 2);
  ASSERT_EQ((channel.end)->load(), capacity / 2);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.dispose();
}

TEST(ChannelTest, WriteOverflow) {
  size_t capacity = TEST_CAPACITY + 3 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  // without wrapping
  buffer bytes = get_random_bytes(capacity + 1);
  try {
    channel.send_bytes(bytes.get_ptr(), capacity + 1);
  } catch (const std::overflow_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  // with wrapping
  buffer bytes2 = get_random_bytes(TEST_CAPACITY / 2);
  channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes2.get_ptr(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  try {
    channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);
  } catch (const std::overflow_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.dispose();
}

TEST(ChannelTest, ReadOutOfBound) {
  size_t capacity = TEST_CAPACITY + 3 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  // without wrapping
  try {
    buffer read_bytes = channel.receive_bytes(false);
  } catch (const std::out_of_range &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  // with wrapping
  buffer bytes = get_random_bytes(TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);

  buffer read_bytes2 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes2.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes3 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes3.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), capacity - sizeof(size_t));
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes4 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes4.get_len(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 1);
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  try {
    buffer read_bytes5 = channel.receive_bytes(false);
  } catch (const std::out_of_range &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 1);
  ASSERT_EQ((channel.end)->load(), 1);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.dispose();
}

TEST(ChannelTest, IpcReadWrite) {
  size_t capacity = TEST_CAPACITY + sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ((channel.start)->load(), 0);
  ASSERT_EQ((channel.end)->load(), 0);
  ASSERT_EQ((channel.full)->load(), false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY);

  pid_t result = fork();
  if (result == 0) {
    // child writes
    channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY);
    ASSERT_NE(channel.shared_mem, nullptr);
    ASSERT_EQ((channel.start)->load(), 0);
    ASSERT_EQ((channel.end)->load(), 0);
    ASSERT_EQ((channel.full)->load(), true);
    ASSERT_EQ(channel.capacity, capacity);

    std::exit(0);
  } else if (result > 0) {
    // check child status
    int status = 0;
    if (waitpid(result, &status, 0) == -1) {
      perror("waitpid() failed");
      abort();
    } else {
      ASSERT_EQ(WIFEXITED(status), 1);
      ASSERT_EQ(WEXITSTATUS(status), 0);
    }

    // parent reads
    buffer read_bytes = channel.receive_bytes(true);
    ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY);
    ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY), 0);
    ASSERT_NE(channel.shared_mem, nullptr);
    ASSERT_EQ((channel.start)->load(), 0);
    ASSERT_EQ((channel.end)->load(), 0);
    ASSERT_EQ((channel.full)->load(), false);
    ASSERT_EQ(channel.capacity, capacity);

    // release resources
    channel.dispose();
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, TransferSmallObj) {
  channel_test channel;

  py::object i1 = py::cast(42);
  channel.send_pyobj(i1);
  py::object i2 = channel.receive_pyobj(true);
  ASSERT_EQ(i2.equal(i1), true);

  channel.dispose();
}

TEST(ChannelTest, TransferLargeObj) {
  channel_test channel;

  py::object i1 = py::eval("[i for i in range(10000)]");
  channel.send_pyobj(i1);
  py::object i2 = channel.receive_pyobj(true);
  ASSERT_EQ(i2.equal(i1), true);
  ASSERT_EQ(len(i1), 10000);

  channel.dispose();
}

TEST(ChannelTest, IpcSmallObj) {
  channel_test channel;

  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::cast(42);
    channel.send_pyobj(i1);

    std::exit(0);
  } else if (result > 0) {
    // check child status
    int status = 0;
    if (waitpid(result, &status, 0) == -1) {
      perror("waitpid() failed");
      abort();
    } else {
      ASSERT_EQ(WIFEXITED(status), 1);
      ASSERT_EQ(WEXITSTATUS(status), 0);
    }

    // parent reads
    py::object i2 = channel.receive_pyobj(false);
    ASSERT_EQ(i2.equal(py::cast(42)), true);

    // release resources
    channel.dispose();
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, IpcLargeObj) {
  channel_test channel;

  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::eval("[i for i in range(10000)]");
    channel.send_pyobj(i1);

    std::exit(0);
  } else if (result > 0) {
    // check child status
    int status = 0;
    if (waitpid(result, &status, 0) == -1) {
      perror("waitpid() failed");
      abort();
    } else {
      ASSERT_EQ(WIFEXITED(status), 1);
      ASSERT_EQ(WEXITSTATUS(status), 0);
    }

    // parent reads
    py::object i2 = channel.receive_pyobj(true);
    ASSERT_EQ(i2.equal(py::eval("[i for i in range(10000)]")), true);
    ASSERT_EQ(len(i2), 10000);

    // release resources
    channel.dispose();
  } else {
    perror("fork() failed");
    abort();
  }
}

#endif // SNAKEFISH_CHANNEL_TESTS_H
