#ifndef SNAKEFISH_SHARED_BUFFER_TESTS_H
#define SNAKEFISH_SHARED_BUFFER_TESTS_H

#include <thread>

#include <gtest/gtest.h>

#include "shared_buffer.h"
#include "util.h"
using namespace snakefish;

class shared_buffer_test : public shared_buffer {
public:
  using shared_buffer::shared_mem;
  using shared_buffer::ref_cnt;
  using shared_buffer::local_ref_cnt;
  using shared_buffer::lock;
  using shared_buffer::start;
  using shared_buffer::end;
  using shared_buffer::full;
  using shared_buffer::capacity;
  using shared_buffer::shared_buffer;
};

TEST(SharedBufferTest, ReadWriteNoWrapping) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes = get_random_bytes(capacity - 1);
  buffer copy = duplicate_bytes(bytes.get_ptr(), capacity - 1);
  shared_buf.write(bytes.get_ptr(), capacity - 1);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, capacity - 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes = buffer(capacity - 1, buffer_type::MALLOC);
  shared_buf.read(read_bytes.get_ptr(), capacity - 1);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity - 1), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity - 1);
  ASSERT_EQ(*shared_buf.end, capacity - 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, ReadWriteWithWrapping) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes = get_random_bytes(capacity);
  buffer copy = duplicate_bytes(bytes.get_ptr(), capacity);
  shared_buf.write(bytes.get_ptr(), capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, true);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes = buffer(capacity, buffer_type::MALLOC);
  shared_buf.read(read_bytes.get_ptr(), capacity);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, ReadWriteAfterWrapping) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes = get_random_bytes(capacity / 2);
  buffer copy = duplicate_bytes(bytes.get_ptr(), capacity / 2);
  shared_buf.write(bytes.get_ptr(), capacity / 2);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes = buffer(capacity / 2, buffer_type::MALLOC);
  shared_buf.read(read_bytes.get_ptr(), capacity / 2);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity / 2), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes2 = get_random_bytes(capacity);
  buffer copy2 = duplicate_bytes(bytes2.get_ptr(), capacity);
  shared_buf.write(bytes2.get_ptr(), capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, true);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes2 = buffer(capacity, buffer_type::MALLOC);
  shared_buf.read(read_bytes2.get_ptr(), capacity);
  ASSERT_EQ(memcmp(copy2.get_ptr(), read_bytes2.get_ptr(), capacity), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, PartialReads) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes = get_random_bytes(capacity);
  buffer copy = duplicate_bytes(bytes.get_ptr(), capacity);
  shared_buf.write(bytes.get_ptr(), capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, true);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes = buffer(capacity, buffer_type::MALLOC);
  shared_buf.read(read_bytes.get_ptr(), capacity / 2);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity / 2), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  shared_buf.read(static_cast<char *>(read_bytes.get_ptr()) + capacity / 2,
                  capacity / 2);
  ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity), 0);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, WriteOverflow) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  // without wrapping
  buffer bytes = get_random_bytes(capacity + 1);
  try {
    shared_buf.write(bytes.get_ptr(), capacity + 1);
  } catch (const std::runtime_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  // with wrapping
  buffer bytes2 = get_random_bytes(capacity / 2);
  shared_buf.write(bytes2.get_ptr(), capacity / 2);

  buffer read_bytes = buffer(capacity / 2, buffer_type::MALLOC);
  shared_buf.read(read_bytes.get_ptr(), capacity / 2);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  shared_buf.write(bytes2.get_ptr(), capacity / 2);
  shared_buf.write(bytes2.get_ptr(), 1);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  try {
    shared_buf.write(bytes2.get_ptr(), capacity / 2);
  } catch (const std::runtime_error &e) {
    ASSERT_EQ(std::string(e.what()), "channel buffer is full");
  }
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, ReadOutOfBound) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  // without wrapping
  buffer read_bytes = buffer(1, buffer_type::MALLOC);
  try {
    shared_buf.read(read_bytes.get_ptr(), 1);
  } catch (const std::runtime_error &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  // with wrapping
  buffer bytes = get_random_bytes(capacity / 2);
  shared_buf.write(bytes.get_ptr(), capacity / 2);

  buffer read_bytes2 = buffer(capacity / 2, buffer_type::MALLOC);
  shared_buf.read(read_bytes2.get_ptr(), capacity / 2);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, capacity / 2);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  shared_buf.write(bytes.get_ptr(), capacity / 2);
  shared_buf.write(bytes.get_ptr(), 1);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer read_bytes3 = buffer(capacity / 2 + 2, buffer_type::MALLOC);
  try {
    shared_buf.read(read_bytes3.get_ptr(), capacity / 2 + 2);
  } catch (const std::runtime_error &e) {
    ASSERT_EQ(std::string(e.what()), "out-of-bounds read detected");
  }
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, capacity / 2);
  ASSERT_EQ(*shared_buf.end, 1);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);
}

TEST(SharedBufferTest, IpcReadWrite) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  buffer bytes = get_random_bytes(capacity);
  buffer copy = duplicate_bytes(bytes.get_ptr(), capacity);

  shared_buf.fork();
  pid_t result = fork();
  if (result == 0) {
    // child writes
    shared_buf.write(bytes.get_ptr(), capacity);
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 2);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, true);
    ASSERT_EQ(shared_buf.capacity, capacity);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    shared_buf.~shared_buffer_test();
    exit(0);
  } else if (result > 0) {
    // sleep for 100 ms so the write can complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // parent reads
    buffer read_bytes = buffer(capacity, buffer_type::MALLOC);
    shared_buf.read(read_bytes.get_ptr(), capacity);
    ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity), 0);
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 1);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, false);
    ASSERT_EQ(shared_buf.capacity, capacity);

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
    ASSERT_EQ(memcmp(copy.get_ptr(), read_bytes.get_ptr(), capacity), 0);
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 1);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, false);
    ASSERT_EQ(shared_buf.capacity, capacity);
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(SharedBufferTest, MultiProcessSharing) {
  size_t capacity = 1024;
  shared_buffer_test shared_buf = shared_buffer_test(capacity);
  ASSERT_NE(shared_buf.shared_mem, nullptr);
  ASSERT_EQ(*shared_buf.ref_cnt, 1);
  ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
  ASSERT_EQ(*shared_buf.start, 0);
  ASSERT_EQ(*shared_buf.end, 0);
  ASSERT_EQ(*shared_buf.full, false);
  ASSERT_EQ(shared_buf.capacity, capacity);

  // parent forks
  shared_buf.fork();
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
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 1);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, false);
    ASSERT_EQ(shared_buf.capacity, capacity);
  } else {
    // before child make a copy of the shared buffer
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 2);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 1);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, false);
    ASSERT_EQ(shared_buf.capacity, capacity);

    shared_buffer_test shared_buf2 = shared_buf;

    // after child make a copy of the shared buffer
    ASSERT_NE(shared_buf.shared_mem, nullptr);
    ASSERT_EQ(*shared_buf.ref_cnt, 3);
    ASSERT_EQ(*shared_buf.local_ref_cnt, 2);
    ASSERT_EQ(*shared_buf.start, 0);
    ASSERT_EQ(*shared_buf.end, 0);
    ASSERT_EQ(*shared_buf.full, false);
    ASSERT_EQ(shared_buf.capacity, capacity);

    // child forks
    shared_buf.fork();
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
      ASSERT_NE(shared_buf.shared_mem, nullptr);
      ASSERT_EQ(*shared_buf.ref_cnt, 3);
      ASSERT_EQ(*shared_buf.local_ref_cnt, 2);
      ASSERT_EQ(*shared_buf.start, 0);
      ASSERT_EQ(*shared_buf.end, 0);
      ASSERT_EQ(*shared_buf.full, false);
      ASSERT_EQ(shared_buf.capacity, capacity);

      // child exits
      // cleanup necessary because d'tors won't be called when calling exit()
      shared_buf.~shared_buffer_test();
      shared_buf2.~shared_buffer_test();
      exit(0);
    } else {
      // grandchild
      ASSERT_NE(shared_buf.shared_mem, nullptr);
      ASSERT_EQ(*shared_buf.ref_cnt, 5);
      ASSERT_EQ(*shared_buf.local_ref_cnt, 2);
      ASSERT_EQ(*shared_buf.start, 0);
      ASSERT_EQ(*shared_buf.end, 0);
      ASSERT_EQ(*shared_buf.full, false);
      ASSERT_EQ(shared_buf.capacity, capacity);

      // grandchild exits
      // cleanup necessary because d'tors won't be called when calling exit()
      shared_buf.~shared_buffer_test();
      shared_buf2.~shared_buffer_test();
      exit(0);
    }
  }
}

#endif // SNAKEFISH_SHARED_BUFFER_TESTS_H
