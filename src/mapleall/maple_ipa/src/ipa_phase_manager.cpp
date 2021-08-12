/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_phase_manager.h"
#include "ipa_phase_manager.h"
#include <iostream>
#include <vector>
#include <string>

#define JAVALANG (mirModule.IsJavaModule())
#define CLANG (mirModule.IsCModule())

namespace maple {
bool IpaSccPM::PhaseRun(MIRModule &m) {
  SetQuiet(true);
  DoPhasesPopulate(m);
  bool changed = false;
  auto admMempool = AllocateMemPoolInPhaseManager("Ipa Phase Manager's Analysis Data Manager mempool");
  auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph);
  // Need reverse sccV
  MapleVector<SCCNode*> topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (size_t i = 0; i < phasesSequence.size(); ++i) {
      const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
      changed |= RunTransformPhase<MapleSccPhase<SCCNode>, SCCNode>(*curPhase, *serialADM, **it);
    }
  }
  return changed;
}

void IpaSccPM::DoPhasesPopulate(const MIRModule &mirModule) {
  // AddPhase("propReturnNull", true);
  (void)mirModule;
}

void IpaSccPM::GetAnalysisDependence(AnalysisDep &aDep) const{
  aDep.AddRequired<M2MClone>();
  aDep.AddRequired<M2MCallGraph>();
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.AddPreserved<M2MKlassHierarchy>();
}
}  // namespace maple
