
#include <memory>
#include <mutex>
#include <print>
#include "concurrency/lock_manager.h"
#include "concurrency/transaction.h"

// bool LockManager::LockTable(Transaction* txn, int table_id, LockMode mode) {
//   // 1. table_id is not in lock_manager
//   std::shared_ptr<LockRequestQueue> lock_queue;
//   std::print("{} lock_table\n", txn->GetTxnId());
//   {
//     std::unique_lock lock(table_lock_map_latch_);
//     if (!table_lock_map_.contains(table_id)) {
//       auto lock_queue = std::make_shared<LockRequestQueue>();
//       lock_queue->requests_queue_.emplace_back(
//           std::make_shared<LockRequest>(txn->GetTxnId(), table_id, mode));
//       lock_queue->requests_queue_.front()->granted_ = true;
//       txn->SetTableOid(table_id);
//       table_lock_map_[table_id] = lock_queue;
//       return true;
//     }
//     lock_queue = table_lock_map_[table_id];
//   }
// 
//   // help function
//   auto check_compatible = [&](const auto& req) {
//     if (txn->GetState() == Transaction::TransactionState::ABORT) {
//       return true;
//     }
//     for (const auto& request : lock_queue->requests_queue_) {
//       if (req == request) {
//         break;
//       }
//       if (!AreCompatible(request->lock_type_, req->lock_type_)) {
//         return false;
//       }
//     }
//     return true;
//   };
//   bool same_lock;
//   std::unique_lock queue_latch(lock_queue->latch_);
//   // upgrade lock
//   if (!UpgradableLockTable(lock_queue, txn, mode, 0, same_lock)) {
//     auto request =
//         std::make_shared<LockRequest>(txn->GetTxnId(), table_id, mode);
//     lock_queue->requests_queue_.emplace_back(request);
//     lock_queue->cv_.wait(queue_latch,
//                          [&] { return check_compatible(request); });
//     if (txn->GetState() == Transaction::TransactionState::ABORT) {
//       std::erase_if(lock_queue->requests_queue_,
//                     [&](const auto& req) { return req == request; });
//       return false;
//     }
//     request->lock_type_ = mode;
//     request->granted_ = true;
//     lock_queue->cv_.notify_all();
//     txn->SetTableOid(table_id);
//     std::print("success Lock Table\n");
//     return true;
//   }
//   // 2. check lock compatibility
//   auto request = std::make_shared<LockRequest>(txn->GetTxnId(), table_id, mode);
//   lock_queue->requests_queue_.emplace_back(request);
//   lock_queue->cv_.wait(queue_latch, [&] { return check_compatible(request); });
//   if (txn->GetState() == Transaction::TransactionState::ABORT) {
//     std::erase_if(lock_queue->requests_queue_,
//                   [&](const auto& req) { return req == request; });
//     return false;
//   }
//   request->lock_type_ = mode;
//   request->granted_ = true;
//   lock_queue->cv_.notify_all();
//   txn->SetTableOid(table_id);
//   return true;
// }

bool LockManager::AreCompatible(LockMode lock_mode1, LockMode lock_mode2) {
  if (lock_mode1 == lock_mode2) {
    return lock_mode1 == LockMode::SHARED;
  }
  return false;
}

bool LockManager::UpgradableLockTable(std::shared_ptr<LockRequestQueue> q,
                                      Transaction* txn, LockMode lock_mode,
                                      int table_id, bool& same_lock) {
  std::unique_lock latch(q->latch_);
  auto it = std::find_if(
      q->requests_queue_.begin(), q->requests_queue_.end(),
      [&](const auto& req) { return req->txn_id_ == txn->GetTxnId(); });
  if (it == q->requests_queue_.end()) {
    return false;
  }
  if (q->upgrading_txn_ != INVALID_TXN_ID) {
    return false;
  }
  if (CanUpgrade((*it)->lock_type_, lock_mode)) {
    q->upgrading_txn_ = txn->GetTxnId();
    q->requests_queue_.erase(it);
    return true;
  }
  return false;
}

bool LockManager::UpgradableLockRow(std::shared_ptr<LockRequestQueue> q,
                                    Transaction* txn, LockMode lock_mode,
                                    int table_id, int row_id, bool& same_lock) {
  std::unique_lock lock(q->latch_);
  auto it = std::find_if(q->requests_queue_.begin(), q->requests_queue_.end(),
                         [&](const auto& req) {
                           return txn->GetTxnId() == req->txn_id_ &&
                                  row_id == req->tuple_id_;
                         });
  if (it == q->requests_queue_.end()) {
    return false;
  }
  if (q->upgrading_txn_ != INVALID_TXN_ID) {
    return false;
  }
  if (CanUpgrade((*it)->lock_type_, lock_mode)) {
    q->upgrading_txn_ = txn->GetTxnId();
    q->requests_queue_.erase(it);
    return true;
  }
  return false;
}

bool LockManager::LockRow(Transaction* txn, int table_id, int row,
                          LockMode mode) {
  std::shared_ptr<LockRequestQueue> lock_queue;
  {
    std::unique_lock lock(tuple_lock_map_latch_);
    if (!tuple_lock_map_.contains(row)) {
      auto lock_queue = std::make_shared<LockRequestQueue>();
      lock_queue->requests_queue_.emplace_back(
          std::make_shared<LockRequest>(txn->GetTxnId(), table_id, row, mode));
      lock_queue->requests_queue_.front()->granted_ = true;
      tuple_lock_map_[row] = lock_queue;
      return true;
    }
    lock_queue = tuple_lock_map_[row];
  }
  auto check_compatible = [&](const auto& req) {
    if (txn->GetState() == Transaction::TransactionState::ABORT) {
      return true;
    }
    for (const auto& request : lock_queue->requests_queue_) {
      if (req == request) {
        break;
      }
      if (!AreCompatible(request->lock_type_, req->lock_type_)) {
        return false;
      }
    }
    return true;
  };
  bool same_lock;
  std::unique_lock queue_latch(lock_queue->latch_);
  // upgrade lock
  auto request =
      std::make_shared<LockRequest>(txn->GetTxnId(), table_id, row, mode);
  if (!UpgradableLockRow(lock_queue, txn, mode, 0, row, same_lock)) {
    lock_queue->requests_queue_.emplace_back(request);
    lock_queue->cv_.wait(queue_latch,
                         [&] { return check_compatible(request); });
    if (txn->GetState() == Transaction::TransactionState::ABORT) {
      lock_queue->requests_queue_.erase(
          std::find_if(lock_queue->requests_queue_.begin(),
                       lock_queue->requests_queue_.end(),
                       [&](const auto& req) { return req == request; }));
      return false;
    }
    request->lock_type_ = mode;
    request->granted_ = true;
    lock_queue->cv_.notify_all();
    return true;
  }
  // 2. check lock compatibility
  lock_queue->requests_queue_.emplace_back(request);
  lock_queue->cv_.wait(queue_latch, [&] { return check_compatible(request); });
  if (txn->GetState() == Transaction::TransactionState::ABORT) {
    lock_queue->requests_queue_.erase(
        std::find_if(lock_queue->requests_queue_.begin(),
                     lock_queue->requests_queue_.end(),
                     [&](const auto& req) { return req == request; }));
    return false;
  }
  request->lock_type_ = mode;
  request->granted_ = true;
  lock_queue->cv_.notify_all();
  return true;
}

bool LockManager::CanUpgrade(LockMode from, LockMode to) {
  return from == LockMode::SHARED && to == LockMode::EXCLUSIVE;
}

bool LockManager::UnlockRow(Transaction* txn, int table_id, int row) {
  txn->SetState(Transaction::TransactionState::SHRINK);
  txn_t txn_id = txn->GetTxnId();
  std::shared_ptr<LockRequestQueue> lock_queue;
  {
    std::unique_lock lock(tuple_lock_map_latch_);
    if (!tuple_lock_map_.contains(row)) {
      return false;
    }
    lock_queue = tuple_lock_map_[row];
  }
  auto it = std::find_if(
      lock_queue->requests_queue_.begin(), lock_queue->requests_queue_.end(),
      [&](const auto& req) { return req->txn_id_ == txn->GetTxnId(); });
  lock_queue->requests_queue_.erase(it);
  lock_queue->cv_.notify_all();
  return true;
}
// bool LockManager::UnlockTable(Transaction* txn, int table_id) {
//   txn->SetState(Transaction::TransactionState::SHRINK);
//   txn_t txn_id = txn->GetTxnId();
//   std::shared_ptr<LockRequestQueue> lock_queue;
//   {
//     std::unique_lock lock(table_lock_map_latch_);
//     if (!table_lock_map_.contains(table_id)) {
//       return false;
//     }
//     lock_queue = table_lock_map_[table_id];
//   }
// 
//   auto it = std::find_if(
//       lock_queue->requests_queue_.begin(), lock_queue->requests_queue_.end(),
//       [&](const auto& req) { return req->txn_id_ == txn_id; });
//   lock_queue->requests_queue_.erase(it);
//   lock_queue->cv_.notify_all();
//   return true;
// }
