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
#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_H
#include "maple_phase_support.h"

namespace maple {
class MaplePhase;
class AnalysisInfoHook;
class MIRModule;
enum MaplePhaseKind : uint8 {
    kModulePM,
    kModulePhase,
    kFunctionPM,
    kFunctionPhase
};

class MaplePhase {
 public:
  MaplePhase(MaplePhaseKind kind, MaplePhaseID id, MemPool &mp)
      : phaseAllocator(&mp),
        phaseKind(kind),
        phaseID(id),
        tempMemPools(phaseAllocator.Adapter()) {}
  virtual ~MaplePhase() {
    if (analysisInfo != nullptr) {
      analysisInfo = nullptr;
    }
  };
  void Dump() const;
  MaplePhaseID GetPhaseID() const {
    return phaseID;
  }
  virtual std::string PhaseName() const = 0;
  void SetAnalysisInfoHook(AnalysisInfoHook *hook);
  AnalysisInfoHook *GetAnalysisInfoHook();

  void AnalysisDepInit(AnalysisDep &aDep) const;
  MapleAllocator *GetPhaseAllocator() {
    return &phaseAllocator;
  }
  MemPool *GetPhaseMemPool() {
    return phaseAllocator.GetMemPool();
  }

  MemPool *ApplyTempMemPool();
  void ClearTempMemPool();

  MapleAllocator phaseAllocator;  //  mempool for phase
  AnalysisInfoHook *analysisInfo = nullptr;
 private:
  virtual void GetAnalysisDependence(AnalysisDep &aDep) const;
  MaplePhaseKind phaseKind;
  MaplePhaseID phaseID;
  MapleVector<MemPool*> tempMemPools; // delete after phase Run  [Thread Local]
};

class MapleModulePhase : public MaplePhase {
 public:
  MapleModulePhase(MaplePhaseID id, MemPool *mp) : MaplePhase(kModulePhase, id, *mp){}
  ~MapleModulePhase() override = default;

  virtual bool PhaseRun(MIRModule &m) = 0;
};

template <class funcT>
class MapleFunctionPhase : public MaplePhase {
 public:
  MapleFunctionPhase(MaplePhaseID id, MemPool *mp) : MaplePhase(kFunctionPhase, id, *mp){}
  ~MapleFunctionPhase() override = default;
  virtual bool PhaseRun(funcT &f) = 0;
};

class MaplePhaseRegister {
 public:
  MaplePhaseRegister() = default;
  ~MaplePhaseRegister() = default;

  static MaplePhaseRegister *GetMaplePhaseRegister();
  void RegisterPhase(const MaplePhaseInfo &PI);
  const MaplePhaseInfo *GetPhaseByID(MaplePhaseID id);
 private:
  std::map<MaplePhaseID, const MaplePhaseInfo*> passInfoMap;
};

template<typename phaseNameT>
class RegisterPhase : public MaplePhaseInfo {
 public:
  RegisterPhase(const std::string name, bool isAnalysis = false, bool CFGOnly = false)
      : MaplePhaseInfo(name, &phaseNameT::id, isAnalysis, CFGOnly) {
    SetConstructor(phaseNameT::CreatePhase);
    auto globalRegistry = maple::MaplePhaseRegister::GetMaplePhaseRegister();
    globalRegistry->RegisterPhase(*this);
  }
  ~RegisterPhase() = default;
};

#define PHASECONSTRUCTOR(PHASENAME)                                  \
  static unsigned int id;                                            \
  static MaplePhase *CreatePhase(MemPool *createMP){                 \
    return createMP->New<PHASENAME>(createMP);                       \
  }

#define OVERRIDE_DEPENDENCE                                             \
 private:                                                               \
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;

#define MAPLE_FUNC_PHASE_DECLARE_BEGIN(PHASENAME, IRTYPE)                  \
class PHASENAME : public MapleFunctionPhase<IRTYPE> {                      \
 public:                                                                   \
  explicit PHASENAME(MemPool *mp) : MapleFunctionPhase<IRTYPE>(&id, mp) {} \
  ~PHASENAME() override = default;                                         \
  std::string PhaseName() const override;                                  \
  static unsigned int id;                                                  \
  static MaplePhase *CreatePhase(MemPool *createMP) {                      \
    return createMP->New<PHASENAME>(createMP);                             \
  }                                                                        \
  bool PhaseRun(IRTYPE &f) override;

#define MAPLE_FUNC_PHASE_DECLARE_END \
};

#define MAPLE_FUNC_PHASE_DECLARE(PHASENAME, IRTYPE)                \
MAPLE_FUNC_PHASE_DECLARE_BEGIN(PHASENAME, IRTYPE)                  \
OVERRIDE_DEPENDENCE                                                \
MAPLE_FUNC_PHASE_DECLARE_END

#define MAPLE_MODULE_PHASE_DECLARE_BEGIN(PHASENAME)                \
class PHASENAME : public MapleModulePhase {                        \
 public:                                                           \
  explicit PHASENAME(MemPool *mp) : MapleModulePhase(&id, mp) {}   \
  ~PHASENAME() override = default;                                 \
  std::string PhaseName() const override;                          \
  static unsigned int id;                                          \
  static MaplePhase *CreatePhase(MemPool *createMP) {              \
    return createMP->New<PHASENAME>(createMP);                     \
  }                                                                \
  bool PhaseRun(MIRModule &m) override;

#define MAPLE_MODULE_PHASE_DECLARE_END \
};

#define MAPLE_MODULE_PHASE_DECLARE(PHASENAME)             \
MAPLE_MODULE_PHASE_DECLARE_BEGIN(PHASENAME)               \
OVERRIDE_DEPENDENCE                                       \
MAPLE_MODULE_PHASE_DECLARE_END

#define MAPLE_ANALYSIS_PHASE_REGISTER(CLASSNAME, PHASENAME)               \
unsigned int CLASSNAME::id = 0;                                           \
std::string CLASSNAME::PhaseName() const { return #PHASENAME; }           \
static RegisterPhase<CLASSNAME> MAPLEPHASE_##PHASENAME(#PHASENAME, true);

#define MAPLE_TRANSFORM_PHASE_REGISTER(CLASSNAME, PHASENAME)              \
unsigned int CLASSNAME::id = 0;                                           \
std::string CLASSNAME::PhaseName() const { return #PHASENAME; }           \
static RegisterPhase<CLASSNAME> MAPLEPHASE_##PHASENAME(#PHASENAME, false);

#define GET_ANALYSIS(PHASENAME) \
static_cast<PHASENAME*>(GetAnalysisInfoHook()->FindAnalysisData(this, &PHASENAME::id))->GetResult()

#define FORCE_GET(PHASENAME) \
static_cast<PHASENAME*>(GetAnalysisInfoHook()-> \
    ForceRunAnalysisPhase<meFuncOptTy, MeFunction>(&PHASENAME::id, f))->GetResult()
}

#endif  // MAPLE_ME_INCLUDE_ME_PHASE_MANAGER_H
