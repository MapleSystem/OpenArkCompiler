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
  void RunMeOptimize(MeFuncPhaseManager &fpm);
  const PhaseManager *GetSupportPhaseManager(const std::string &phase);
  void AddIPAPhases(std::vector<std::string> &phases, bool timePhases = false, bool genMpl = false);
  void IPARun(MeFuncPhaseManager&);
  long GetEmitVtableImplTime() const {
    return emitVtableImplMplTime;
  }
  void SetEmitVtableImplTime(long time) {
    emitVtableImplMplTime = time;
  }
  static std::vector<std::pair<std::string, time_t>> interleavedTimer;
 private:
  void InitSupportPhaseManagers();
  void OptimizeFuncs(MeFuncPhaseManager &fpm, MapleVector<MIRFunction*> &compList);

  MapleAllocator allocator;
  MIRModule &mirModule;
  MapleVector<PhaseManager*> phaseManagers;
  MapleVector<PhaseManager*> supportPhaseManagers;  // Used to check whether a phase is supported and by which manager
  std::string meInput;
  bool timePasses = false;
  time_t genMeMplTime = 0;
  time_t emitVtableImplMplTime = 0;
};
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_INTERLEAVED_MANAGER_H
