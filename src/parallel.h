// A small pool of persistent worker threads for work that comes in many short
// rounds (-play: every 64 samples, the song's parts rendered side by side).
// run(n, f) calls f(0) .. f(n-1), spread over the workers and the calling
// thread, and returns when all are done. The workers wait by spinning for a
// while (a round is ~1 ms apart), then by sleeping, so an idle pool costs
// nothing.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
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

// The one pool everything renders on (-play's parts, an ensemble's voices, an organ's swell
// boxes), so that the work of all of them spreads over the same threads. Unlike WorkPool, run()
// nests: a task may itself call run() (a part renders its voices side by side). Each run takes a
// slot of its own; its caller does its own items first, then helps any other run while it waits
// (it never waits on work nobody has taken: what it has not taken it does itself); idle workers
// take items from every open slot. PJ_THREADS (default: the cores, at most 32; PJ_PLAY_THREADS
// is also read) sets the threads, the caller's included; 1: everything on the caller's thread.
class TaskPool {
 public:
  explicit TaskPool(int threads) {
    for (int i = 1; i < threads; ++i) workers_.emplace_back([this] { loop(); });
  }
  ~TaskPool() {
    {
      std::lock_guard<std::mutex> lk(m_);
      quit_.store(true, std::memory_order_seq_cst);
      gen_.fetch_add(1, std::memory_order_seq_cst);
    }
    cv_.notify_all();
    for (std::thread& t : workers_) t.join();
  }
  TaskPool(const TaskPool&) = delete;
  TaskPool& operator=(const TaskPool&) = delete;
  int threads() const { return int(workers_.size()) + 1; }

  static TaskPool& shared() {
    static TaskPool pool([] {
      const char* e = std::getenv("PJ_THREADS");
      if (!e || !*e) e = std::getenv("PJ_PLAY_THREADS");
      const int hw = std::max(1, int(std::thread::hardware_concurrency()));
      return std::max(1, e && *e ? std::atoi(e) : std::min(32, hw));
    }());
    return pool;
  }

  void run(int n, const std::function<void(int)>& f) {
    if (n <= 0) return;
    Slot* s = nullptr;
    if (n > 1 && !workers_.empty())
      for (Slot& c : slots_) {
        int free = 0;
        if (c.owned.compare_exchange_strong(free, 1, std::memory_order_acq_rel)) {
          s = &c;
          break;
        }
      }
    if (!s) {  // (one item, no workers, or every slot taken: here, in order)
      for (int i = 0; i < n; ++i) f(i);
      return;
    }
    s->f = &f;
    s->n = n;
    s->next.store(0, std::memory_order_relaxed);
    s->done.store(0, std::memory_order_relaxed);
    s->open.store(true, std::memory_order_seq_cst);
    {
      std::lock_guard<std::mutex> lk(m_);
      gen_.fetch_add(1, std::memory_order_seq_cst);
    }
    // (wake no more sleepers than there are items for others)
    for (int w = std::min(n - 1, sleepers_.load(std::memory_order_seq_cst)); w > 0; --w) cv_.notify_one();
    for (int i; (i = s->next.fetch_add(1, std::memory_order_acq_rel)) < n;) {
      f(i);
      s->done.fetch_add(1, std::memory_order_acq_rel);
    }
    while (s->done.load(std::memory_order_acquire) < n)
      if (!help())
        for (int k = 0; k < 16 && s->done.load(std::memory_order_acquire) < n; ++k) spin();
    // Closed; free once no helper is still looking at it.
    s->open.store(false, std::memory_order_seq_cst);
    while (s->users.load(std::memory_order_seq_cst) != 0) spin();
    s->owned.store(0, std::memory_order_release);
  }

 private:
  struct Slot {
    std::atomic<int> owned{0};     // a run holds it
    std::atomic<bool> open{false}; // its items may be taken
    std::atomic<int> users{0};     // helpers looking at it
    const std::function<void(int)>* f = nullptr;
    int n = 0;
    std::atomic<int> next{0}, done{0};
  };
  static constexpr int kSlots = 64;

  static void spin() {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
  }
  static int spinUs() {
    static const int us = std::getenv("PJ_POOL_SPIN_US") ? std::atoi(std::getenv("PJ_POOL_SPIN_US")) : 0;
    return us;
  }
  // One item from any open run; false if there was none.
  bool help() {
    const unsigned start = rot_.fetch_add(1, std::memory_order_relaxed);
    for (int k = 0; k < kSlots; ++k) {
      Slot& s = slots_[(start + k) % kSlots];
      if (!s.open.load(std::memory_order_acquire)) continue;
      s.users.fetch_add(1, std::memory_order_seq_cst);
      bool did = false;
      if (s.open.load(std::memory_order_seq_cst) && s.next.load(std::memory_order_relaxed) < s.n) {
        const int i = s.next.fetch_add(1, std::memory_order_acq_rel);
        if (i < s.n) {
          (*s.f)(i);
          s.done.fetch_add(1, std::memory_order_acq_rel);
          did = true;
        }
      }
      s.users.fetch_sub(1, std::memory_order_seq_cst);
      if (did) return true;
    }
    return false;
  }
  void loop() {
    for (;;) {
      if (quit_.load(std::memory_order_acquire)) return;
      if (help()) continue;
      // Nothing to take: watch for a new run (one load a turn, not a look at every slot) for
      // ~20 us, then sleep; a run wakes as many sleepers as it has items for others. (Spinning
      // longer kept every idle thread busy: 16 threads burned 7 x the work's CPU.)
      const unsigned seen = gen_.load(std::memory_order_seq_cst);
      bool fresh = false;
      const auto t0 = std::chrono::steady_clock::now();
      for (int i = 1;; ++i) {
        if (gen_.load(std::memory_order_acquire) != seen) { fresh = true; break; }
        spin();
        if ((i & 63) == 0 && std::chrono::steady_clock::now() - t0 > std::chrono::microseconds(spinUs())) break;
      }
      if (fresh || help()) continue;
      std::unique_lock<std::mutex> lk(m_);
      sleepers_.fetch_add(1, std::memory_order_seq_cst);
      cv_.wait(lk, [&] { return gen_.load(std::memory_order_seq_cst) != seen || quit_.load(std::memory_order_acquire); });
      sleepers_.fetch_sub(1, std::memory_order_seq_cst);
    }
  }

  std::vector<std::thread> workers_;
  Slot slots_[kSlots];
  std::atomic<unsigned> rot_{0};
  std::atomic<int> sleepers_{0};
  std::atomic<unsigned> gen_{0};
  std::mutex m_;
  std::condition_variable cv_;
  std::atomic<bool> quit_{false};
};

}  // namespace pajupilli
