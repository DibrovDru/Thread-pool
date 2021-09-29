#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include "task.hpp"
#include <optional>
#include <deque>

namespace tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedBlockingQueue {
 public:
  bool Put(T value) {
    std::lock_guard guard(mutex_);

    if (closed_) {
      return false;
    }

    buffer_.push_back(std::move(value));
    isempty_.notify_one();
    return true;
  }

  std::optional<T> Take() {
    std::unique_lock lock(mutex_);

    while (buffer_.empty()) {
      if (closed_) {
        return std::nullopt;
      }

      isempty_.wait(lock);
    }

    T curr = std::move(buffer_.front());
    buffer_.pop_front();
    return curr;
  }

  void Close() {
    std::lock_guard guard(mutex_);
    CloseImpl(true);
  }

  void Cancel() {
    std::lock_guard guard(mutex_);
    CloseImpl(true);
    buffer_.clear();
  }

 private:
  void CloseImpl(bool block) {
    closed_ = block;
    isempty_.notify_all();
  }

 private:
  std::deque<T> buffer_;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable isempty_;
  bool closed_ = false;
};

}  // namespace tp
