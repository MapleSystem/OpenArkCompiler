/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <unordered_set>
#include <stack>
#include <map>
#include <string>
#include <mutex>
#include "mir_config.h"
#include "mpl_logging.h"
#include "thread_env.h"

// Local debug control
#ifdef MP_DEBUG
#include <iostream>
#endif  // MP_DEBUG

namespace maple {
#ifdef _WIN32
#define FALSE 0
#define TRUE 1
#endif

#define BITS_ALIGN(size) (((size) + 7) & (0xFFFFFFF8))

constexpr size_t kMemBlockSizeMin = 2 * 1024;

struct MemBlock {
  explicit MemBlock(size_t size) : memSize(size) {
    startPtr = reinterpret_cast<uint8_t *>(malloc(size));
#ifdef MP_DEBUG
    LogInfo::MapleLogger() << "New MemBlock: " << this << ", startPtr: " << (void *)startPtr << ", memSize: " << memSize
                           << std::endl;
#endif
  }

  ~MemBlock() {
    if (startPtr != nullptr) {
      free(startPtr);
    }
  }

  uint8_t *EndPtr() const {
    return startPtr + memSize;
  }

  uint8_t *startPtr = nullptr;
  size_t memSize = 0;
  MemBlock *nextMemBlock = nullptr;
};

// Class declaration
class MemPool;
class StackMemPool;
class MemPoolCtrler;
extern MemPoolCtrler memPoolCtrler;

static inline bool IsGlobalCtrler(const MemPoolCtrler &mpCtrler) {
  return &mpCtrler == &maple::memPoolCtrler;
}

// Memory Pool controller class
class MemPoolCtrler {
  friend MemPool;

 public:
  static bool freeMemInTime;
  MemPoolCtrler() = default;

  ~MemPoolCtrler();

  MemPool *NewMemPool(const std::string&, bool isLocalPool);
  void DeleteMemPool(MemPool *memPool);
  bool HaveRace() const {
    return ThreadEnv::IsMeParallel() && IsGlobalCtrler(*this);
  }

  MemBlock *AllocMemBlock(const MemPool &pool, size_t size);
  MemBlock *AllocFixMemBlock(const MemPool &pool);
  MemBlock *AllocBigMemBlock(const MemPool &pool, size_t size);

 private:
  struct MemBlockCmp {
    bool operator()(const MemBlock *l, const MemBlock *r) const {
      return l->memSize > r->memSize;
    }
  };

  void FreeMem();
  void FreeMemBlocks(const MemPool &pool, MemBlock *fixedMemHead, MemBlock *bigMemHead);

  std::mutex ctrlerMutex;                  // this mutex is used to protect memPools
  std::unordered_set<MemPool *> memPools;  // set of mempools managed by it
  MemBlock *fixedFreeMemBlocks = nullptr;
  std::multiset<MemBlock *, MemBlockCmp> bigFreeMemBlocks;
#ifdef MP_DEBUG
  size_t totalFreeMem = 0;
  size_t maxFreeMen = 0;
  size_t allocFailed = 0;
  size_t allocFailedSize = 0;
  size_t allocTotal = 0;
  size_t allocTotalSize = 0;
#endif
};

class MemPool {
  friend MemPoolCtrler;

 public:
  MemPool(MemPoolCtrler &ctl, const std::string &name) : ctrler(ctl) {
#ifdef MP_DEBUG
    LogInfo::MapleLogger() << "MEMPOOL: New " << name << '\n';
    this->name = name;
#endif
    (void)(name);
  }

  virtual ~MemPool();

  virtual void *Malloc(size_t size);
  void *Calloc(size_t size);
  void *Realloc(const void *ptr, size_t oldSize, size_t newSize);
  virtual void ReleaseContainingMem();

  const MemPoolCtrler &GetCtrler() const {
    return ctrler;
  }

  template <class T>
  T *Clone(const T &t) {
    void *p = Malloc(sizeof(T));
    ASSERT(p != nullptr, "ERROR: New error");
    p = new (p) T(t);
    return static_cast<T *>(p);
  }

  template <class T, typename... Arguments>
  T *New(Arguments &&... args) {
    void *p = Malloc(sizeof(T));
    ASSERT(p != nullptr, "ERROR: New error");
    p = new (p) T(std::forward<Arguments>(args)...);
    return static_cast<T *>(p);
  }

  template <class T>
  T *NewArray(size_t num) {
    void *p = Malloc(sizeof(T) * num);
    ASSERT(p != nullptr, "ERROR: NewArray error");
    p = new (p) T[num];
    return static_cast<T *>(p);
  }

 protected:
  MemPoolCtrler &ctrler;  // Hookup controller object
  uint8_t *endPtr = nullptr;
  uint8_t *curPtr = nullptr;
  MemBlock *fixedMemHead = nullptr;
  MemBlock *bigMemHead = nullptr;

#ifdef MP_DEBUG
  std::string name;
  size_t alloc = 0;
  size_t used = 0;
  static size_t sumAlloc;
  static size_t sumUsed;
#endif
  uint8_t *AllocNewMemBlock(size_t bytes);
};

using ThreadLocalMemPool = MemPool;

class ThreadShareMemPool : public MemPool {
 public:
  using MemPool::MemPool;
  void *Malloc(size_t size) override {
    ParallelGuard guard(poolMutex, ctrler.HaveRace());
    return MemPool::Malloc(size);
  }
  void ReleaseContainingMem() override {
    ParallelGuard guard(poolMutex, ctrler.HaveRace());
    MemPool::ReleaseContainingMem();
  }

 private:
  std::mutex poolMutex;  // this mutex is used to protect memPools
};

class LocalMapleAllocator;
class StackMemPool : public MemPool {
 public:
  using MemPool::MemPool;
  friend LocalMapleAllocator;

 private:
  // all malloc requested from LocalMapleAllocator
  void *Malloc(size_t size);
  uint8_t *AllocTailMemBlock(size_t size);

  // these methods should be called from LocalMapleAllocator
  template <class T>
  T *Clone(const T &t) = delete;

  template <class T, typename... Arguments>
  T *New(Arguments &&... args) = delete;

  template <class T>
  T *NewArray(size_t num) = delete;

#ifdef DEBUG
  void PushAllocator(const LocalMapleAllocator *alloc) {
    allocators.push(alloc);
  }
  const LocalMapleAllocator *TopAllocator() {
    return allocators.top();
  }
  void PopAllocator() {
    allocators.pop();
  }
  std::stack<const LocalMapleAllocator *> allocators;
#endif

  // reuse mempool fixedMemHead, bigMemHead, (curPtr, endPtr for fixed memory)
  MemBlock *fixedMemStackTop = nullptr;
  MemBlock *bigMemStackTop = nullptr;
  uint8_t *bigCurPtr = nullptr;
  uint8_t *bigEndPtr = nullptr;
  MemBlock *AllocMemBlockBySize(size_t size);
  void ResetStackTop(const LocalMapleAllocator *alloc, uint8_t *fixedCurPtrMark, MemBlock *fixedStackTopMark,
                     uint8_t *bigCurPtrMark, MemBlock *bigStackTopMark) noexcept;
};
}  // namespace maple
#endif  // MEMPOOL_INCLUDE_MEMPOOL_H
