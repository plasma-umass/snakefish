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
  using channel::ref_cnt;
  using channel::local_ref_cnt;
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
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY - 1);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY - 1);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY - 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, capacity - 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY - 1);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY - 1), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, capacity - 1);
  ASSERT_EQ(*channel.end, capacity - 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);
}

TEST(ChannelTest, ReadWriteWithWrapping) {
  size_t capacity = TEST_CAPACITY + sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, true);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);
}

TEST(ChannelTest, ReadWriteAfterWrapping) {
  size_t capacity = TEST_CAPACITY + 2 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY / 2);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, capacity / 2);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY / 2);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), TEST_CAPACITY / 2), 0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, capacity / 2);
  ASSERT_EQ(*channel.end, capacity / 2);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes2 = get_random_bytes(capacity - sizeof(size_t));
  buffer copy2 = duplicate_bytes(bytes2.get_ptr(), capacity - sizeof(size_t));
  channel.send_bytes(bytes2.get_ptr(), capacity - sizeof(size_t));
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, capacity / 2);
  ASSERT_EQ(*channel.end, capacity / 2);
  ASSERT_EQ(*channel.full, true);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes2 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes2.get_len(), capacity - sizeof(size_t));
  ASSERT_EQ(
      memcmp(copy2.get_ptr(), read_bytes2.get_ptr(), capacity - sizeof(size_t)),
      0);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, capacity / 2);
  ASSERT_EQ(*channel.end, capacity / 2);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);
}

TEST(ChannelTest, WriteOverflow) {
  size_t capacity = TEST_CAPACITY + 3 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  // without wrapping
  buffer bytes = get_random_bytes(capacity + 1);
  try {
    channel.send_bytes(bytes.get_ptr(), capacity + 1);
  } catch (const std::overflow_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  // with wrapping
  buffer bytes2 = get_random_bytes(TEST_CAPACITY / 2);
  channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);

  buffer read_bytes = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.end, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes2.get_ptr(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  try {
    channel.send_bytes(bytes2.get_ptr(), TEST_CAPACITY / 2);
  } catch (const std::overflow_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);
}

TEST(ChannelTest, ReadOutOfBound) {
  size_t capacity = TEST_CAPACITY + 3 * sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  // without wrapping
  try {
    buffer read_bytes = channel.receive_bytes(false);
  } catch (const std::out_of_range &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  // with wrapping
  buffer bytes = get_random_bytes(TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);

  buffer read_bytes2 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes2.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.end, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY / 2);
  channel.send_bytes(bytes.get_ptr(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, TEST_CAPACITY / 2 + sizeof(size_t));
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes3 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes3.get_len(), TEST_CAPACITY / 2);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, capacity - sizeof(size_t));
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer read_bytes4 = channel.receive_bytes(true);
  ASSERT_EQ(read_bytes4.get_len(), 1);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 1);
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  try {
    buffer read_bytes5 = channel.receive_bytes(false);
  } catch (const std::out_of_range &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 1);
  ASSERT_EQ(*channel.end, 1);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);
}

TEST(ChannelTest, IpcReadWrite) {
  size_t capacity = TEST_CAPACITY + sizeof(size_t);
  channel_test channel = channel_test(capacity);
  ASSERT_NE(channel.shared_mem, nullptr);
  ASSERT_EQ(*channel.ref_cnt, 1);
  ASSERT_EQ(*channel.local_ref_cnt, 1);
  ASSERT_EQ(*channel.start, 0);
  ASSERT_EQ(*channel.end, 0);
  ASSERT_EQ(*channel.full, false);
  ASSERT_EQ(channel.capacity, capacity);

  buffer bytes = get_random_bytes(TEST_CAPACITY);
  buffer copy = duplicate_bytes(bytes.get_ptr(), TEST_CAPACITY);

  channel.fork();
  pid_t result = fork();
  if (result == 0) {
    // child writes
    channel.send_bytes(bytes.get_ptr(), TEST_CAPACITY);
    ASSERT_NE(channel.shared_mem, nullptr);
    ASSERT_EQ(*channel.ref_cnt, 2);
    ASSERT_EQ(*channel.local_ref_cnt, 1);
    ASSERT_EQ(*channel.start, 0);
    ASSERT_EQ(*channel.end, 0);
    ASSERT_EQ(*channel.full, true);
    ASSERT_EQ(channel.capacity, capacity);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    channel.~channel_test();
    exit(0);
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
    ASSERT_EQ(*channel.ref_cnt, 1);
    ASSERT_EQ(*channel.local_ref_cnt, 1);
    ASSERT_EQ(*channel.start, 0);
    ASSERT_EQ(*channel.end, 0);
    ASSERT_EQ(*channel.full, false);
    ASSERT_EQ(channel.capacity, capacity);
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
}

TEST(ChannelTest, TransferLargeObj) {
  channel_test channel;

  py::object i1 = py::eval("[i for i in range(10000)]");
  channel.send_pyobj(i1);
  py::object i2 = channel.receive_pyobj(true);
  ASSERT_EQ(i2.equal(i1), true);
  ASSERT_EQ(len(i1), 10000);
}

TEST(ChannelTest, IpcSmallObj) {
  channel_test channel;

  channel.fork();
  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::cast(42);
    channel.send_pyobj(i1);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    channel.~channel_test();
    exit(0);
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
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, IpcLargeObj) {
  channel_test channel;

  channel.fork();
  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::eval("[i for i in range(10000)]");
    channel.send_pyobj(i1);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    channel.~channel_test();
    exit(0);
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
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, MultiProcessSharing) {
  channel_test channel;

  // parent forks
  channel.fork();
  pid_t result = fork();
  if (result < 0) {
    perror("fork() failed");
    abort();
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

    // check again after child exits
    ASSERT_EQ(*channel.ref_cnt, 1);
    ASSERT_EQ(*channel.local_ref_cnt, 1);
  } else {
    // before child make a copy of the second channel
    ASSERT_EQ(*channel.ref_cnt, 2);
    ASSERT_EQ(*channel.local_ref_cnt, 1);

    channel_test channel_copy = channel;

    // after child make a copy of the shared buffer
    ASSERT_EQ(*channel.ref_cnt, 3);
    ASSERT_EQ(*channel.local_ref_cnt, 2);

    // child forks
    channel.fork();
    result = fork();
    if (result < 0) {
      perror("fork() failed");
      abort();
    } else if (result > 0) {
      // check grandchild status
      int status = 0;
      if (waitpid(result, &status, 0) == -1) {
        perror("waitpid() failed");
        abort();
      } else {
        ASSERT_EQ(WIFEXITED(status), 1);
        ASSERT_EQ(WEXITSTATUS(status), 0);
      }

      // check again after grandchild exits
      ASSERT_EQ(*channel.ref_cnt, 3);
      ASSERT_EQ(*channel.local_ref_cnt, 2);

      // child exits
      // cleanup necessary because d'tors won't be called when calling exit()
      channel.~channel_test();
      channel_copy.~channel_test();
      exit(0);
    } else {
      // grandchild
      ASSERT_EQ(*channel.ref_cnt, 5);
      ASSERT_EQ(*channel.local_ref_cnt, 2);

      // grandchild exits
      // cleanup necessary because d'tors won't be called when calling exit()
      channel.~channel_test();
      channel_copy.~channel_test();
      exit(0);
    }
  }
}

#endif // SNAKEFISH_CHANNEL_TESTS_H
