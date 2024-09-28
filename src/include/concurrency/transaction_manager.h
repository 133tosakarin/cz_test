
#pragma once
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "transaction.h"
#include "lock_manager.h"
class TransactionManager {
 public:
  TransactionManager(LockManager* lock_mgr) : next_txn_id_(0), lock_mgr_(lock_mgr) {}
  ~TransactionManager() = default;

  Transaction* Begin(Transaction* txn = nullptr) {
    int txn_id;
    std::shared_ptr<Transaction> txn_ptr;
    if (txn == nullptr) {
      txn_id = next_txn_id_++;
      txn_ptr = std::make_shared<Transaction>(txn_id, -1);
      txn = txn_ptr.get();
      std::unique_lock latch(transactions_latch_);
      transactions_[txn->GetTxnId()] = txn_ptr;
    }
    return txn;
  }

  Transaction* GetTransaction(txn_t txn_id) {
    return transactions_[txn_id].get();
  }

  void Commit(Transaction* txn);

  void Abort(Transaction* txn);

  void ReleaseLocks(Transaction* txn);
 private:
  std::atomic<txn_t> next_txn_id_;
  LockManager* lock_mgr_;
  std::unordered_map<txn_t, std::shared_ptr<Transaction>> transactions_;
  std::mutex transactions_latch_;
};