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
#ifndef MAPLE_ME_VERIFY_H
#define MAPLE_ME_VERIFY_H
#include "me_ir.h"
#include "me_phase.h"
#include "me_function.h"
#include "me_irmap.h"
#include "class_hierarchy.h"

namespace maple {
void VerifyGlobalTypeTable();

class MeVerify {
 public:
  explicit MeVerify(MeFunction &func) : meFunc(func) {}
  ~MeVerify() = default;

  void VerifyFunction();
  void VerifyPhiNode(const BB &bb, Dominance &dom) const;

  static bool enableDebug;
 private:
  void VerifySuccAndPredOfBB(const BB &bb) const;
  void VerifyBBKind(const BB &bb) const;
  void DealWithSpecialCase(const BB &currBB, const BB &tryBB) const;
  void VerifyNestedTry(const BB &tryBB, const BB &currBB) const;
  void VerifyAttrTryBB(BB &tryBB, int index);
  void VerifyCondGotoBB(const BB &bb) const;
  void VerifyGotoBB(const BB &bb) const;
  void VerifyFallthruBB(const BB &bb) const;
  void VerifyCommonExitBB() const;
  bool IsOnlyHaveReturnOrThrowStmt(const BB &bb, Opcode op) const;
  void VerifyReturnBB(const BB &bb) const;
  void VerifySwitchBB(const BB &bb) const;
  void VerifyPredBBOfSuccBB(const BB &bb, const MapleVector<BB*> &succs) const;

  MeFunction &meFunc;
};

class MeDoVerify : public MeFuncPhase {
 public:
  explicit MeDoVerify(MePhaseID id) : MeFuncPhase(id) {}
  ~MeDoVerify() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*mrm) override;
  std::string PhaseName() const override {
    return "meverify";
  }
};
} // namespace maple
#endif  // MAPLE_ME_VERIFY_H
