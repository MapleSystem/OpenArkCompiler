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
#include "interleaved_manager.h"
#include <string>
#include <vector>
#include <iomanip>
#include "module_phase.h"
#include "mir_function.h"
#include "me_option.h"
#include "mempool.h"
#include "phase_manager.h"
#include "mpl_timer.h"
#include "call_graph.h"
#include "ipa_escape_analysis.h"
#include "me_ssa_devirtual.h"
#include "me_func_opt.h"
#include "thread_env.h"

namespace maple {
std::vector<std::pair<std::string, time_t>> InterleavedManager::interleavedTimer;
void InterleavedManager::AddPhases(const std::vector<std::string> &phases, bool isModulePhase, bool timePhases,
                                   bool genMpl) {
  ModuleResultMgr *mrm = nullptr;
  if (!phaseManagers.empty()) {
    // ModuleResult such class hierarchy need to be carried on
    PhaseManager *pm = phaseManagers.back();
    mrm = pm->GetModResultMgr();
  }

  if (isModulePhase) {
    auto *mpm = GetMemPool()->New<ModulePhaseManager>(GetMemPool(), mirModule, mrm);
    mpm->RegisterModulePhases();
    mpm->AddModulePhases(phases);
    if (timePhases) {
      mpm->SetTimePhases(true);
    }
    phaseManagers.push_back(mpm);
  } else {  // MeFuncPhase
    auto *fpm = GetMemPool()->New<MeFuncPhaseManager>(GetMemPool(), mirModule, mrm);
    fpm->RegisterFuncPhases();
    if (genMpl) {
      fpm->SetGenMeMpl(true);
    }
    if (timePhases) {
      fpm->SetTimePhases(true);
    }
    fpm->AddPhasesNoDefault(phases);
    phaseManagers.push_back(fpm);
  }
}

void InterleavedManager::AddIPAPhases(std::vector<std::string> &phases, bool timePhases, bool genMpl) {
  ModuleResultMgr *mrm = nullptr;
  if (!phaseManagers.empty()) {
    // ModuleResult such class hierarchy needs to be carried on
    PhaseManager *pm = phaseManagers.back();
    mrm = pm->GetModResultMgr();
  }

  auto *fpm = GetMemPool()->New<MeFuncPhaseManager>(GetMemPool(), mirModule, mrm);
  fpm->RegisterFuncPhases();
  if (genMpl) {
    fpm->SetGenMeMpl(true);
  }
  if (timePhases) {
    fpm->SetTimePhases(true);
  }
  fpm->AddPhasesNoDefault(phases);
  phaseManagers.push_back(fpm);
  fpm->SetIPA(true);
}

void InterleavedManager::IPARun(MeFuncPhaseManager &fpm) {
  uint64 rangeNum = 0;
  CHECK_NULL_FATAL(fpm.GetModResultMgr());
  auto *pcg = static_cast<CallGraph*>(fpm.GetModResultMgr()->GetAnalysisResult(MoPhase_CALLGRAPH_ANALYSIS,
                                                                               &mirModule));
  CHECK_NULL_FATAL(pcg);

  const MapleVector<SCCNode*> &sccTopVec = pcg->GetSCCTopVec();
  for (auto nodeIt = sccTopVec.rbegin(); nodeIt != sccTopVec.rend(); ++nodeIt) {
    ++rangeNum;
    SCCNode *curSCCNode = *nodeIt;

    if (IPAEscapeAnalysis::kDebug) {
      LogInfo::MapleLogger() << "<<<<<<<<<<<<<<<<<< SCC BEGIN <<<<<<<<<<<<<<<<<<" << "\n";
      LogInfo::MapleLogger() << "[SCCCOUNT] " << rangeNum << ", [SIZE] " << curSCCNode->GetCGNodes().size() << "\n";
      curSCCNode->Dump();
    }
    bool needConservation = false;
    if (curSCCNode->GetCGNodes().size() > IPAEscapeAnalysis::kFuncInSCCLimit) {
      if (IPAEscapeAnalysis::kDebug) {
        LogInfo::MapleLogger() << "[kFuncInSCCLimit] [SCCCOUNT] " << rangeNum << ", [SIZE] " <<
            curSCCNode->GetCGNodes().size() << "\n";
      }
      needConservation = true;
    }
    bool cgUpdated = true;
    int iterationTime = 0;
    do {
      if (iterationTime == IPAEscapeAnalysis::kSCCConvergenceLimit) {
        if (IPAEscapeAnalysis::kDebug) {
          LogInfo::MapleLogger() << "[kSCCConvergenceLimit] [SCCCOUNT] " << rangeNum << ", [SIZE] " <<
              curSCCNode->GetCGNodes().size() << "\n";
        }
        needConservation = true;
      }
      ++iterationTime;
      if (IPAEscapeAnalysis::kDebug) {
        LogInfo::MapleLogger() << "===========LOOP BEGIN============ " << iterationTime << "\n";
      }

      // At the loop beginning, we set cgUpdated to false, if any eaCG of the function in
      // the scc changed, we set cgUpdated to true.
      cgUpdated = false;
      for (auto *node : curSCCNode->GetCGNodes()) {
        MIRFunction *func = node->GetMIRFunction();
        if (func->GetBody() == nullptr) {
          // No matter whether current node needs update or not,
          // there is no need to run the following code, because the function has no body.
          continue;
        }
        if (func->IsNative() || func->IsAbstract()) {
          continue;
        }
        if (fpm.GetPhaseSequence()->empty()) {
          continue;
        }
        mirModule.SetCurFunction(func);
        if (func->GetEACG() != nullptr) {
          func->GetEACG()->UnSetCGUpdateFlag();
        }
        if (IPAEscapeAnalysis::kDebug) {
          LogInfo::MapleLogger() << "[FUNC_IN_SCC] " << func->GetName() << " [SCCCOUNT] " << rangeNum << "\n";
        }
        if (func->GetMeFunc() != nullptr) {
          if (IPAEscapeAnalysis::kDebug) {
            LogInfo::MapleLogger() << "[HANDLED_BEFORE] skip all phase but ipaea" << "\n";
          }
          if (needConservation) {
            ASSERT_NOT_NULL(func->GetEACG());
            func->GetEACG()->SetNeedConservation();
          }
          fpm.RunFuncPhase(func->GetMeFunc(), static_cast<MeFuncPhase*>(fpm.GetPhaseFromName("ipaea")));
        } else {
          // lower, create BB and build cfg
          fpm.Run(func, rangeNum, meInput);
        }
        cgUpdated = func->GetEACG()->CGHasUpdated();
      }
      if (IPAEscapeAnalysis::kDebug) {
        LogInfo::MapleLogger() << "===========LOOP END============ " << iterationTime << "\n";
      }
      if (needConservation || !curSCCNode->HasRecursion()) {
        break;
      }
    } while (cgUpdated);

    for (auto *node : curSCCNode->GetCGNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      mirModule.SetCurFunction(func);
      if (func->GetEACG() != nullptr && func->GetBody() != nullptr && IPAEscapeAnalysis::kDebug) {
        func->GetEACG()->CountObjEAStatus();
      }
      if (func->GetMeFunc() != nullptr) {
        delete func->GetMeFunc()->GetMemPool();
      }
    }
    fpm.GetAnalysisResultManager()->InvalidAllResults();
    if (IPAEscapeAnalysis::kDebug) {
      LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>> SCC END >>>>>>>>>>>>>>>>>>" << "\n\n";
    }
  }
}

void InterleavedManager::OptimizeFuncs(MeFuncPhaseManager &fpm, MapleVector<MIRFunction*> &compList) {
  for (size_t i = 0; i < compList.size(); ++i) {
    MIRFunction *func = compList[i];
    ASSERT_NOT_NULL(func);
    // when partO2 is set, skip func which not exists in partO2FuncList.
    if (mirModule.HasPartO2List() && !mirModule.IsInPartO2List(func->GetNameStrIdx())) {
      continue;
    }
    // skip empty func, and skip the func out of range  if `useRange` is true
    if (func->GetBody() == nullptr || (MeOption::useRange && (i < MeOption::range[0] || i > MeOption::range[1]))) {
      continue;
    }
    mirModule.SetCurFunction(func);
    // lower, create BB and build cfg
    fpm.Run(func, i, meInput);
  }
}

void InterleavedManager::OptimizeFuncsParallel(const MeFuncPhaseManager &fpm, MapleVector<MIRFunction*> &compList,
                                               uint32 nThreads) {
  auto mainMpCtrler = std::make_unique<MemPoolCtrler>();
  MemPool *mainMp = mainMpCtrler->NewMemPool("main thread mempool", false /* isLocalPool */);
  MeFuncPhaseManager &fpmCopy = fpm.Clone(*mainMp, *mainMpCtrler);
  auto funcOpt = std::make_unique<MeFuncOptExecutor>(std::move(mainMpCtrler), fpmCopy);
  MeFuncOptScheduler funcOptScheduler("me func opt", std::move(funcOpt));
  funcOptScheduler.Init();
  for (size_t i = 0; i < compList.size(); ++i) {
    MIRFunction *func = compList[i];
    ASSERT_NOT_NULL(func);
    // when partO2 is set, skip func which not exists in partO2FuncList.
    if (mirModule.HasPartO2List() && !mirModule.IsInPartO2List(func->GetNameStrIdx())) {
      continue;
    }
    // skip empty func, and skip the func out of range if `useRange` is true
    if (func->GetBody() == nullptr || (MeOption::useRange && (i < MeOption::range[0] || i > MeOption::range[1]))) {
      continue;
    }
    funcOptScheduler.AddFuncOptTask(mirModule, *func, i, meInput);
  }
  // lower, create BB and build cfg
  ThreadEnv::SetMeParallel(true);
  (void)funcOptScheduler.RunTask(nThreads, false);
  ThreadEnv::SetMeParallel(false);
}

void InterleavedManager::Run() {
  MPLTimer optTimer;
  for (auto *pm : phaseManagers) {
    if (pm == nullptr) {
      continue;
    }
    auto *fpm = dynamic_cast<MeFuncPhaseManager*>(pm);
    if (fpm == nullptr) {
      optTimer.Start();
      pm->Run();
      optTimer.Stop();
      LogInfo::MapleLogger() << "[mpl2mpl]" << " Module phases cost " << optTimer.ElapsedMilliseconds() << "ms\n";
      continue;
    }
    if (fpm->GetPhaseSequence()->empty()) {
      continue;
    }
    RunMeOptimize(*fpm);
  }
}

void InterleavedManager::RunMeOptimize(MeFuncPhaseManager &fpm) {
  MapleVector<MIRFunction*> *compList;
  if (!mirModule.GetCompilationList().empty()) {
    if ((mirModule.GetCompilationList().size() != mirModule.GetFunctionList().size()) &&
        (mirModule.GetCompilationList().size() !=
          mirModule.GetFunctionList().size() - mirModule.GetOptFuncsSize())) {
      ASSERT(false, "should be equal");
    }
    compList = &mirModule.GetCompilationList();
  } else {
    compList = &mirModule.GetFunctionList();
  }
  MPLTimer optTimer;
  optTimer.Start();
  std::string logPrefix = mirModule.IsInIPA() ? "[ipa]" : "[me]";
  uint32 nThreads = MeOption::threads;
  bool enableMultithreading = (nThreads > 1);
  if (enableMultithreading && !mirModule.IsInIPA()) {  // parallel
    MeOption::ignoreInferredRetType = true;  // parallel optimization always ignore return type inferred by ssadevirt
    LogInfo::MapleLogger() << logPrefix << " Parallel optimization (" << nThreads << " threads)\n";
    OptimizeFuncsParallel(fpm, *compList, nThreads);
  } else {  // serial
    LogInfo::MapleLogger() << logPrefix << " Serial optimization\n";
    OptimizeFuncs(fpm, *compList);
  }
  optTimer.Stop();
  LogInfo::MapleLogger() << logPrefix << " Function phases cost " << optTimer.ElapsedMilliseconds() << "ms\n";
  optTimer.Start();
  if (fpm.GetGenMeMpl()) {
    mirModule.Emit("comb.me.mpl");
  }
  optTimer.Stop();
  genMeMplTime += optTimer.ElapsedMicroseconds();
}

void InterleavedManager::DumpTimers() {
  std::ios_base::fmtflags f(LogInfo::MapleLogger().flags());
  auto TimeLogger = [](const std::string &itemName, time_t itemTimeUs, time_t totalTimeUs) {
    LogInfo::MapleLogger() << std::left << std::setw(25) << itemName << std::setw(10)
                           << std::right << std::fixed << std::setprecision(2)
                           << (100.0 * itemTimeUs / totalTimeUs) << "%" << std::setw(10)
                           << std::setprecision(0) << (itemTimeUs / 1000.0) << "ms\n";
  };
  std::vector<std::pair<std::string, time_t>> timeVec;
  long total = 0;
  long parserTotal = 0;
  LogInfo::MapleLogger() << "==================== PARSER ====================\n";
  for (const auto &parserTimer : interleavedTimer) {
    parserTotal += parserTimer.second;
  }
  for (const auto &parserTimer : interleavedTimer) {
    TimeLogger(parserTimer.first, parserTimer.second, parserTotal);
  }
  total += parserTotal;
  timeVec.emplace_back(std::pair<std::string, time_t>("parser", parserTotal));
  LogInfo::MapleLogger() << "================== TIMEPHASES ==================\n";
  for (auto *manager : phaseManagers) {
    long temp = manager->DumpTimers();
    total += temp;
    timeVec.push_back(std::pair<std::string, time_t>(manager->GetMgrName(), temp));
    LogInfo::MapleLogger() << "================================================\n";
  }
  total += genMeMplTime;
  total += emitVtableImplMplTime;
  timeVec.emplace_back(std::pair<std::string, time_t>("genMeMplFile", genMeMplTime));
  timeVec.emplace_back(std::pair<std::string, time_t>("emitVtableImplMpl", emitVtableImplMplTime));
  timeVec.emplace_back(std::pair<std::string, time_t>("Total", total));
  LogInfo::MapleLogger() << "=================== SUMMARY ====================\n";
  CHECK_FATAL(total != 0, "calculation check");
  for (const auto &lapse : timeVec) {
    TimeLogger(lapse.first, lapse.second, total);
  }
  LogInfo::MapleLogger() << "================================================\n";
  LogInfo::MapleLogger().flags(f);
}

void InterleavedManager::InitSupportPhaseManagers() {
  ASSERT(supportPhaseManagers.empty(), "Phase managers already initialized");

  auto *mpm = GetMemPool()->New<ModulePhaseManager>(GetMemPool(), mirModule, nullptr);
  mpm->RegisterModulePhases();
  supportPhaseManagers.push_back(mpm);

  ModuleResultMgr *mrm = mpm->GetModResultMgr();

  auto *fpm = GetMemPool()->New<MeFuncPhaseManager>(GetMemPool(), mirModule, mrm);
  fpm->RegisterFuncPhases();
  supportPhaseManagers.push_back(fpm);
}

const PhaseManager *InterleavedManager::GetSupportPhaseManager(const std::string &phase) {
  if (supportPhaseManagers.empty()) {
    InitSupportPhaseManagers();
  }

  for (auto pm : supportPhaseManagers) {
    if (pm->ExistPhase(phase)) {
      return pm;
    }
  }

  return nullptr;
}
} // namespace maple
