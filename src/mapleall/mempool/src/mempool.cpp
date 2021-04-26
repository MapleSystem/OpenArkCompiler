/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mempool.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include "thread_env.h"
#include "securec.h"
#include "mpl_logging.h"

namespace maple {
MemPoolCtrler memPoolCtrler;
bool MemPoolCtrler::freeMemInTime = false;

#ifdef MP_DEBUG
size_t MemPool::sumAlloc = 0;
size_t MemPool::sumUsed = 0;
#endif

void MemPoolCtrler::FreeMemBlocks(const MemPool &pool, MemBlock *fixedMemHead, MemBlock *bigMemHead) {
  (void)(pool);

  MemBlock *fixedTail = nullptr;

  if (fixedMemHead != nullptr) {
#ifdef MP_DEBUG
    MemBlock *cur = fixedMemHead;
    while (cur != nullptr) {
      totalFreeMem += cur->memSize;
      maxFreeMen = std::max(cur->memSize, maxFreeMen);
      LogInfo::MapleLogger() << "Giveback MemBlock: " << cur << ", startPtr: " << (void *)cur->startPtr
                             << ", memSize: " << cur->memSize << std::endl;
      cur = cur->nextMemBlock;
    }
#endif
    fixedTail = fixedMemHead;
    while (fixedTail->nextMemBlock != nullptr) {
      fixedTail = fixedTail->nextMemBlock;
    }
  }

  ParallelGuard guard(ctrlerMutex, HaveRace());
  if (fixedTail != nullptr) {
    fixedTail->nextMemBlock = fixedFreeMemBlocks;
    ASSERT(fixedTail->nextMemBlock != fixedTail, "error");
    fixedFreeMemBlocks = fixedMemHead;
  }

  if (bigMemHead != nullptr) {
    auto *cur = bigMemHead;
    while (cur != nullptr) {
#ifdef MP_DEBUG
      totalFreeMem += cur->memSize;
      maxFreeMen = std::max(cur->memSize, maxFreeMen);
      LogInfo::MapleLogger() << "Giveback MemBlock: " << cur << ", startPtr: " << (void *)cur->startPtr
                             << ", memSize: " << cur->memSize << std::endl;
#endif
      bigFreeMemBlocks.insert(cur);
      cur = cur->nextMemBlock;
    }
  }

#ifdef MP_DEBUG
  LogInfo::MapleLogger() << "MEMPOOL: Free Success, " << pool.name << " Total Free Mem: " << totalFreeMem
                         << " Max Free Mem: " << maxFreeMen << '\n';
#endif
}

// Destructor, free all allocated memories
MemPoolCtrler::~MemPoolCtrler() {
  // Delete Memory Pool
  for (MemPool *memPool : memPools) {
#ifdef MP_DEBUG
    if (memPool != nullptr) {
      LogInfo::MapleLogger() << "MEMPOOL: Left mempool " << memPool->name << "\n";
    }
#endif
    delete memPool;
  }

#ifdef MP_DEBUG
  LogInfo::MapleLogger() << "MEMPOOL: Alloc Failed Stat: Alloc Failed: " << allocFailed
                         << " Alloc Total: " << allocTotal
                         << " Alloc Fail Rate: " << (1.0 * allocFailed / std::max(static_cast<size_t>(1), allocTotal))
                         << " Alloc Failed Size: " << allocFailedSize << " Alloc Total Size: " << allocTotalSize
                         << " Alloc Failed Size Rate "
                         << (1.0 * allocFailedSize / std::max(static_cast<size_t>(1), allocTotalSize)) << '\n';
#endif
  FreeMem();
}

// Allocate a new memory pool and register it in controller
MemPool *MemPoolCtrler::NewMemPool(const std::string &name, bool isLocalPool) {
  MemPool *memPool = nullptr;

  if (isLocalPool) {
    memPool = new ThreadLocalMemPool(*this, name);
  } else {
    memPool = new ThreadShareMemPool(*this, name);
  }

  ParallelGuard guard(ctrlerMutex, HaveRace());
  memPools.insert(memPool);
  return memPool;
}

// Re-cycle all memories allocated on a memory pool to free list
void MemPoolCtrler::DeleteMemPool(MemPool *memPool) {
  ASSERT_NOT_NULL(memPool);

  {
    ParallelGuard guard(ctrlerMutex, HaveRace());
    // Delete entry of this mempool in memPools and delete the mempool node
    memPools.erase(memPool);
  }
  delete memPool;

  if (freeMemInTime) {
    FreeMem();
  }
}

void MemPoolCtrler::FreeMem() {
  ParallelGuard guard(ctrlerMutex, HaveRace());

  while (fixedFreeMemBlocks != nullptr) {
    MemBlock *arena = fixedFreeMemBlocks;
    fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
    delete arena;
  }

  for (MemBlock *block : bigFreeMemBlocks) {
    delete block;
  }
  bigFreeMemBlocks.clear();
}

MemBlock *MemPoolCtrler::AllocMemBlock(const MemPool &pool, size_t size) {
  if (size <= kMemBlockSizeMin) {
    return AllocFixMemBlock(pool);
  } else {
    return AllocBigMemBlock(pool, size);
  }
}

MemBlock *MemPoolCtrler::AllocFixMemBlock(const MemPool &pool) {
  (void)(pool);
  MemBlock *ret = nullptr;

  {
    ParallelGuard guard(ctrlerMutex, HaveRace());
    if (fixedFreeMemBlocks != nullptr) {
      ret = fixedFreeMemBlocks;
      fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
#ifdef MP_DEBUG
      totalFreeMem -= ret->memSize;
      if (ret->memSize == maxFreeMen) {
        if (fixedFreeMemBlocks == nullptr) {
          maxFreeMen = 0;
        }
      }
      LogInfo::MapleLogger() << "MEMPOOL: Alloc Success, " << pool.name << " Alloc size: " << kMemBlockSizeMin
                             << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
      LogInfo::MapleLogger() << "Reuse MemBlock: " << ret << ", startPtr: " << (void *)ret->startPtr
                             << ", memSize: " << ret->memSize << std::endl;
      allocTotal += 1;
#endif
      return ret;
    }
  }

#ifdef MP_DEBUG
  if (maxFreeMen > kMemBlockSizeMin) {
    allocFailed += 1;
    allocFailedSize += kMemBlockSizeMin;
    LogInfo::MapleLogger() << "MEMPOOL: Alloc Failed, " << pool.name << " Alloc size: " << kMemBlockSizeMin
                           << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
  } else {
    LogInfo::MapleLogger() << "MEMPOOL: Alloc Success, " << pool.name << " Alloc size: " << kMemBlockSizeMin
                           << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
  }
  allocTotal += 1;
  allocTotalSize += kMemBlockSizeMin;
#endif

  return new MemBlock(kMemBlockSizeMin);
}

MemBlock *MemPoolCtrler::AllocBigMemBlock(const MemPool &pool, size_t size) {
  ASSERT(size > kMemBlockSizeMin, "Big memory block must be bigger than fixed memory block");
  (void)(pool);
  MemBlock *ret = nullptr;

  {
    ParallelGuard guard(ctrlerMutex, HaveRace());
    if (!bigFreeMemBlocks.empty() && (*bigFreeMemBlocks.begin())->memSize >= size) {
      ret = *bigFreeMemBlocks.begin();
      bigFreeMemBlocks.erase(bigFreeMemBlocks.begin());
#ifdef MP_DEBUG
      totalFreeMem -= ret->memSize;
      if (!bigFreeMemBlocks.empty()) {
        maxFreeMen = (*bigFreeMemBlocks.begin())->memSize;
      } else if (fixedFreeMemBlocks != nullptr) {
        maxFreeMen = kMemBlockSizeMin;
      } else {
        maxFreeMen = 0;
      }
      LogInfo::MapleLogger() << "MEMPOOL: Alloc Success, " << pool.name << " Alloc size: " << kMemBlockSizeMin
                             << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
      LogInfo::MapleLogger() << "Reuse MemBlock: " << ret << ", startPtr: " << (void *)ret->startPtr
                             << ", memSize: " << ret->memSize << std::endl;
      allocTotal += 1;
#endif
      return ret;
    }
  }

#ifdef MP_DEBUG
  if (maxFreeMen > size) {
    allocFailed += 1;
    allocFailedSize += size;
    LogInfo::MapleLogger() << "MEMPOOL: Alloc Failed, " << pool.name << " Alloc size: " << size
                           << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
  } else {
    LogInfo::MapleLogger() << "MEMPOOL: Alloc Success, " << pool.name << " Alloc size: " << size
                           << " Total Free Mem: " << totalFreeMem << " Max Free Mem: " << maxFreeMen << '\n';
  }
  allocTotal += 1;
  allocTotalSize += size;
#endif

  return new MemBlock(size);
}

MemPool::~MemPool() {
#ifdef MP_DEBUG
  MemPool::sumAlloc += alloc;
  MemPool::sumUsed += used;
  LogInfo::MapleLogger() << "MEMPOOL: Deleted " << name << " alloc: " << alloc << " used: " << used
                         << " Use Rate: " << (1.0 * used / std::max(static_cast<size_t>(1), alloc))
                         << " sum alloc: " << MemPool::sumAlloc << " sum used: " << MemPool::sumUsed << " sum rate: "
                         << (1.0 * MemPool::sumUsed / std::max(static_cast<size_t>(1), MemPool::sumAlloc)) << "\n";
#endif
  ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);
}

void *MemPool::Malloc(size_t size) {
  size = BITS_ALIGN(size);
  ASSERT(endPtr >= curPtr, "endPtr should >= curPtr");
  if (size > static_cast<size_t>(endPtr - curPtr)) {
    return AllocNewMemBlock(size);
  }
  uint8_t *retPtr = curPtr;
  curPtr += size;
#ifdef MP_DEBUG
  used += size;
#endif
  return retPtr;
}

void MemPool::ReleaseContainingMem() {
  ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);

  fixedMemHead = nullptr;
  bigMemHead = nullptr;
  endPtr = nullptr;
  curPtr = nullptr;
}

// Malloc size of memory from memory pool, then set 0
void *MemPool::Calloc(size_t size) {
  void *p = Malloc(BITS_ALIGN(size));
  ASSERT(p != nullptr, "ERROR: Calloc error");
  errno_t eNum = memset_s(p, BITS_ALIGN(size), 0, BITS_ALIGN(size));
  CHECK_FATAL(eNum == EOK, "memset_s failed");
  return p;
}

// Realloc new size of memory
void *MemPool::Realloc(const void *ptr, size_t oldSize, size_t newSize) {
  void *result = Malloc(newSize);
  ASSERT(result != nullptr, "ERROR: Realloc error");
  size_t copySize = ((newSize > oldSize) ? oldSize : newSize);
  if (copySize != 0 && ptr != nullptr) {
    errno_t eNum = memcpy_s(result, copySize, ptr, copySize);
    CHECK_FATAL(eNum == EOK, "memcpy_s failed");
  }
  return result;
}

uint8_t *MemPool::AllocNewMemBlock(size_t size) {
  MemBlock **head = nullptr;
  MemBlock *newMemBlock = ctrler.AllocMemBlock(*this, size);
  if (newMemBlock->memSize <= kMemBlockSizeMin) {
    head = &fixedMemHead;
  } else {
    head = &bigMemHead;
  }
#ifdef MP_DEBUG
  alloc += newMemBlock->memSize;
#endif

  newMemBlock->nextMemBlock = *head;
  *head = newMemBlock;
  CHECK_FATAL(newMemBlock->nextMemBlock != newMemBlock, "error");

  curPtr = newMemBlock->startPtr + size;
  endPtr = newMemBlock->startPtr + newMemBlock->memSize;
  ASSERT(curPtr <= endPtr, "must be");

  return newMemBlock->startPtr;
}

void *StackMemPool::Malloc(size_t size) {
  size = BITS_ALIGN(size);
  uint8_t **curPtrPtr = nullptr;
  uint8_t *curEndPtr = nullptr;
  if (size <= kMemBlockSizeMin) {
    curPtrPtr = &curPtr;
    curEndPtr = endPtr;
  } else {
    curPtrPtr = &bigCurPtr;
    curEndPtr = bigEndPtr;
  }
  uint8_t *retPtr = *curPtrPtr;
  ASSERT(curEndPtr >= *curPtrPtr, "endPtr should >= curPtr");
  if (size > static_cast<size_t>(curEndPtr - *curPtrPtr)) {
    retPtr = AllocTailMemBlock(size);
  }
#ifdef MP_DEBUG
  LogInfo::MapleLogger() << "Malloc MemBlock: "
                         << " bigMemStackTop, " << (void *)bigMemStackTop << " retPtr, " << (void *)retPtr
                         << ", curPtr: " << (void *)(retPtr + size) << " bigCurPtr, " << (void *)bigCurPtr
                         << " bigEndPtr, " << (void *)bigEndPtr << std::endl;
#endif
  *curPtrPtr = retPtr + size;
  return retPtr;
}

// scoped mem pool don't use big mem block for small size, different with normal mempool
MemBlock *StackMemPool::AllocMemBlockBySize(size_t size) {
  if (size <= kMemBlockSizeMin) {
    return ctrler.AllocFixMemBlock(*this);
  } else {
    return ctrler.AllocBigMemBlock(*this, size);
  }
}

void StackMemPool::ResetStackTop(const LocalMapleAllocator *alloc, uint8_t *fixedCurPtrMark,
                                 MemBlock *fixedStackTopMark, uint8_t *bigCurPtrMark,
                                 MemBlock *bigStackTopMark) noexcept {
#ifdef DEBUG
  CHECK_FATAL(alloc == TopAllocator(), "ResetStackTop Eror, Only Top Allocator can ResetStackTop");
  PopAllocator();
#else
  (void)alloc;
#endif
  if (fixedStackTopMark != nullptr) {
    fixedMemStackTop = fixedStackTopMark;
    curPtr = fixedCurPtrMark;
    endPtr = fixedMemStackTop->EndPtr();
  } else if (fixedMemHead != nullptr) {
    fixedMemStackTop = fixedMemHead;
    curPtr = fixedMemStackTop->startPtr;
    endPtr = fixedMemStackTop->EndPtr();
  }

  if (bigStackTopMark != nullptr) {
    bigMemStackTop = bigStackTopMark;
    bigCurPtr = bigCurPtrMark;
    bigEndPtr = bigMemStackTop->EndPtr();
  } else if (bigMemHead != nullptr) {
    bigMemStackTop = bigMemHead;
    bigCurPtr = bigMemStackTop->startPtr;
    bigEndPtr = bigMemStackTop->EndPtr();
  }
}

uint8_t *StackMemPool::AllocTailMemBlock(size_t size) {
  MemBlock **head = nullptr;
  MemBlock **stackTop = nullptr;
  uint8_t **endPtrPtr = nullptr;

  if (size <= kMemBlockSizeMin) {
    head = &fixedMemHead;
    stackTop = &fixedMemStackTop;
    endPtrPtr = &endPtr;
  } else {
    head = &bigMemHead;
    stackTop = &bigMemStackTop;
    endPtrPtr = &bigEndPtr;
  }

  if (*stackTop == nullptr) {
    MemBlock *newMemBlock = AllocMemBlockBySize(size);
    *stackTop = newMemBlock;
    *head = newMemBlock;
    (*stackTop)->nextMemBlock = nullptr;
  } else {
    if ((*stackTop)->nextMemBlock != nullptr && (*stackTop)->nextMemBlock->memSize >= size) {
      *stackTop = (*stackTop)->nextMemBlock;
    } else {
      MemBlock *newMemBlock = AllocMemBlockBySize(size);
      auto *tmp = (*stackTop)->nextMemBlock;
      (*stackTop)->nextMemBlock = newMemBlock;
      *stackTop = newMemBlock;
      newMemBlock->nextMemBlock = tmp;
    }
  }
  *endPtrPtr = (*stackTop)->EndPtr();
  return (*stackTop)->startPtr;
}
}  // namespace maple
