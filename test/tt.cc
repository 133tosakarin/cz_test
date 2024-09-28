
#include <latch>
#include <iostream>
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <memory>

class Test {
 public:
  static constexpr int N = 1000000;
  Test(int works) : works_(works), threads_(works_), data_(1000000, 1), s_mutexs_(1000000), latch_(works) {}

  ~Test() {
    for (auto &&thr : threads_) {
      thr->join();
    }
  }

  void Start() {
    for (int i = 0; i < works_; ++i) {
      threads_[i] = std::make_unique<std::thread>([this, i] { this->Run(i); });
    }
  }


 private:

  void Run(int id) {
    auto contains = [](int i, std::vector<int>& idxs) {
      for (int j : idxs) {
        if (i == j) {
          return true;
        }
      }
      return false;
    };
    latch_.count_down();
    std::fprintf(stdout, "%d: thread  start\n", id);
    latch_.wait();
    for (int i = 0; i < 10000; ++i) {
      int idx = rand_.GetRandom();
      int idx2 = (idx + 1) % N;
      int idx3 = (idx + 2) % N;
      int j = rand_.GetRandom();

      // for reduce the probalility of deadlock, we simply skip the case that j in (idx, idx2, idx3)
      std::vector<int> idxs = {idx, idx2, idx3};
      if (contains(j, idxs)) {
        continue;
      }
      int sum = 0;
      std::sort(idxs.begin(), idxs.end());
      auto s_locks = Read(idxs, &sum);
      Write(j, sum);
    }
  }

  void Write(int idx, int sum) {
    std::fprintf(stdout, "Write (%d, %d)\n", idx, sum);
    std::unique_lock<std::shared_mutex> lock(s_mutexs_[idx]);
    data_[idx] = sum;
  }

  std::vector<std::shared_lock<std::shared_mutex>> Read(std::vector<int>& idxs, int* sum) {
    std::vector<std::shared_lock<std::shared_mutex>> locks;
    for (int i : idxs) {
      std::fprintf(stdout, "Read (%d, %d) ", i, data_[i]);
      locks.emplace_back(s_mutexs_[i]);
    }
    puts("");
    for (int i : idxs) {
      *sum += data_[i];
    }
    return locks;
  }

  class Random {
   public:
    Random() : engine_(rd_()), rand_(0, N - 1) {}
    int GetRandom() { return rand_(engine_); }

   private:
    std::random_device rd_;
    std::default_random_engine engine_;
    std::uniform_int_distribution<int> rand_;
  };
  int works_;
  std::vector<std::unique_ptr<std::thread>> threads_;
  std::vector<int> data_;
  std::vector<std::shared_mutex> s_mutexs_;
  Random rand_;
  std::latch latch_;
  

};

int main() {
  Test test(4);
  test.Start();
}