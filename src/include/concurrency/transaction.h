
#pragma once
#include <unordered_set>
using txn_t = int;
#define INVALID_TXN_ID -1

class Transaction {
 public:
  enum class TransactionState { GROWING, SHRINK, COMMITED, ABORT };

  explicit Transaction(txn_t txn_id, int table_oid) : txn_id_(txn_id), table_oid_(table_oid) {}

  std::unordered_set<int>* GetSLockSet() { return &s_lock_set_; }
  std::unordered_set<int>* GetXLockSet() { return &x_lock_set_; }

  bool IsSharedLocked(int idx) {
    return s_lock_set_.find(idx) != s_lock_set_.end();
  }
  bool IsExcludecLocked(int idx) {
    return x_lock_set_.find(idx) != x_lock_set_.end();
  }

  TransactionState GetState() const {
    return state_;
  }

  txn_t GetTxnId() const {
    return txn_id_;
  }

  void SetState(TransactionState state) {
    state_ = state;
  }

  void SetTableOid(int table_id) {
    table_oid_ = table_id;
  }

  int GetTableOid() const {
    return table_oid_;
  }
 private:

  txn_t txn_id_;   
  TransactionState state_;
  int table_oid_;
  std::unordered_set<int> s_lock_set_;
  std::unordered_set<int> x_lock_set_;
};