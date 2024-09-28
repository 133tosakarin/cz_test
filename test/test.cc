#include <fstream>
#include <random>
#include <thread>
#include <vector>
#include <print>
#include <latch>
#include "concurrency/lock_manager.h"
#include "concurrency/transaction.h"
#include "concurrency/transaction_manager.h"
constexpr int N = 1000000;
class Test {
 public:
  Test(int works)
      : vec_(N, 1),
        workers_(works),
        threads_(works),
        lock_mgr_{std::make_unique<LockManager>()},
        txn_mgr_{std::make_unique<TransactionManager>(lock_mgr_.get())}, latch_(works) {}

  ~Test() {
    for (auto &&thr : threads_) {
      thr->join();
    }
  }

  std::pair<int, int> GetRandom() {
    return {rand_.GetRandom(), rand_.GetRandom()};
  }

  void Start() {
    for (int i = 0; i < workers_; ++i) {
      threads_[i] = std::make_unique<std::thread>([this, i] { latch_.count_down();this->Run(i); });
    }
  }


 private:
  void Run(int id) {
    auto check = [](int i, std::vector<int>& idxs) {
      for (int j : idxs) {
        if (i == j) {
          return true;
        }
      }
      return false;
    };
    std::fprintf(stdout, "%d: thread  start\n", id);
    latch_.wait();
    for (int k = 0; k < 10000; ++k) {
      auto [i, j] = GetRandom();
      int i2 = (i + 1) % N;
      int i3 = (i + 2) % N;
      std::vector<int> idxs = {i, i2, i3};
      if (check(j, idxs)) {
        continue;
      }
      auto txn = txn_mgr_->Begin();
      txn->SetState(Transaction::TransactionState::GROWING);
      int sum = 0; 
      std::sort(idxs.begin(), idxs.end());
      bool success = false;
      // 1. for each idx, we think it is a row of relation-table.
      // 2. We are not considering table-level locks for now.
      // 3. If we do not skip j in (idx, idx2, idx3), there is a high possibility of deadlock in this case
      if (Read(txn, idxs, &sum)) {
          success = Write(txn, j, sum);
      }
      if (success) {
        txn_mgr_->Commit(txn);
      } else {
        txn_mgr_->Abort(txn);
      } 
    }
  }

  bool Read(Transaction* txn, std::vector<int> &idxs, int* sum) {
    for (int i : idxs) {
      int v = Read(txn, i);
      std::fprintf(stdout, "Read (%d, %d)\n", i, v);
      if (v == -1) {
        return false;
      }
      *sum += v;
    }
    return true;
  }

  int Read(Transaction* txn, int idx) {
    if (!lock_mgr_->LockRow(txn, 0, idx, LockManager::LockMode::SHARED)) {
      return -1;
    }
    txn->GetSLockSet()->insert(idx);
    return vec_[idx];
  }

  bool Write(Transaction* txn, int idx, int val) {
    std::fprintf(stdout,"Write (%d, %d)\n", idx, val);
    if (!lock_mgr_->LockRow(txn, 0, idx, LockManager::LockMode::EXCLUSIVE)) {
      return false;
    }
    txn->GetXLockSet()->insert(idx);
    vec_[idx] = val;
    return true;
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
  std::vector<int> vec_;
  int workers_;
  std::vector<std::unique_ptr<std::thread>> threads_;
  std::unique_ptr<LockManager> lock_mgr_;
  std::unique_ptr<TransactionManager> txn_mgr_;
  Random rand_;
  std::latch latch_;
};

int main() {
  Test test(3);
  test.Start();
}