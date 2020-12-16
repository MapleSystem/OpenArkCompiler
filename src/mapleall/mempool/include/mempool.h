/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef MEMPOOL_INCLUDE_MEMPOOL_H
#define MEMPOOL_INCLUDE_MEMPOOL_H
#include <list>
#include <set>
#include <stack>
#include <map>
#include <string>
#include "mir_config.h"
#include "mpl_logging.h"


namespace maple {
#ifdef _WIN32
#define FALSE 0
#define TRUE 1
#endif

// Class declaration
class MemPool;  // circular dependency exists, no other choice
// Memory Pool controller class
class MemPoolCtrler {
  friend MemPool;

 public:  // Methods
  static bool freeMemInTime;
  MemPoolCtrler() = default;

  ~MemPoolCtrler();

  MemPool *NewMemPool(const std::string&);
  void DeleteMemPool(MemPool *memPool);
  void FreeMem();
  bool IsEmpty() const {
    return memPools.empty();
  }

 private:  // Methods
  struct MemBlock {
    size_t available;  // Available memory size
    size_t origSize;   // original size
    void *ptr;               // Current pointer to the first available position
  };

  class MemBlockCmp {
   public:
    bool operator()(const MemBlock *l, const MemBlock *r) const {
      if (l->available != r->available) {
        return l->available > r->available;
      }
      return (std::uintptr_t)(l->ptr) > (std::uintptr_t)(r->ptr);
    }
  };

  // Free small/large size memory block list
  std::list<MemBlock*> freeMemBlocks;
  std::map<size_t, std::set<MemBlock*, MemBlockCmp>> largeFreeMemBlocks;
  std::set<MemPool*> memPools;  // set of mempools managed by it
};

class MemPool {
  friend MemPoolCtrler;

 public:  // Methods
  MemPool(MemPoolCtrler &ctl, const std::string &name) : ctrler(ctl), name(name) {
  }

  ~MemPool();
  void *Malloc(size_t size);
  void *Calloc(size_t size);
  void *Realloc(const void *ptr, size_t oldSize, size_t newSize);
  void ReleaseContainingMem();

  void Release() {
    ctrler.DeleteMemPool(this);
  }

  const std::string &GetName() const {
    return name;
  }

  const MemPoolCtrler &GetCtrler() const {
    return ctrler;
  }

  template <class T>
  T *Clone(const T &t) {
    void *p = Malloc(sizeof(T));
    CHECK_FATAL(p != nullptr, "ERROR: New error");
    p = new (p) T(t);  // Call clone constructor
    return static_cast<T*>(p);
  }

  // New templates
  template <class T, typename... Arguments>
  T *New(Arguments &&... args) {
    void *p = Malloc(sizeof(T));
    CHECK_FATAL(p != nullptr, "ERROR: New error");
    p = new (p) T(std::forward<Arguments>(args)...);  // Call constructor
    return static_cast<T*>(p);
  }

  // New Array template
  template <class T>
  T *NewArray(size_t num) {
    void *p = Malloc(sizeof(T) * num);
    CHECK_FATAL(p != nullptr, "ERROR: NewArray error");
    p = new (p) T[num];
    return static_cast<T*>(p);
  }

#define BITS_ALIGN(size) (((size) + 7) & (0xFFFFFFF8))
#define MEM_BLOCK_FIRST_PTR(x) \
  static_cast<void*>((reinterpret_cast<char*>(x)) + BITS_ALIGN(sizeof(MemPoolCtrler::MemBlock)))

 private:                                         // constants
  static constexpr size_t kMinBlockSize = 0x800;  // Minimum BlockSize is 2K
  static constexpr size_t kMemBlockOverhead = (BITS_ALIGN(sizeof(MemPoolCtrler::MemBlock)));
  MemPoolCtrler::MemBlock *GetLargeMemBlock(size_t size);  // Raw allocate large memory block
  MemPoolCtrler::MemBlock *GetMemBlock();
  MemPoolCtrler &ctrler;  // Hookup controller object
  std::string name;       // Name of the memory pool
  // Save the memory block stack
  std::stack<MemPoolCtrler::MemBlock*> memBlockStack;
  std::stack<MemPoolCtrler::MemBlock*> largeMemBlockStack;
};

extern MemPoolCtrler memPoolCtrler;
}  // namespace maple
#endif  // MEMPOOL_INCLUDE_MEMPOOL_H
