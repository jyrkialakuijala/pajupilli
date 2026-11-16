// A small pool of persistent worker threads for work that comes in many short
// rounds (-play: every 64 samples, the song's parts rendered side by side).
// run(n, f) calls f(0) .. f(n-1), spread over the workers and the calling
// thread, and returns when all are done. The workers wait by spinning for a
// while (a round is ~1 ms apart), then by sleeping, so an idle pool costs
// nothing.
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace pajupilli {

class WorkPool {
 public:
  explicit WorkPool(int threads) {
    for (int i = 1; i < threads; ++i) workers_.emplace_back([this] { loop(); });
  }
  ~WorkPool() {
    {
      std::lock_guard<std::mutex> lk(m_);
      quit_.store(true, std::memory_order_release);
      gen_.fetch_add(1, std::memory_order_release);
    }
    cv_.notify_all();
    for (std::thread& t : workers_) t.join();
  }
  WorkPool(const WorkPool&) = delete;
  WorkPool& operator=(const WorkPool&) = delete;
  int threads() const { return int(workers_.size()) + 1; }

  void run(int n, const std::function<void(int)>& f) {
    if (n <= 0) return;
    if (workers_.empty() || n == 1) {
      for (int i = 0; i < n; ++i) f(i);
      return;
    }
    // (A worker late from the last round may already take items of this one:
    // the job is published before the counter it takes them by.)
    f_.store(&f, std::memory_order_relaxed);
    n_.store(n, std::memory_order_relaxed);
    done_.store(0, std::memory_order_relaxed);
    next_.store(0, std::memory_order_release);
    {
      std::lock_guard<std::mutex> lk(m_);
      gen_.fetch_add(1, std::memory_order_release);
    }
    if (sleepers_.load(std::memory_order_acquire) > 0) cv_.notify_all();
    work();
    while (done_.load(std::memory_order_acquire) < n) spin();
  }

 private:
  static void spin() {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
  }
  void work() {
    for (int i; (i = next_.fetch_add(1, std::memory_order_acq_rel)) < n_.load(std::memory_order_relaxed);) {
      (*f_.load(std::memory_order_relaxed))(i);
      done_.fetch_add(1, std::memory_order_acq_rel);
    }
  }
  void loop() {
    unsigned seen = gen_.load(std::memory_order_acquire);
    for (;;) {
      // Spin a while (~50 us) for the next round, then sleep.
      int tries = 0;
      while (gen_.load(std::memory_order_acquire) == seen && ++tries < 20000) spin();
      if (gen_.load(std::memory_order_acquire) == seen) {
        std::unique_lock<std::mutex> lk(m_);
        sleepers_.fetch_add(1, std::memory_order_acq_rel);
        cv_.wait(lk, [&] { return gen_.load(std::memory_order_acquire) != seen; });
        sleepers_.fetch_sub(1, std::memory_order_acq_rel);
      }
      seen = gen_.load(std::memory_order_acquire);
      if (quit_.load(std::memory_order_acquire)) return;
      work();
    }
  }

  std::vector<std::thread> workers_;
  std::atomic<const std::function<void(int)>*> f_{nullptr};
  std::atomic<int> n_{0};
  std::atomic<int> next_{0}, done_{0}, sleepers_{0};
  std::atomic<unsigned> gen_{0};
  std::mutex m_;
  std::condition_variable cv_;
  std::atomic<bool> quit_{false};
};

}  // namespace pajupilli
