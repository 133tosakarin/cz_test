
#include "concurrency/transaction_manager.h"
#include "concurrency/transaction.h"

void TransactionManager::Commit(Transaction* txn) {
  ReleaseLocks(txn);
  txn->SetState(Transaction::TransactionState::COMMITED);
}

void TransactionManager::Abort(Transaction* txn) {
  // TODO: undo operation
  // current just release all lock
  ReleaseLocks(txn);
  txn->SetState(Transaction::TransactionState::ABORT);
}

void TransactionManager::ReleaseLocks(Transaction* txn) {
  for (auto row_id : *txn->GetSLockSet()) {
    lock_mgr_->UnlockRow(txn, txn->GetTableOid(), row_id);
  }

  for (auto row_id : *txn->GetXLockSet()) {
    lock_mgr_->UnlockRow(txn, txn->GetTableOid(), row_id);
  }

  // lock_mgr_->UnlockTable(txn, txn->GetTableOid());
}