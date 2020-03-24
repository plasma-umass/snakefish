#ifndef SNAKEFISH_CHANNEL_TESTS_H
#define SNAKEFISH_CHANNEL_TESTS_H

#include <gtest/gtest.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"
using namespace snakefish;

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
};

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
