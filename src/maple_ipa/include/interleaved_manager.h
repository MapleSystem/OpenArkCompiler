/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MAPLE_IPA_INCLUDE_INTERLEAVED_MANAGER_H
#define MAPLE_IPA_INCLUDE_INTERLEAVED_MANAGER_H

#include "module_phase_manager.h"

namespace maple {
class InterleavedManager {
 public:
  InterleavedManager(MemPool *memPool, MIRModule *module, const std::string &input, bool timer)
      : allocator(memPool),
        mirModule(*module),
        phaseManagers(allocator.Adapter()),
        supportPhaseManagers(allocator.Adapter()),
        meInput(input),
        timePasses(timer) {}

  InterleavedManager(MemPool *memPool, MIRModule *module)
      : allocator(memPool),
        mirModule(*module),
        phaseManagers(allocator.Adapter()),
        supportPhaseManagers(allocator.Adapter()) {}

  ~InterleavedManager() {
    if (timePasses) {
      DumpTimers();
    }
  }

  void DumpTimers();

  const MapleAllocator *GetMemAllocator() const {
    return &allocator;
  }

  MemPool *GetMemPool() {
    return allocator.GetMemPool();
  }

  void AddPhases(const std::vector<std::string> &phases, bool isModulePhase, bool timePhases = false,
                 bool genMpl = false);
  void Run();
  const PhaseManager *GetSupportPhaseManager(const std::string &phase);

 private:
  void InitSupportPhaseManagers();
  void OptimizeFuncs(MeFuncPhaseManager &fpm, MapleVector<MIRFunction*> &compList);

  MapleAllocator allocator;
  MIRModule &mirModule;
  MapleVector<PhaseManager*> phaseManagers;
  MapleVector<PhaseManager*> supportPhaseManagers;  // Used to check whether a phase is supported and by which manager
  std::string meInput;
  bool timePasses = false;
};
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_INTERLEAVED_MANAGER_H
