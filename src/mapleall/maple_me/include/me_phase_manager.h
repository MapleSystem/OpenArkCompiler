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
#ifndef MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
#define MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "phase_manager.h"
#include "mir_module.h"
#include "me_phase.h"
#include "mir_function.h"

namespace maple {
enum MePhaseType {
  kMePhaseInvalid,
  kMePhaseMainopt,
  kMePhaseLno
};

// driver of Me
class MeFuncPhaseManager : public PhaseManager {
 public:
  MeFuncPhaseManager(MemPool *memPool, MIRModule &mod, ModuleResultMgr *mrm = nullptr)
      : PhaseManager(*memPool, "mephase"),
        mirModule(mod),
        modResMgr(mrm) {}

  ~MeFuncPhaseManager() {
    arFuncManager.InvalidAllResults();
  }

  void RunFuncPhase(MeFunction *func, MeFuncPhase *phase);
  void RegisterFuncPhases();
  void AddPhases(const std::unordered_set<std::string> &skipPhases);
  void AddPhasesNoDefault(const std::vector<std::string> &phases);
  void SetMePhase(MePhaseType mephase) {
    mePhaseType = mephase;
  }

  void SetModResMgr(ModuleResultMgr *mrm) {
    modResMgr = mrm;
  }

  void Run(MIRFunction *mirFunc, uint64 rangeNum, const std::string &meInput,
           MemPoolCtrler &localMpCtrler = memPoolCtrler);
  void IPACleanUp(MeFunction *mirfunc);
  void Run() override {}

  MeFuncPhaseManager &Clone(MemPool &mp, MemPoolCtrler &ctrler) const;

  MeFuncResultMgr *GetAnalysisResultManager() {
    return &arFuncManager;
  }

  ModuleResultMgr *GetModResultMgr() override {
    return modResMgr;
  }

  bool FuncFilter(const std::string &filter, const std::string &name);

  bool GetGenMeMpl() {
    return genMeMpl;
  }

  void SetGenMeMpl(bool pl) {
    genMeMpl = pl;
  }

  void SetTimePhases(bool phs) {
    timePhases = phs;
  }

  bool IsIPA() const {
    return ipa;
  }

  void SetIPA(bool ipaVal) {
    ipa = ipaVal;
  }

 private:
  // analysis phase result manager
  MeFuncResultMgr arFuncManager{ GetMemAllocator() };
  MIRModule &mirModule;
  ModuleResultMgr *modResMgr;
  MePhaseType mePhaseType = kMePhaseInvalid;
  bool genMeMpl = false;
  bool timePhases = false;
  bool ipa = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
