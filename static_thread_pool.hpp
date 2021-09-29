#pragma once

#include <tp/blocking_queue.hpp>
#include <tp/task.hpp>

#include <twist/stdlike/thread.hpp>

namespace tp {

// Fixed-size pool of worker threads

class StaticThreadPool {
 public:
  explicit StaticThreadPool(size_t workers);
  ~StaticThreadPool();

  // Non-copyable
  StaticThreadPool(const StaticThreadPool&) = delete;
  StaticThreadPool& operator=(const StaticThreadPool&) = delete;

  // Schedules task for execution in one of the worker threads
  void Submit(Task task);

  // Graceful shutdown
  // Waits until outstanding work count has reached 0
  // and joins worker threads
  void Join();

  // Hard shutdown
  // Joins worker threads ASAP
  void Shutdown();

  // Locate current thread pool from worker thread
  static StaticThreadPool* Current();

 private:
  void FinishWorkers();
  void WorkerRoutine();
  void Finish();

 private:
  twist::stdlike::mutex mutex_;
  UnboundedBlockingQueue<Task> task_queue_;
  std::vector<twist::stdlike::thread> workers_;
  twist::stdlike::condition_variable notasks_;
  int tasks_ = 0;
};

inline StaticThreadPool* Current() {
  return StaticThreadPool::Current();
}

}  // namespace tp
