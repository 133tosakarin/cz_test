
1. require c++20 

1. std::latch is used, so C++20 is required for compilation.

2. Two versions have been implemented: one using simple read-write locks, and the other considering a transactional approach, but neither has implemented deadlock detection yet.

### How to build

`git clone https://github.com/133tosakarin/cz_test`

`cd cz_tet`


`mkdir build && cd build`


`make`

For simple_test:
  Use a shared_lock for all elements in the array so that the entire array does not need to be locked.

For with_txn_test:
  Treat the array as a relational table, where each element in the array is considered a row in the table. Each loop within the work is a transaction, and a LockManager is used to handle the locks for the transactions.

Summary:
  The first approach is simple, but deadlock detection is difficult. 
  
  The second approach requires a LockManager, txn, and txn_manager, which is more complex to implement but can be extended later to support deadlock detection.
