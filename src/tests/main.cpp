#include <gtest/gtest.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "channel.h"
using namespace snakefish;

TEST(ChannelTest, TransferSmallObj) {
  std::tuple<sender, receiver, sender, receiver> channel = sync_channel();

  py::object i1 = py::cast(42);
  std::get<0>(channel).send_pyobj(i1);
  py::object i2 = std::get<3>(channel).receive_pyobj();
  ASSERT_EQ(i1, i2);

  std::get<0>(channel).dispose();
  std::get<1>(channel).dispose();
  std::get<2>(channel).dispose();
  std::get<3>(channel).dispose();
}

int main(int argc, char **argv) {
  py::scoped_interpreter guard{};

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
