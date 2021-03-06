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
#include "me_irmap.h"
#include "dominance.h"
#include "mir_builder.h"

namespace maple {
void MeIRMap::DumpBB(const BB &bb) {
  int i = 0;
  for (const auto &meStmt : bb.GetMeStmts()) {
    if (GetDumpStmtNum()) {
      LogInfo::MapleLogger() << "(" << i++ << ") ";
    }
    meStmt.Dump(this);
  }
}

void MeIRMap::Dump() {
  // back up mempool and use a new mempool every time
  // we dump IRMap, restore the mempool afterwards
  MIRFunction *mirFunction = func.GetMirFunc();
  MemPool *backup = mirFunction->GetCodeMempool();
  mirFunction->SetMemPool(memPoolCtrler.NewMemPool("IR Dump"));
  LogInfo::MapleLogger() << "===================Me IR dump==================\n";
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->DumpHeader(&GetMIRModule());
    LogInfo::MapleLogger() << "frequency : " << bb->GetFrequency() << "\n";
    bb->DumpMeVarPiList(this);
    bb->DumpMeVarPhiList(this);
    bb->DumpMeRegPhiList(this);
    int i = 0;
    for (auto &meStmt : bb->GetMeStmts()) {
      if (GetDumpStmtNum()) {
        LogInfo::MapleLogger() << "(" << i++ << ") ";
      }
      if (meStmt.GetOp() != OP_piassign) {
        meStmt.EmitStmt(GetSSATab()).Dump(0);
      }
      meStmt.Dump(this);
    }
  }
  memPoolCtrler.DeleteMemPool(mirFunction->GetCodeMempool());
  mirFunction->SetMemPool(backup);
}

// this function only emits statement
void MeIRMap::EmitBBStmts(BB &bb, BlockNode &curblk) {
  auto &meStmts = bb.GetMeStmts();
  for (auto &meStmt : meStmts) {
    if (!GetNeedAnotherPass()) {
      if (meStmt.GetOp() == OP_interfaceicall) {
        meStmt.SetOp(OP_icall);
      } else if (meStmt.GetOp() == OP_interfaceicallassigned) {
        meStmt.SetOp(OP_icallassigned);
      }
    }
    StmtNode &stmt = meStmt.EmitStmt(GetSSATab());
    curblk.AddStatement(&stmt);
  }
}

void MeIRMap::EmitBB(BB &bb, BlockNode &curblk) {
  // emit head. label
  LabelIdx labidx = bb.GetBBLabel();
  if (labidx != 0) {
    // not a empty bb
    LabelNode *lbnode = GetSSATab().GetModule().CurFunction()->GetCodeMempool()->New<LabelNode>();
    lbnode->SetLabelIdx(labidx);
    curblk.AddStatement(lbnode);
  }
  EmitBBStmts(bb, curblk);
  if (bb.GetAttributes(kBBAttrIsTryEnd)) {
    // generate op_endtry
    StmtNode *endtry = GetSSATab().GetModule().CurFunction()->GetCodeMempool()->New<StmtNode>(OP_endtry);
    curblk.AddStatement(endtry);
  }
}

AnalysisResult *MeDoIRMap::Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr*) {
  auto *dom = static_cast<Dominance*>(funcResMgr->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  ASSERT(dom != nullptr, "dominance phase has problem");
  MemPool *irMapMemPool = NewMemPool();
  MeIRMap *irMap = irMapMemPool->New<MeIRMap>(*func, *dom, *irMapMemPool, *NewMemPool());
  func->SetIRMap(irMap);
#if DEBUG
  globalIRMap = irMap;
#endif
  std::vector<bool> bbIRMapProcessed(func->NumBBs(), false);
  irMap->BuildBB(*func->GetCommonEntryBB(), bbIRMapProcessed);
  if (DEBUGFUNC(func)) {
    irMap->Dump();
  }
  irMap->GetTempAlloc().SetMemPool(nullptr);
  // delete input IR code for current function
  MIRFunction *mirFunc = func->GetMirFunc();
  mirFunc->GetCodeMempool()->Release();
  mirFunc->SetCodeMemPool(nullptr);
  // delete versionst_table
#if MIR_FEATURE_FULL
  // nullify all references to the versionst_table contents
  for (size_t i = 0; i < func->GetMeSSATab()->GetVersionStTable().GetVersionStVectorSize(); ++i) {
    func->GetMeSSATab()->GetVersionStTable().SetVersionStVectorItem(i, nullptr);
  }
  // clear BB's phi_list_ which uses versionst; nullify first_stmt_, last_stmt_
  auto eIt = func->valid_end();
  for (auto bIt = func->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->ClearPhiList();
    bb->SetFirst(nullptr);
    bb->SetLast(nullptr);
  }
#endif
  func->GetMeSSATab()->GetVersionStTable().GetVSTAlloc().GetMemPool()->Release();
  return irMap;
}
}  // namespace maple
