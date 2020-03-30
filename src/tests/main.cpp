#include <gtest/gtest.h>

#include <pybind11/embed.h>
namespace py = pybind11;

#include "channel_tests.h"

int main(int argc, char **argv) {
  py::scoped_interpreter guard{};

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
