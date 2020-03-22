#ifndef SNAKEFISH_CHANNEL_TESTS_H
#define SNAKEFISH_CHANNEL_TESTS_H

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"
#include "util.h"
using namespace snakefish;

class channel_test : public channel {
public:
  using channel::socket_fd;
  using channel::shared_mem;
  using channel::ref_cnt;
  using channel::local_ref_cnt;
  using channel::fork_shared_mem;
  using channel::channel;

  friend std::pair<channel_test, channel_test>
  sync_channel_test(size_t channel_size);
};

std::pair<channel_test, channel_test>
sync_channel_test(const size_t channel_size) {
  // open unix domain sockets
  int socket_fd[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, socket_fd)) {
    perror("socketpair() failed");
    throw std::runtime_error("socketpair() failed");
  }

  // update settings so sockets would close on exec()
  if (fcntl(socket_fd[0], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for sender socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for sender socket");
  }
  if (fcntl(socket_fd[1], F_SETFD, FD_CLOEXEC)) {
    perror("fcntl(FD_CLOEXEC) failed for receiver socket");
    throw std::runtime_error("fcntl(FD_CLOEXEC) failed for receiver socket");
  }

  // create shared buffer
  shared_buffer shared_mem(channel_size);

  // create and initialize metadata variables
  auto ref_cnt = static_cast<std::atomic_uint32_t *>(
      util::get_shared_mem(sizeof(std::atomic_uint32_t), true));
  auto local_ref_cnt =
      static_cast<std::atomic_uint32_t *>(malloc(sizeof(std::atomic_uint32_t)));
  auto ref_cnt2 = static_cast<std::atomic_uint32_t *>(
      util::get_shared_mem(sizeof(std::atomic_uint32_t), true));
  auto local_ref_cnt2 =
      static_cast<std::atomic_uint32_t *>(malloc(sizeof(std::atomic_uint32_t)));
  *ref_cnt = 1;
  *local_ref_cnt = 1;
  *ref_cnt2 = 1;
  *local_ref_cnt2 = 1;

  return {channel_test(socket_fd[0], shared_mem, ref_cnt, local_ref_cnt, true),
          channel_test(socket_fd[1], shared_mem, ref_cnt2, local_ref_cnt2, false)};
}

TEST(ChannelTest, TransferSmallObj) {
  std::pair<channel, channel> channels = sync_channel();

  py::object i1 = py::cast(42);
  channels.first.send_pyobj(i1);
  py::object i2 = channels.second.receive_pyobj();
  ASSERT_EQ(i2.equal(i1), true);
}

TEST(ChannelTest, TransferLargeObj) {
  std::pair<channel, channel> channels = sync_channel();

  py::object i1 = py::eval("[i for i in range(10000)]");
  channels.first.send_pyobj(i1);
  py::object i2 = channels.second.receive_pyobj();
  ASSERT_EQ(i2.equal(i1), true);
  ASSERT_EQ(len(i1), 10000);
}

TEST(ChannelTest, IpcSmallObj) {
  std::pair<channel, channel> channels = sync_channel();

  channels.first.fork();
  channels.second.fork();

  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::cast(42);
    channels.first.send_pyobj(i1);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    channels.first.~channel();
    channels.second.~channel();
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
    py::object i2 = channels.second.receive_pyobj();
    ASSERT_EQ(i2.equal(py::cast(42)), true);
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, IpcLargeObj) {
  std::pair<channel, channel> channels = sync_channel();

  channels.first.fork();
  channels.second.fork();

  pid_t result = fork();
  if (result == 0) {
    // child writes
    py::object i1 = py::eval("[i for i in range(10000)]");
    channels.first.send_pyobj(i1);

    // child exits
    // cleanup necessary because d'tors won't be called when calling exit()
    channels.first.~channel();
    channels.second.~channel();
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
    py::object i2 = channels.second.receive_pyobj();
    ASSERT_EQ(i2.equal(py::eval("[i for i in range(10000)]")), true);
    ASSERT_EQ(len(i2), 10000);
  } else {
    perror("fork() failed");
    abort();
  }
}

TEST(ChannelTest, MultiProcessSharing) {
  std::pair<channel_test, channel_test> channels =
      sync_channel_test(1024 * 1024);
  std::pair<channel_test, channel_test> channels2 =
      sync_channel_test(1024 * 1024);

  // parent forks
  channels.first.fork();
  channels.second.fork();
  channels2.first.fork();
  channels2.second.fork();

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
    ASSERT_EQ(*channels.first.ref_cnt, 1);
    ASSERT_EQ(*channels.first.local_ref_cnt, 1);
    ASSERT_EQ(*channels.second.ref_cnt, 1);
    ASSERT_EQ(*channels.second.local_ref_cnt, 1);
    ASSERT_EQ(*channels2.first.ref_cnt, 1);
    ASSERT_EQ(*channels2.first.local_ref_cnt, 1);
    ASSERT_EQ(*channels2.second.ref_cnt, 1);
    ASSERT_EQ(*channels2.second.local_ref_cnt, 1);
  } else {
    // before child make a copy of the second channel
    ASSERT_EQ(*channels.first.ref_cnt, 2);
    ASSERT_EQ(*channels.first.local_ref_cnt, 1);
    ASSERT_EQ(*channels.second.ref_cnt, 2);
    ASSERT_EQ(*channels.second.local_ref_cnt, 1);
    ASSERT_EQ(*channels2.first.ref_cnt, 2);
    ASSERT_EQ(*channels2.first.local_ref_cnt, 1);
    ASSERT_EQ(*channels2.second.ref_cnt, 2);
    ASSERT_EQ(*channels2.second.local_ref_cnt, 1);

    std::pair<channel_test, channel_test> channels2_copy = channels2;

    // after child make a copy of the shared buffer
    ASSERT_EQ(*channels.first.ref_cnt, 2);
    ASSERT_EQ(*channels.first.local_ref_cnt, 1);
    ASSERT_EQ(*channels.second.ref_cnt, 2);
    ASSERT_EQ(*channels.second.local_ref_cnt, 1);
    ASSERT_EQ(*channels2.first.ref_cnt, 3);
    ASSERT_EQ(*channels2.first.local_ref_cnt, 2);
    ASSERT_EQ(*channels2.second.ref_cnt, 3);
    ASSERT_EQ(*channels2.second.local_ref_cnt, 2);

    // child forks
    channels.first.fork();
    channels.second.fork();
    channels2.first.fork();
    channels2.second.fork();

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
      ASSERT_EQ(*channels.first.ref_cnt, 2);
      ASSERT_EQ(*channels.first.local_ref_cnt, 1);
      ASSERT_EQ(*channels.second.ref_cnt, 2);
      ASSERT_EQ(*channels.second.local_ref_cnt, 1);
      ASSERT_EQ(*channels2.first.ref_cnt, 3);
      ASSERT_EQ(*channels2.first.local_ref_cnt, 2);
      ASSERT_EQ(*channels2.second.ref_cnt, 3);
      ASSERT_EQ(*channels2.second.local_ref_cnt, 2);

      // child exits
      // cleanup necessary because d'tors won't be called when calling exit()
      channels.first.~channel_test();
      channels.second.~channel_test();
      channels2.first.~channel_test();
      channels2.second.~channel_test();
      channels2_copy.first.~channel_test();
      channels2_copy.second.~channel_test();
      exit(0);
    } else {
      // grandchild
      ASSERT_EQ(*channels.first.ref_cnt, 3);
      ASSERT_EQ(*channels.first.local_ref_cnt, 1);
      ASSERT_EQ(*channels.second.ref_cnt, 3);
      ASSERT_EQ(*channels.second.local_ref_cnt, 1);
      ASSERT_EQ(*channels2.first.ref_cnt, 5);
      ASSERT_EQ(*channels2.first.local_ref_cnt, 2);
      ASSERT_EQ(*channels2.second.ref_cnt, 5);
      ASSERT_EQ(*channels2.second.local_ref_cnt, 2);

      // grandchild exits
      // cleanup necessary because d'tors won't be called when calling exit()
      channels.first.~channel_test();
      channels.second.~channel_test();
      channels2.first.~channel_test();
      channels2.second.~channel_test();
      channels2_copy.first.~channel_test();
      channels2_copy.second.~channel_test();
      exit(0);
    }
  }
}

#endif // SNAKEFISH_CHANNEL_TESTS_H
