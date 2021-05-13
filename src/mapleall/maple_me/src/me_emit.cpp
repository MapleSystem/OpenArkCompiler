/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_emit.h"
#include <mutex>
#include "thread_env.h"
#include "me_bb_layout.h"
#include "me_irmap.h"
#include "me_cfg.h"
#include "constantfold.h"

static const std::set<std::string> kHssaPhases{
  "irmapbuild",  "hprop",      "hssa",        "hdse",        "ssadevirt",    "ea",          "epre",
  "lpre",   "stmtpre",    "rename2preg", "ti",          "analyzerc",
  "spre",   "rclowering", "delegaterc",  "condbasedrc", "condbasednpc", "may2dassign", "pregrename",
  "bdcopt", "syncselect", "cfgopt",      "placementrc",  "symrename",   "checkcastopt", "bblayout",
};

namespace maple {
// emit IR to specified file
AnalysisResult *MeDoEmit::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) {
  static std::mutex mtx;
  ParallelGuard guard(mtx, ThreadEnv::IsMeParallel());
  if (func->NumBBs() > 0) {
    std::string passName = GetPreviousPhaseName();
    bool emitHssaOrAfter = kHssaPhases.find(std::string(passName)) != kHssaPhases.end();
    if (emitHssaOrAfter) {
      CHECK_FATAL(func->GetIRMap() != nullptr, "Why not hssa?");
      // generate bblist after layout (bb physical position)
      CHECK_FATAL(func->HasLaidOut(), "Check/Run bb layout phase.");
      auto layoutBBs = func->GetLaidOutBBs();
      MIRFunction *mirFunction = func->GetMirFunc();
      mirFunction->ReleaseCodeMemory();

      mirFunction->SetMemPool(new ThreadLocalMemPool(memPoolCtrler, "IR from IRMap::Emit()"));
      mirFunction->SetBody(mirFunction->GetCodeMempool()->New<BlockNode>());
      // initialize is_deleted field to true; will reset when emitting Maple IR
      for (size_t k = 1; k < mirFunction->GetSymTab()->GetSymbolTableSize(); ++k) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(k);
        if (sym->GetSKind() == kStVar) {
          sym->SetIsDeleted();
        }
      }
      for (BB *bb : layoutBBs) {
        ASSERT(bb != nullptr, "Check bblayout phase");
        bb->EmitBB(*func->GetMeSSATab(), *mirFunction->GetBody(), false);
      }
    } else {
      if(!func->HasLaidOut()) {
        (void)m->GetAnalysisResult(MeFuncPhase_BBLAYOUT, func);
        CHECK_FATAL(func->HasLaidOut(), "Check bb layout phase.");
      }
      func->EmitBeforeHSSA((*(func->GetMirFunc())), func->GetLaidOutBBs());
    }
    if (!DEBUGFUNC(func)) {
      // constantfolding does not update BB's stmtNodeList, which breaks MirCFG::DumpToFile()
      ConstantFold cf(func->GetMIRModule());
      cf.Simplify(func->GetMirFunc()->GetBody());
    }
    if (DEBUGFUNC(func)) {
      LogInfo::MapleLogger() << "\n==============after meemit =============" << '\n';
      func->GetMirFunc()->Dump();
    }
    if (DEBUGFUNC(func)) {
      func->GetTheCfg()->DumpToFile("emit", true);
    }
  }
  return nullptr;
}
}  // namespace maple
