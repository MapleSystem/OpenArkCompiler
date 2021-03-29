/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_PREME_H
#define MPL2MPL_INCLUDE_PREME_H
#include "module_phase.h"
#include "phase_impl.h"
#include "me_option.h"

namespace maple {
class Preme : public FuncOptimizeImpl {
 public:
  Preme(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}
  ~Preme() = default;
  // Create global symbols and functions here when iterating mirFunc is needed
  void ProcessFunc(MIRFunction *func) override;
  FuncOptimizeImpl *Clone() override {
    return new Preme(*this);
  }

 private:
  void CreateMIRTypeForAddrof(const MIRFunction &func, const BaseNode *baseNode) const;
};

class DoPreme : public ModulePhase {
 public:
  explicit DoPreme(ModulePhaseID id) : ModulePhase(id) {}
  ~DoPreme() = default;
  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "preme";
  }

 private:
  void CreateMIRTypeForLowerGlobalDreads() const;
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_PREME_H
