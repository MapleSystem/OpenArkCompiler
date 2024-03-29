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
#include "alias_class.h"
#include "mir_builder.h"
#include "me_irmap.h"
#include "preg_renamer.h"
#include "union_find.h"
#include "me_verify.h"

namespace maple {
void PregRenamer::RunSelf() {
  // BFS the graph of register phi node;
  const MapleVector<MeExpr *> &regmeexprtable = meirmap->GetVerst2MeExprTable();
  MIRPregTable *pregtab = func->GetMirFunc()->GetPregTab();
  std::vector<bool> firstappeartable(pregtab->GetPregTable().size());
  uint32 renameCount = 0;
  UnionFind unionFind(*mp, regmeexprtable.size());
  auto cfg = func->GetCfg();
  // iterate all the bbs' phi to setup the union
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr || bb == cfg->GetCommonEntryBB() || bb == cfg->GetCommonExitBB()) {
      continue;
    }
    MapleMap<OStIdx, MePhiNode *> &mePhiList =  bb->GetMePhiList();
    for (auto it = mePhiList.begin(); it != mePhiList.end(); ++it) {
      OriginalSt *ost = func->GetMeSSATab()->GetOriginalStTable().GetOriginalStFromID(it->first);
      if (!ost->IsPregOst()) { // only handle reg phi
        continue;
      }
      MePhiNode *meRegPhi = it->second;
      size_t vstIdx = meRegPhi->GetLHS()->GetVstIdx();
      size_t nOpnds = meRegPhi->GetOpnds().size();
      for (size_t i = 0; i < nOpnds; ++i) {
        unionFind.Union(vstIdx, meRegPhi->GetOpnd(i)->GetVstIdx());
      }
    }
  }
  std::map<uint32, std::vector<uint32> > root2childrenMap;
  for (uint32 i = 0; i < regmeexprtable.size(); ++i) {
    MeExpr *meexpr = regmeexprtable[i];
    if (!meexpr || meexpr->GetMeOp() != kMeOpReg)
      continue;
    RegMeExpr *regmeexpr = static_cast<RegMeExpr *> (meexpr);
    if (regmeexpr->GetRegIdx() < 0) {
      continue;  // special register
    }
    uint32 rootVstidx = unionFind.Root(i);

    auto mpit = root2childrenMap.find(rootVstidx);
    if (mpit == root2childrenMap.end()) {
      std::vector<uint32> vec(1, i);
      root2childrenMap[rootVstidx] = vec;
    } else {
      std::vector<uint32> &vec = mpit->second;
      vec.push_back(i);
    }
  }

  for (auto it = root2childrenMap.begin(); it != root2childrenMap.end(); ++it) {
    std::vector<uint32> &vec = it->second;
    bool isIntryOrZerov = false; // in try block or zero version
    for (uint32 i = 0; i < vec.size(); ++i) {
      uint32 vstIdx = vec[i];
      ASSERT(vstIdx < regmeexprtable.size(), "over size");
      RegMeExpr *tregMeexpr = static_cast<RegMeExpr *> (regmeexprtable[vstIdx]);
      if (tregMeexpr->GetDefBy() == kDefByNo ||
          tregMeexpr->DefByBB()->GetAttributes(kBBAttrIsTry)) {
        isIntryOrZerov = true;
        break;
      }
    }
    if (isIntryOrZerov) {
      continue;
    }
    // get all the nodes in candidates the same register
    RegMeExpr *regMeexpr = static_cast<RegMeExpr *>(regmeexprtable[it->first]);
    PregIdx newpregidx = regMeexpr->GetRegIdx();
    ASSERT(static_cast<uint32>(newpregidx) < firstappeartable.size(), "oversize ");
    if (!firstappeartable[newpregidx]) {
      // use the previous register
      firstappeartable[newpregidx] = true;
      continue;
    }
    newpregidx = pregtab->ClonePreg(*pregtab->PregFromPregIdx(regMeexpr->GetRegIdx()));
    OriginalSt *newost =
        func->GetMeSSATab()->GetOriginalStTable().CreatePregOriginalSt(newpregidx, func->GetMirFunc()->GetPuidx());
    renameCount++;
    if (DEBUGFUNC(func)) {
      LogInfo::MapleLogger() << "%" << pregtab->PregFromPregIdx(regMeexpr->GetRegIdx())->GetPregNo();
      LogInfo::MapleLogger() << " renamed to %" << pregtab->PregFromPregIdx(newpregidx)->GetPregNo() << std::endl;
    }
    // reneme all the register
    for (uint32 i = 0; i < vec.size(); ++i) {
      RegMeExpr *canregnode =  static_cast<RegMeExpr *> (regmeexprtable[vec[i]]);
      canregnode->SetOst(newost);  // rename it to a new register
    }
  }
}

AnalysisResult *MeDoPregRename::Run(MeFunction *func, MeFuncResultMgr *frm, ModuleResultMgr *mrm) {
  MeIRMap *irmap = static_cast<MeIRMap *>(frm->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  std::string renamePhaseName = PhaseName();
  MemPool *renamemp = memPoolCtrler.NewMemPool(renamePhaseName, true /* isLocalPool */);
  PregRenamer pregrenamer(renamemp, func, irmap);
  pregrenamer.RunSelf();
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "------------after pregrename:-------------------\n";
    func->Dump();
  }
  delete renamemp;
  if (MeOption::meVerify) {
    frm->InvalidAnalysisResult(MeFuncPhase_DOMINANCE, func);
    CHECK_FATAL(mrm != nullptr, "Needs module result manager for ipa");
    auto *dom = static_cast<Dominance*>(frm->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
    ASSERT(dom != nullptr, "dominance phase has problem");
    auto *kh = static_cast<KlassHierarchy*>(mrm->GetAnalysisResult(MoPhase_CHA, &func->GetMIRModule()));
    CHECK_FATAL(kh != nullptr, "kh phase has problem");
    MeVerify verify(*func);
    for (auto &bb : func->GetCfg()->GetAllBBs()) {
      if (bb == nullptr) {
        continue;
      }
      verify.VerifyPhiNode(*bb, *dom);
    }
  }
  return nullptr;
}
}  // namespace maple
