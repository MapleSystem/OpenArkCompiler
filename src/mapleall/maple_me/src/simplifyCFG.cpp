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
#include "simplifyCFG.h"
#include "me_phase_manager.h"

namespace maple {
static bool debug = false;
#define LOG_BBID(BB) ((BB)->GetBBId().GetIdx())
#define DEBUG_LOG() if (debug)            \
LogInfo::MapleLogger() << "[SimplifyCFG] "
static bool RemoveUnreachableBB(maple::MeFunction &f) {
  // WORK TO BE DONE
  (void)f;
  return false;
}

static bool MergeEmptyReturnBB(maple::MeFunction &f) {
  // WORK TO BE DONE
  (void)f;
  return false;
}

// For BB Level simplification
class SimplifyCFG {
 public:
  SimplifyCFG(BB *bb, MeFunction &func, Dominance *dominance, MeIRMap *hmap,
              MapleMap<OStIdx, MapleSet<BBId>*> *candidates, MemPool *mp, MapleAllocator *ma)
      : currBB(bb), f(func), cfg(func.GetCfg()), dom(dominance), irmap(hmap), cands(candidates), MP(mp), MA(ma) {
    (void)dom; // to be use by some optimization, not used now
    (void)irmap;
  }
  // run on each BB(currBB) until no more change for currBB
  bool RunIterativelyOnBB();

 private:
  BB *currBB = nullptr;     // BB we currently perform simplification on
  MeFunction &f;            // function we currently perform simplification on
  MeCFG *cfg = nullptr;     // requiring cfg to find pattern to simplify
  Dominance *dom = nullptr; // some simplification need to check dominance
  MeIRMap *irmap = nullptr; // used to create new MeExpr/MeStmt
  MapleMap<OStIdx, MapleSet<BBId>*> *cands = nullptr; // candidates ost need to be updated ssa
  MemPool *MP = nullptr;    // for creating elements to cands
  MapleAllocator *MA = nullptr; // for creating elements to cands

  bool runOnSameBBAgain = false; // It will be always set false by RunIterativelyOnBB. If there is some optimization
                                 // opportunity for currBB after a/some simplification, we should set it true.
                                 // Otherwise it will stop after all simplification finished and continue to nextBB.
  enum BBErrStat {
    kBBNoErr,                 // BB is normal
    kBBErrNull,               // BB is nullptr
    kBBErrOOB,                // BB id is out-of-bound
    kBBErrCommonEntryOrExit,  // BB is CommoneEntry or CommonExit
    kBBErrDel                 // BB has been deleted before
  };

  BBErrStat bbErrStat = kBBNoErr;

  // run once time on bb, some common cfg opt and peephole cfg opt
  // will be performed on currBB
  bool RunOnceOnBB();
  // elminate BB that is unreachable:
  // 1.BB has no pred(expect then entry block)
  // 2.BB has itself as pred
  bool EliminateDeadBB();
  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  bool ChangeCondBr2UnCond();
  // eliminate PHI
  // 1.PHI has only one opnd
  // 2.PHI is duplicate(all opnds is the same as another one in the same BB)
  // return true if cfg is changed
  bool EliminatePHI();
  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  bool DisconnectErrorIntroducingPredBB();
  // currBB has only one pred, and pred has only one succ
  // try to merge two BB, return true if merged, return false otherwise.
  bool MergeDistinctBBPair();
  // sink common code to their common succ BB, decrease the register pressure
  bool SinkCommonCode();
  // Following function check for BB pattern according to BB's Kind
  bool SimplifyCondBB();
  bool SimplifyUncondBB();
  bool SimplifyFallthruBB();
  bool SimplifyReturnBB();
  bool SimplifySwitchBB();

  // for sub-pattern in SimplifyCondBB
  bool FoldBranchToCommonDest();

  // Check before every simplification to avoid error induced by other optimization on currBB
  // please use macro CHECK_CURR_BB instead
  bool CheckCurrBB();
  // Insert ost of philist in bb to cand
  void UpdateCand(BB *bb);

  // Remove Pred and add succ's philist to cand
  void RemovePred(BB *succ, BB *pred, bool predNumChanged); // if predNumChanged, we should updatePhi
  // Remove succ and add succ's philist to cand
  void RemoveSucc(BB *pred, BB *succ, bool predNumChanged); // if predNumChanged, we should updatePhi
  // Delete BB and add its philist to cand
  void DeleteBB(BB *bb);

  // merge two bb, return {ifCombined, combinedBB/succ(if not combined)}
  std::pair<bool, BB *>CombineTwoBB(BB *pred, BB *succ);
  void SetBBRunAgain() {
    runOnSameBBAgain = true;
  }
  void ResetBBRunAgain() {
    runOnSameBBAgain = false;
  }
#define CHECK_CURR_BB() \
if (!CheckCurrBB()) {   \
  return false;         \
}
};

bool SimplifyCFG::CheckCurrBB() {
  if (bbErrStat != kBBNoErr) { // has been checked before
    return false;
  }
  if  (currBB == nullptr) {
    bbErrStat = kBBErrNull;
    return false;
  }
  if (currBB->GetBBId() >= cfg->GetAllBBs().size()) {
    bbErrStat = kBBErrOOB;
    return false;
  }
  if (currBB == cfg->GetCommonEntryBB() || currBB == cfg->GetCommonExitBB()) {
    bbErrStat = kBBErrCommonEntryOrExit;
    return false;
  }
  if (cfg->GetBBFromID(currBB->GetBBId()) == nullptr) {
    // BB is deleted, it will be set nullptr in cfg's bbvec, but currBB has been set before(, so it is not null)
    bbErrStat = kBBErrDel;
    return false;
  }
  bbErrStat = kBBNoErr;
  return true;
}

void SimplifyCFG::UpdateCand(BB *bb) {
  if (bb == nullptr || bb->GetMePhiList().empty()) {
    return;
  }
  for (auto phi : bb->GetMePhiList()) {
    OStIdx ostIdx = phi.first;
    if (!phi.second->GetIsLive()) {
      continue;
    }
    if (cands->find(ostIdx) == cands->end()) {
      MapleSet<BBId> *bbSet = MP->New<MapleSet<BBId>>(std::less<BBId>(), MA->Adapter());
      bbSet->insert(bb->GetBBId());
      (*cands)[ostIdx] = bbSet;
    } else {
      (*cands)[ostIdx]->insert(bb->GetBBId());
    }
  }
}

void SimplifyCFG::RemovePred(BB *succ, BB *pred, bool predNumChanged) {
  // if pred num is not changed, its phiOpnd num will be the same as before.
  // and we should not updatephi to remove phiOpnd
  succ->RemovePred(*pred, predNumChanged);
  UpdateCand(succ);
}

void SimplifyCFG::RemoveSucc(BB *pred, BB *succ, bool predNumChanged) {
  // if pred num is not changed, its phiOpnd num will be the same as before.
  // and we should not updatephi to remove phiOpnd
  pred->RemoveSucc(*succ, predNumChanged);
  UpdateCand(succ); // we update cand for succ, because its pred is changed
}

void SimplifyCFG::DeleteBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  UpdateCand(bb);
  cfg->DeleteBasicBlock(*bb);
}

// eliminate dead BB :
// 1.BB has no pred(expect then entry block)
// 2.BB has itself as pred
bool SimplifyCFG::EliminateDeadBB() {
  CHECK_CURR_BB();
  return false;
}

// chang condition branch to unconditon branch if possible
// 1.condition is a constant
// 2.all branches of condition branch is the same BB
bool SimplifyCFG::ChangeCondBr2UnCond() {
  CHECK_CURR_BB();
  return false;
}

// eliminate PHI
// 1.PHI has only one opnd
// 2.PHI is duplicate(all opnds is the same as another one in the same BB)
// return true if cfg is changed
bool SimplifyCFG::EliminatePHI() {
  CHECK_CURR_BB();
  return false;
}

// disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
// If a expr is always cause error in predBB, predBB will never reach currBB
bool SimplifyCFG::DisconnectErrorIntroducingPredBB() {
  CHECK_CURR_BB();
  return false;
}

std::pair<bool, BB *> SimplifyCFG::CombineTwoBB(BB *pred, BB *succ) {
  (void)pred;
  return { false, succ };
}

// currBB has only one pred, and pred has only one succ
// try to merge two BB, return true if merged, return false otherwise.
bool SimplifyCFG::MergeDistinctBBPair() {
  CHECK_CURR_BB();
  return false;
}

// sink common code to their common succ BB, decrease the register pressure
bool SimplifyCFG::SinkCommonCode() {
  CHECK_CURR_BB();
  return false;
}

// CurrBB is Condition BB, we will look upward its predBB(s) to see if we can simplify
// 1. currBB is X == constVal, and predBB has checked for the same expr, the result is known for currBB's condition,
//    so we can make currBB to be an uncondBB.
// 2. predBB is CondBB, one of predBB's succBB is currBB, and another is one of currBB's successors(commonBB)
//    we can merge currBB to predBB if currBB is simple enough(has only one stmt).
// 3. currBB has only one stmt(conditional branch stmt), and the condition's value is calculated by all its predBB
//    we can hoist currBB's stmt to predBBs if it is profitable
bool SimplifyCFG::SimplifyCondBB() {
  CHECK_CURR_BB();
  MeStmt *stmt = currBB->GetLastMe();
  CHECK_FATAL(stmt != nullptr, "[FUNC: %s] CondBB has no stmt", f.GetName().c_str());
  CHECK_FATAL(kOpcodeInfo.IsCondBr(stmt->GetOp()), "[FUNC: %s] Opcode is error!", f.GetName().c_str());
  // 2.fold two continuous condBB to one condBB, use or/and to combine two condition
  if (FoldBranchToCommonDest()) {
    ResetBBRunAgain();
    return true;
  }
  return false;
}

bool SimplifyCFG::SimplifyUncondBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::SimplifyFallthruBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::SimplifyReturnBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::SimplifySwitchBB() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::FoldBranchToCommonDest() {
  CHECK_CURR_BB();
  return false;
}

bool SimplifyCFG::RunOnceOnBB() {
  CHECK_CURR_BB();
  bool everChanged = false;
  // eliminate dead BB :
  // 1.BB has no pred(expect then entry block)
  // 2.BB has itself as pred
  everChanged |= EliminateDeadBB();

  // chang condition branch to unconditon branch if possible
  // 1.condition is a constant
  // 2.all branches of condition branch is the same BB
  everChanged |= ChangeCondBr2UnCond();

  // eliminate PHI
  // 1.PHI has only one opnd
  // 2.PHI is duplicate(all opnds is the same as another one in the same BB)
  // return true if cfg is changed
  everChanged |= EliminatePHI();

  // disconnect predBB and currBB if predBB must cause error(e.g. null ptr deref)
  // If a expr is always cause error in predBB, predBB will never reach currBB
  everChanged |= DisconnectErrorIntroducingPredBB();

  // merge currBB to predBB if currBB has only one predBB and predBB has only one succBB
  everChanged |= MergeDistinctBBPair();

  // sink common code to their common succ BB, decrease the register pressure
  everChanged |= SinkCommonCode();

  switch (currBB->GetKind()) {
    case kBBCondGoto: {
      everChanged |= SimplifyCondBB();
      break;
    }
    case kBBGoto: {
      everChanged |= SimplifyUncondBB();
      break;
    }
    case kBBFallthru: {
      everChanged |= SimplifyFallthruBB();
      break;
    }
    case kBBReturn: {
      everChanged |= SimplifyReturnBB();
      break;
    }
    case kBBSwitch: {
      everChanged |= SimplifySwitchBB();
      break;
    }
    default: {
      // do nothing for the time being
      break;
    }
  }
  return everChanged;
}

// run on each BB until no more change for currBB
bool SimplifyCFG::RunIterativelyOnBB() {
  bool changed = false;
  do {
    runOnSameBBAgain = false;
    // run only once on currBB, if we should run simplify on the same BB, runOnSameBBAgain will be set by optimization
    changed |= RunOnceOnBB();
  } while (runOnSameBBAgain);
  return changed;
}

// For function Level simplification
class SimplifyFuntionCFG {
 public:
  SimplifyFuntionCFG(maple::MeFunction &func, Dominance *dominance, MeIRMap *hmap,
                     MapleMap<OStIdx, MapleSet<BBId>*> *candidates, MemPool *mp, MapleAllocator *ma)
      : f(func), dom(dominance), irmap(hmap), cands(candidates), MP(mp), MA(ma) {}

  bool RunSimplifyOnFunc();

 private:
  MeFunction &f;
  Dominance *dom = nullptr;
  MeIRMap *irmap = nullptr;

  MapleMap<OStIdx, MapleSet<BBId>*> *cands = nullptr; // candidates ost need to update ssa
  MemPool *MP = nullptr;
  MapleAllocator *MA = nullptr;

  bool RepeatSimplifyFunctionCFG();
  bool SimplifyCFGForBB(BB *currBB);
};

bool SimplifyFuntionCFG::SimplifyCFGForBB(BB *currBB) {
  return SimplifyCFG(currBB, f, dom, irmap, cands, MP, MA)
      .RunIterativelyOnBB();
}
// run until no changes happened
bool SimplifyFuntionCFG::RepeatSimplifyFunctionCFG() {
  bool everChanged = false;
  bool changed = true;
  while (changed) {
    changed = false;
    // Since we may delete BB during traversal, we cannot use iterator here.
    auto &bbVec = f.GetCfg()->GetAllBBs();
    for (size_t idx = 0; idx < bbVec.size(); ++idx) {
      BB *currBB = bbVec[idx];
      if (currBB == nullptr) {
        continue;
      }
      changed |= SimplifyCFGForBB(currBB);
    }
    everChanged |= changed;
  }
  return everChanged;
}

bool SimplifyFuntionCFG::RunSimplifyOnFunc() {
  bool everChanged = RemoveUnreachableBB(f);
  everChanged |= MergeEmptyReturnBB(f);
  everChanged |= RepeatSimplifyFunctionCFG();
  if (!everChanged) {
    return false;
  }
  // RepeatSimplifyFunctionCFG may generate unreachable BB.
  // So RemoveUnreachableBB should be called to check for and
  // remove dead BB. And RemoveUnreachableBB may also generate
  // other optimize opportunity for RepeatSimplifyFunctionCFG.
  // Hench we should iterate between these two optimizations.
  // Here, we call RemoveUnreachableBB first to avoid running
  // RepeatSimplifyFunctionCFG for no changed situation.
  if (!RemoveUnreachableBB(f)) {
    return true;
  }
  do {
    everChanged = RepeatSimplifyFunctionCFG();
    everChanged |= RemoveUnreachableBB(f);
  } while (everChanged);
  return true;
}

static bool SkipSimplifyCFG(maple::MeFunction &f) {
  for (auto bbIt = f.GetCfg()->valid_begin(); bbIt != f.GetCfg()->valid_end(); ++bbIt) {
    if ((*bbIt)->GetKind() == kBBIgoto) {
      return true;
    }
  }
  return false;
}

void MESimplifyCFG::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MESimplifyCFG::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  auto *irmap = GET_ANALYSIS(MEIRMapBuild, f);
  debug = DEBUGFUNC_NEWPM(f);
  if (SkipSimplifyCFG(f)) {
    DEBUG_LOG() << "Skip SimplifyCFG phase because of igotoBB\n";
    return false;
  }
  DEBUG_LOG() << "Start Simplifying Function : " << f.GetName() << "\n";
  if (debug) {
    f.GetCfg()->DumpToFile("Before_SimplifyCFG");
  }
  auto *MP = GetPhaseMemPool();
  MapleAllocator MA = MapleAllocator(MP);
  MapleMap<OStIdx, MapleSet<BBId>*> cands((std::less<OStIdx>(), MA.Adapter()));
  // simplify entry
  bool change = SimplifyFuntionCFG(f, dom, irmap, &cands, MP, &MA).RunSimplifyOnFunc();
  if (change) {
    if (debug) {
      f.GetCfg()->DumpToFile("After_SimplifyCFG");
      f.Dump(false);
    }
    // we should invalid dom here FORCE_INVALID(MEDominance, f);
    dom = FORCE_GET(MEDominance);
    if (!cands.empty()) {
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, cands, *MP);
      ssaUpdate.Run();
    }
  }
  return change;
}
} // namespace maple

