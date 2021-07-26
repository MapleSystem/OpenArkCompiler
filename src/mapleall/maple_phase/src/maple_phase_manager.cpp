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
 * See the Mulan PSL v2 for more details.
 */
#include "maple_phase_manager.h"
#include "cgfunc.h"
#include "mpl_timer.h"

namespace maple {
using meFuncOptTy = MapleFunctionPhase<MeFunction>;
using cgFuncOptTy = MapleFunctionPhase<maplebe::CGFunc>;
MemPool *AnalysisDataManager::ApplyMemPoolForAnalysisPhase(const MaplePhaseInfo &pi) {
  std::string mempoolName = pi.PhaseName() + " memPool";
  MemPool *phaseMempool = nullptr;
  if (UseGlobalMpCtrler()) {
    phaseMempool = memPoolCtrler.NewMemPool(mempoolName, true);
  } else {
    phaseMempool = innerCtrler->NewMemPool(mempoolName, true);
  }

  (void)analysisPhaseMemPool.emplace(std::pair<MaplePhaseID, MemPool*>(pi.GetPhaseID(), phaseMempool));
  return phaseMempool;
}

void AnalysisDataManager::AddAnalysisPhase(MaplePhase *p) {
  CHECK_FATAL(p != nullptr, "invalid phase when AddAnalysisPhase"); // change to assert after testing
  (void)availableAnalysisPhases.emplace(std::pair<MaplePhaseID, MaplePhase*>(p->GetPhaseID(), p));
}

// Erase phase at O2
void AnalysisDataManager::EraseAnalysisPhase(MaplePhaseID pid) {
  auto it = analysisPhaseMemPool.find(pid);
  auto itanother = availableAnalysisPhases.find(pid);
  if (it != analysisPhaseMemPool.end() && itanother != availableAnalysisPhases.end()) {
    auto resultanother = availableAnalysisPhases.erase(pid);
    CHECK_FATAL(resultanother, "Release Failed");
    delete it->second;
    it->second = nullptr;
    auto result = analysisPhaseMemPool.erase(pid);  // erase to release mempool ?
    CHECK_FATAL(result, "Release Failed");
  }
}

// Erase safely
void AnalysisDataManager::EraseAnalysisPhase(MapleMap<MaplePhaseID, MaplePhase*>::iterator &anaPhaseMapIt) {
  auto it = analysisPhaseMemPool.find(anaPhaseMapIt->first);
  if (it != analysisPhaseMemPool.end()) {
    anaPhaseMapIt = availableAnalysisPhases.erase(anaPhaseMapIt);
    delete it->second;
    it->second = nullptr;
#ifdef DEBUG
    bool result = analysisPhaseMemPool.erase(it->first);  // erase to release mempool
#else
    (void)analysisPhaseMemPool.erase(it->first);
#endif
    ASSERT(result, "Release Failed");
#ifdef DEBUG
  } else {
    ASSERT(false, "cannot delete phase which is not exist  &&  mempool is not create TOO");
#endif
  }
}

void AnalysisDataManager::ClearInVaildAnalysisPhase(AnalysisDep &ADep) {
  if (!ADep.GetPreservedAll()) {
    // delete phases which are not preserved
    if (ADep.GetPreservedPhase().empty()) {
      for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
        EraseAnalysisPhase(it);
      }
    }
    for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
      if (!ADep.FindIsPreserved(it->first)) {
        EraseAnalysisPhase(it);
      } else {
        ++it;
      }
    }
  } else {
    if (!ADep.GetPreservedExceptPhase().empty()) {
      for (auto exceptPhaseID : ADep.GetPreservedExceptPhase()) {
        auto it = availableAnalysisPhases.find(exceptPhaseID);
        CHECK_FATAL(it != availableAnalysisPhases.end(), " Phase is not in mempool");
        EraseAnalysisPhase(it);
      }
    }
  }
}

MaplePhase *AnalysisDataManager::GetVaildAnalysisPhase(MaplePhaseID pid) {
  auto it = availableAnalysisPhases.find(pid);
  if (it == availableAnalysisPhases.end()) {
    LogInfo::MapleLogger() << "Required " <<
        MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(pid)->PhaseName() << " running before \n";
    CHECK_FATAL(false, "find analysis phase failed");
    return nullptr;
  } else {
    return it->second;
  }
}

bool AnalysisDataManager::IsAnalysisPhaseAvailable(MaplePhaseID pid) {
  auto it = availableAnalysisPhases.find(pid);
  return it != availableAnalysisPhases.end();
}

void MaplePhaseManager::AddPhase(MaplePhaseID pid, bool condition) {
  if (condition) {
    phasesSequence.emplace_back(pid);
  }
}

void MaplePhaseManager::AddPhaseTime(MaplePhaseID pid, long phaseTime) {
  if (phaseTimers.count(pid)) {
    phaseTimers[pid] += phaseTime;
  } else {
    (void)phaseTimers.emplace(std::make_pair(pid, phaseTime));
  }
}

void MaplePhaseManager::DumpPhaseTime() {
  auto TimeLogger = [](const std::string &itemName, time_t itemTimeUs, time_t totalTimeUs) {
    LogInfo::MapleLogger() << std::left << std::setw(25) << itemName <<
                           std::setw(10) << std::right << std::fixed << std::setprecision(2) <<
                           (maplebe::kPercent * itemTimeUs / totalTimeUs) << "%" << std::setw(10) <<
                           std::setprecision(0) << (itemTimeUs / maplebe::kMicroSecPerMilliSec) << "ms\n";
  };
  int64 phasesTotal = 100000;

  // std::ios::fmtflags flag(LogInfo::MapleLogger().flags());
  LogInfo::MapleLogger() << "================== TIMEPHASES ==================\n";
  LogInfo::MapleLogger() << "================================================\n";
  for (auto phaseIt : phasesSequence) {
    /*
     * output information by specified format, setw function parameter specifies show width
     * setprecision function parameter specifies precision
     */
    const MaplePhaseInfo* phaseInfo = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phaseIt);
    if (!phaseInfo->IsAnalysis()) {
      TimeLogger(phaseInfo->PhaseName(), GetPhaseTime(phaseIt), phasesTotal);
    }
  }
  LogInfo::MapleLogger() << "================================================\n";
}

AnalysisDep *MaplePhaseManager::FindAnalysisDep(const MaplePhase *phase) {
  AnalysisDep *anDependence = nullptr;
  auto anDepIt = analysisDepMap.find(phase->GetPhaseID());
  if (anDepIt != analysisDepMap.end()) {
    anDependence = anDepIt->second;
  } else {
    anDependence = allocator.New<AnalysisDep>(*GetManagerMemPool());
    phase->AnalysisDepInit(*anDependence);
    (void)analysisDepMap.emplace(std::pair<MaplePhaseID, AnalysisDep*>(phase->GetPhaseID(), anDependence));
  }
  return anDependence;
}

AnalysisDataManager *MaplePhaseManager::ApplyAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP) {
  auto *adm = threadMP.New<AnalysisDataManager>(threadMP);
#ifdef DEBUG
  auto result = analysisDataManagers.emplace(std::pair<std::thread::id, AnalysisDataManager*>(threadID, adm));
#else
  (void)analysisDataManagers.emplace(std::pair<std::thread::id, AnalysisDataManager*>(threadID, adm));
#endif
  ASSERT(adm != nullptr && result.second, "apply AnalysisDataManager failed");
  return adm;
}

AnalysisDataManager *MaplePhaseManager::GetAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP) {
  auto admIt = analysisDataManagers.find(threadID);
  if (admIt != analysisDataManagers.end()) {
    return admIt->second;
  } else {
    return ApplyAnalysisDataManager(threadID, threadMP);
  }
}

std::unique_ptr<ThreadLocalMemPool> MaplePhaseManager::AllocateMemPoolInPhaseManager(const std::string &mempoolName) {
  if (!UseGlobalMpCtrler()) {
    LogInfo::MapleLogger() << " Inner Ctrler has not been supported yet \n";
  }
  return std::make_unique<ThreadLocalMemPool>(memPoolCtrler, mempoolName);
}

template <typename phaseT, typename IRTemplate>
void MaplePhaseManager::RunDependentAnalysisPhase(const MaplePhase &phase,
                                                  AnalysisDataManager &adm,
                                                  IRTemplate &irUnit) {
  AnalysisDep *anaDependence = FindAnalysisDep(&phase);
  for (auto requiredAnaPhase : anaDependence->GetRequiredPhase()) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(requiredAnaPhase);
    CHECK_FATAL(curPhase->IsAnalysis(), "Must be analysis phase.");
    RunAnalysisPhase<phaseT, IRTemplate>(*curPhase, adm, irUnit);
  }
}

/* live range of a phase should be short than mempool */
template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunTransformPhase(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit) {
  auto transformPhaseMempool = AllocateMemPoolInPhaseManager(phaseInfo.PhaseName() + "'s mempool");
  auto *phase = static_cast<phaseT*>(phaseInfo.GetConstructor()(transformPhaseMempool.get()));
  RunDependentAnalysisPhase<phaseT, IRTemplate>(*phase, adm, irUnit);
  phase->SetAnalysisInfoHook(transformPhaseMempool->New<AnalysisInfoHook>(*(transformPhaseMempool.get()), adm, this));
  bool result = phase->PhaseRun(irUnit);
  phase->ClearTempMemPool();
  adm.ClearInVaildAnalysisPhase(*FindAnalysisDep(phase));
  return result;
}

template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunAnalysisPhase(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit) {
  bool result = false;
  phaseT *phase = nullptr;
  if (adm.IsAnalysisPhaseAvailable(phaseInfo.GetPhaseID())) {
    return result;
  }
  MemPool *anasPhaseMempool = adm.ApplyMemPoolForAnalysisPhase(phaseInfo);
  phase = static_cast<phaseT*>(phaseInfo.GetConstructor()(anasPhaseMempool));
  RunDependentAnalysisPhase<phaseT, IRTemplate>(*phase, adm, irUnit);
  // change analysis info hook mempool from ADM Allocator to phase allocator?
  phase->SetAnalysisInfoHook(anasPhaseMempool->New<AnalysisInfoHook>(*anasPhaseMempool, adm, this));
  result |= phase->PhaseRun(irUnit);
  phase->ClearTempMemPool();
  adm.AddAnalysisPhase(phase);
  return result;
}

// declaration for functionPhase (template only)
template bool MaplePhaseManager::RunTransformPhase<MapleModulePhase, MIRModule>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MIRModule &irUnit);
template bool MaplePhaseManager::RunAnalysisPhase<MapleModulePhase, MIRModule>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MIRModule &irUnit);
template bool MaplePhaseManager::RunTransformPhase<meFuncOptTy, MeFunction>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MeFunction &irUnit);
template bool MaplePhaseManager::RunAnalysisPhase<meFuncOptTy, MeFunction>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MeFunction &irUnit);
template bool MaplePhaseManager::RunTransformPhase<cgFuncOptTy, maplebe::CGFunc>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, maplebe::CGFunc &irUnit);
template bool MaplePhaseManager::RunAnalysisPhase<cgFuncOptTy, maplebe::CGFunc>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, maplebe::CGFunc &irUnit);
}
