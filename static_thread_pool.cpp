#include <tp/static_thread_pool.hpp>

#include <tp/helpers.hpp>

#include <twist/util/thread_local.hpp>

namespace tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocal<StaticThreadPool*> pool{nullptr};

////////////////////////////////////////////////////////////////////////////////

StaticThreadPool::StaticThreadPool(size_t amount) {
  tasks_ = amount;
  for (size_t i = 0; i < amount; ++i) {
    workers_.emplace_back([&]() {
      *pool = this;
      WorkerRoutine();
    });
  }
}

StaticThreadPool::~StaticThreadPool() {
  if (!workers_.empty()) {
    WHEELS_PANIC("No join or shutdown");
  }
}

void StaticThreadPool::Submit(Task task) {
  std::lock_guard guard(mutex_);

  if (task_queue_.Put(std::move(task))) {
    ++tasks_;
  }
}

void StaticThreadPool::Join() {
  std::unique_lock lock(mutex_);
  while (tasks_ != 0) {
    notasks_.wait(lock);
  }

  task_queue_.Close();
  FinishWorkers();
}

void StaticThreadPool::Shutdown() {
  task_queue_.Cancel();
  FinishWorkers();
}

void StaticThreadPool::FinishWorkers() {
  for (auto& worker : workers_) {
    worker.join();
  }

  workers_.clear();
}

void StaticThreadPool::WorkerRoutine() {
  while (true) {
    Finish();
    auto task = task_queue_.Take();

    if (task) {
      ExecuteHere(*task);
    } else {
      break;
    }
  }
}

void StaticThreadPool::Finish() {
  std::lock_guard guard(mutex_);
  --tasks_;

  if (tasks_ == 0) {
    notasks_.notify_all();
  }
}

StaticThreadPool* StaticThreadPool::Current() {
  return *pool;
}

}  // namespace tp
