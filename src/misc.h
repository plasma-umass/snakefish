/**
 * \file misc.h
 *
 * \brief Standalone, top-level functions.
 */

#ifndef SNAKEFISH_MISC_H
#define SNAKEFISH_MISC_H

#include <cstdint>
#include <vector>
#include <x86intrin.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

namespace snakefish {

/**
 * \brief Get a high resolution timestamp.
 *
 * The timestamp is obtained from [`rdtsc`]
 * (https://www.felixcloutier.com/x86/rdtsc). It may be used to establish an
 * ordering of IPC messages.
 *
 * \return The high resolution timestamp.
 */
inline uint64_t get_timestamp() { return __rdtsc(); }

/**
 * \brief Get a serialized high resolution timestamp.
 *
 * This function is like `get_timestamp()`, but with `lfence` and compiler
 * fence applied. For most use cases, this is probably not needed, and
 * `get_timestamp()` would be sufficient.
 *
 * \return The high resolution timestamp.
 */
inline uint64_t get_timestamp_serialized() {
  asm volatile("lfence" ::: "memory");
  return __rdtsc();
}

/**
 * \brief `map(f, args)` executed in parallel, with no global variable merging.
 *
 * \param f The Python function that should be applied to each argument.
 *
 * \param args The arguments as a Python iterable.
 *
 * \param concurrency The level of concurrency. If not supplied, this is set
 * to the number of cores in the system.
 *
 * \param chunksize The size of each process' job. If not supplied, `args` are
 * handed out evenly to each process.
 *
 * \return The return values as a `vector` (or a `list` in Python).
 */
std::vector<py::object> map(const py::function &f, const py::iterable &args,
                            uint concurrency = 0, uint chunksize = 0);

/**
 * \brief `map(f, args)` executed in parallel, with global variable merging.
 *
 * \param f The Python function that should be applied to each argument.
 *
 * \param args The arguments as a Python iterable.
 *
 * \param concurrency The level of concurrency. If not supplied, this is set
 * to the number of cores in the system.
 *
 * \param chunksize The size of each process' job. If not supplied, `args` are
 * handed out evenly to each process.
 *
 * \return The return values as a `vector` (or a `list` in Python).
 */
std::vector<py::object> map_merge(const py::function &f,
                                  const py::iterable &args,
                                  py::function extract, py::function merge,
                                  uint concurrency = 0, uint chunksize = 0);

/**
 * \brief `starmap(f, args)` executed in parallel, with no global variable
 * merging.
 *
 * \param f The Python function that should be applied to each argument (after
 * unpacking).
 *
 * \param args The arguments as a Python iterable.
 *
 * \param concurrency The level of concurrency. If not supplied, this is set
 * to the number of cores in the system.
 *
 * \param chunksize The size of each process' job. If not supplied, `args` are
 * handed out evenly to each process.
 *
 * \return The return values as a `vector` (or a `list` in Python).
 */
std::vector<py::object> starmap(const py::function &f, const py::iterable &args,
                                uint concurrency = 0, uint chunksize = 0);

/**
 * \brief `starmap(f, args)` executed in parallel, with global variable merging.
 *
 * \param f The Python function that should be applied to each argument (after
 * unpacking).
 *
 * \param args The arguments as a Python iterable.
 *
 * \param concurrency The level of concurrency. If not supplied, this is set
 * to the number of cores in the system.
 *
 * \param chunksize The size of each process' job. If not supplied, `args` are
 * handed out evenly to each process.
 *
 * \return The return values as a `vector` (or a `list` in Python).
 */
std::vector<py::object> starmap_merge(const py::function &f,
                                      const py::iterable &args,
                                      py::function extract, py::function merge,
                                      uint concurrency = 0, uint chunksize = 0);

} // namespace snakefish

#endif // SNAKEFISH_MISC_H
