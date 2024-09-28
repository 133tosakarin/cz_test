
#include <atomic>
#include <condition_variable>
#include <list>
#include <thread>
#include <unordered_map>

#include "transaction.h"
#pragma once
class LockManager {
 public:
  enum class LockMode { SHARED, EXCLUSIVE };
  class LockRequest {
   public:
    LockRequest(int id, int table_id, LockMode lock_type)
        : txn_id_(id), table_id_(table_id), lock_type_(lock_type) {}
    LockRequest(int id, int table_id, int tuple_id, LockMode lock_type)
        : txn_id_(id),
          table_id_(table_id),
          tuple_id_(tuple_id),
          lock_type_(lock_type) {}
    txn_t txn_id_;
    int table_id_{-1};
    int tuple_id_;
    LockMode lock_type_;
    bool granted_{false};
  };

  class LockRequestQueue {
   public:
    std::list<std::shared_ptr<LockRequest>> requests_queue_;
    std::condition_variable cv_;
    std::mutex latch_;
    txn_t upgrading_txn_{INVALID_TXN_ID};
  };

  LockManager() = default;

  bool LockTable(Transaction* txn, int table_id, LockMode mode);
  bool UnlockTable(Transaction* txn, int table_id);
  bool LockRow(Transaction* txn, int table_id, int row, LockMode mode);
  bool UnlockRow(Transaction* txn, int table_id, int row);
  bool AreCompatible(LockMode, LockMode);
  bool UpgradableLockTable(std::shared_ptr<LockRequestQueue> q,
                           Transaction* txn, LockMode, int, bool&);
  bool UpgradableLockRow(std::shared_ptr<LockRequestQueue> q, Transaction* txn,
                         LockMode lock_mode, int table_id, int row_id,
                         bool& same_lock);
  bool CanUpgrade(LockMode from, LockMode to);
  bool GetLockTableSet(Transaction* txn, LockMode lock_mode);

 private:
  std::unordered_map<txn_t, std::shared_ptr<LockRequestQueue>> table_lock_map_;
  std::unordered_map<txn_t, std::shared_ptr<LockRequestQueue>> tuple_lock_map_;
  std::mutex table_lock_map_latch_;
  std::mutex tuple_lock_map_latch_;
  std::atomic_bool enable_cycle_detection_{false};
  std::unique_ptr<std::thread> cycle_detection_thread_;

  std::unordered_map<txn_t, std::vector<txn_t>> wait_for_graph_;
  std::mutex wait_for_graph_latch_;
};