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
#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#include "phase.h"

namespace maple {
using MaplePhaseID = const void *;
class MaplePhase;
typedef MaplePhase* (*MaplePhase_t)(MemPool*);

/* record every phase known by the system */
class MaplePhaseInfo {
 public:
  MaplePhaseInfo(const std::string &pName, MaplePhaseID pID, bool isAS, bool isCFGonly)
      : phaseName(pName),
        phaseID(pID),
        isAnalysis(isAS),
        isCFGOnlyPass(isCFGonly) {}

  ~MaplePhaseInfo() {
    constructor = nullptr;
    phaseID = nullptr;
  };
  MaplePhaseID GetPhaseID() const {
    return phaseID;
  }
  void SetConstructor(MaplePhase_t newConstructor) {
    constructor = newConstructor;
  }
  MaplePhase_t GetConstructor() const {
    return constructor;
  }
  bool IsAnalysis() const {
    return isAnalysis;
  };
  bool IsCFGonly() const {
    return isCFGOnlyPass;
  };
  std::string PhaseName() const {
    return phaseName;
  }
  MaplePhase_t constructor = nullptr;
 private:
  std::string phaseName;
  MaplePhaseID phaseID;
  const bool isAnalysis ;
  const bool isCFGOnlyPass;
};

// usasge :: analysis dependency
class AnalysisDep {
 public:
  explicit AnalysisDep(MemPool &mp)
      : allocator(&mp),
        required(allocator.Adapter()),
        preserved(allocator.Adapter()),
        preservedExcept(allocator.Adapter()) {};
  template<class PhaseT>
  void AddRequired() {
    required.emplace_back(&PhaseT::id);
  }
  template<class PhaseT>
  void AddPreserved() {
    (void)preserved.emplace(&PhaseT::id);
  }
  template<class PhaseT>
  void PreservedAllExcept(){
    SetPreservedAll();
    (void)preservedExcept.emplace(&PhaseT::id);
  }
  void SetPreservedAll() {
    preservedAll = true;
  }
  bool GetPreservedAll() const {
    return preservedAll;
  }
  bool FindIsPreserved(const MaplePhaseID pid) {
    return preserved.find(pid) != preserved.end();
  }
  const MapleVector<MaplePhaseID> &GetRequiredPhase() const;
  const MapleSet<MaplePhaseID> &GetPreservedPhase() const;
  const MapleSet<MaplePhaseID> &GetPreservedExceptPhase() const;
 private:
  MapleAllocator allocator;
  MapleVector<MaplePhaseID> required;
  MapleSet<MaplePhaseID> preserved;  // keep analysis result as it is
  MapleSet<MaplePhaseID> preservedExcept;  // keep analysis result except
  bool preservedAll = false;
};
}
#endif // MAPLE_PHASE_SUPPORT_H
