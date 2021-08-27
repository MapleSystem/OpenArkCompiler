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
#include "foo.h"
#include "module_phase_manager.h"

namespace maple {

void Foo::DoFoo(const maple::SCCNode &scc) {
  (void) mirModule;
  (void) allocator;
  (void) mirBuilder;
  (void) kh;
  return;
}

void SCCFoo::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}

bool SCCFoo::PhaseRun(SCCNode &scc) {
  LogInfo::MapleLogger() << "SCC " << scc.GetID() << " contains\n";
  for (CGNode *node : scc.GetCGNodes()) {
    MIRFunction *func = node->GetMIRFunction();
    if (func != nullptr) {
      if (func->IsEmpty()) {
        continue;
      }
      LogInfo::MapleLogger() << "  function(" << func->GetPuidx() << "): " << func->GetName() << "\n";
    } else {
      LogInfo::MapleLogger() << "  function: external\n";
    }
  }
  std::cout << "foo run" << std::endl;
//  maple::MIRBuilder dexMirBuilder(&m);
//  KlassHierarchy *kh = GET_ANALYSIS(M2MKlassHierarchy);
//  foo = GetPhaseAllocator()->New<Foo>(&m, GetPhaseMemPool(), dexMirBuilder, kh);
//  foo->DoFoo(scc);
  return true;
}
}  // namespace maple
