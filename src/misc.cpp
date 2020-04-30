#include <thread>

#include "misc.h"
#include "thread.h"

namespace snakefish {

static std::vector<py::object>
map_thread_func(const py::function &f, const std::vector<py::handle> &args) {
  std::vector<py::object> results;
  results.reserve(args.size());

  for (auto arg : args) {
    results.push_back(f(arg));
  }
  return results;
}

static std::vector<py::object>
starmap_thread_func(const py::function &f,
                    const std::vector<py::handle> &args) {
  std::vector<py::object> results;
  results.reserve(args.size());

  for (auto arg : args) {
    results.push_back(f(*arg));
  }
  return results;
}

static inline py::cpp_function
get_thread_func(const py::function &f, const std::vector<py::handle> &args,
                bool star) {
  if (star) {
    return [f, args]() { return starmap_thread_func(f, args); };
  } else {
    return [f, args]() { return map_thread_func(f, args); };
  }
}

static std::vector<py::object>
_map(const py::function &f, const py::iterable &args, py::function *extract,
     py::function *merge, uint concurrency, uint chunksize, bool star) {

  py::list arg_list = py::list(args); // assemble args

  // use default concurrency (i.e. # of physical cores)?
  if (concurrency == 0) {
    concurrency = std::thread::hardware_concurrency();
  }

  // use default chunk size?
  if (chunksize == 0) {
    chunksize = (arg_list.size() + concurrency - 1) / concurrency;
  }

  // calculate # of batches
  uint n_batch = (arg_list.size() + (concurrency * chunksize) - 1) /
                 (concurrency * chunksize);

  // run jobs
  std::vector<thread> threads;
  std::vector<py::object> results;
  std::vector<py::handle> thread_args;

  threads.reserve(concurrency);
  results.reserve(arg_list.size());
  thread_args.reserve(chunksize);

  auto iter = arg_list.begin();
  auto end = arg_list.end();

  for (uint i = 0; i < n_batch; i++) {
    for (uint j = 0; j < concurrency; j++) {
      // split args
      for (uint k = 0; k < chunksize; k++) {
        if (iter == end) {
          break;
        }
        thread_args.push_back(*iter);
        iter++;
      }

      // if thread_args is empty, then the iterator has been exhausted
      if (thread_args.empty()) {
        break;
      }

      // spawn thread
      if ((extract != nullptr) && (merge != nullptr)) {
        // with merging
        thread t(get_thread_func(f, thread_args, star), *extract, *merge);
        t.start();
        threads.push_back(std::move(t));
      } else {
        // without merging
        thread t(get_thread_func(f, thread_args, star));
        t.start();
        threads.push_back(std::move(t));
      }

      // reset
      thread_args.clear();
    }

    // join threads and concatenate results
    for (thread &t : threads) {
      t.join();

      auto thread_results = py::cast<std::vector<py::object>>(t.get_result());
      std::move(std::begin(thread_results), std::end(thread_results),
                std::back_inserter(results));

      t.dispose();
    }

    // reset
    threads.clear();
  }

  return results;
}

std::vector<py::object> map(const py::function &f, const py::iterable &args,
                            uint concurrency, uint chunksize) {
  return _map(f, args, nullptr, nullptr, concurrency, chunksize, false);
}

std::vector<py::object> map_merge(const py::function &f,
                                  const py::iterable &args,
                                  py::function extract, py::function merge,
                                  uint concurrency, uint chunksize) {
  return _map(f, args, &extract, &merge, concurrency, chunksize, false);
}

std::vector<py::object> starmap(const py::function &f, const py::iterable &args,
                                uint concurrency, uint chunksize) {
  return _map(f, args, nullptr, nullptr, concurrency, chunksize, true);
}

std::vector<py::object> starmap_merge(const py::function &f,
                                      const py::iterable &args,
                                      py::function extract, py::function merge,
                                      uint concurrency, uint chunksize) {
  return _map(f, args, &extract, &merge, concurrency, chunksize, true);
}

} // namespace snakefish
