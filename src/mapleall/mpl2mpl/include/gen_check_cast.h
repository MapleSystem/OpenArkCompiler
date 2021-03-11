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
#ifndef MPL2MPL_INCLUDE_GEN_CHECK_CAST_H
#define MPL2MPL_INCLUDE_GEN_CHECK_CAST_H
#include "class_hierarchy.h"
#include "module_phase.h"
#include "phase_impl.h"

namespace maple {
class PreCheckCast : public FuncOptimizeImpl {
 public:
  PreCheckCast(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~PreCheckCast() {}
  FuncOptimizeImpl *Clone() override {
    return new PreCheckCast(*this);
  }

  void ProcessFunc(MIRFunction *func) override;
};

class CheckCastGenerator : public FuncOptimizeImpl {
 public:
  CheckCastGenerator(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~CheckCastGenerator() {}

  FuncOptimizeImpl *Clone() override {
    return new CheckCastGenerator(*this);
  }

  void ProcessFunc(MIRFunction *func) override;

 private:
  void InitTypes();
  void InitFuncs();
  void GenAllCheckCast();
  void GenCheckCast(StmtNode &stmt);
  BaseNode *GetObjectShadow(BaseNode *opnd);
  MIRSymbol *GetOrCreateClassInfoSymbol(const std::string &className);
  void GenAllCheckCast(bool isHotFunc);
  void OptimizeInstanceof();
  void OptimizeIsAssignableFrom();
  void CheckIsAssignableFrom(BlockNode &blockNode, StmtNode &stmt, const IntrinsicopNode &intrinsicNode);
  void ConvertCheckCastToIsAssignableFrom(StmtNode &stmt);
  void AssignedCastValue(StmtNode &stmt);
  void ConvertInstanceofToIsAssignableFrom(StmtNode &stmt, const IntrinsicopNode &intrinsicNode);
  void ReplaceNoSubClassIsAssignableFrom(BlockNode &blockNode, StmtNode &stmt, const MIRPtrType &ptrType,
                                         const IntrinsicopNode &intrinsicNode);
  void ReplaceIsAssignableFromUsingCache(BlockNode &blockNode, StmtNode &stmt, const MIRPtrType &targetClassType,
                                         const IntrinsicopNode &intrinsicNode);
  bool IsDefinedConstClass(StmtNode &stmt, const MIRPtrType &targetClassType,
                           PregIdx &classSymPregIdx, MIRSymbol *&classSym);
  MIRType *pointerObjType = nullptr;
  MIRType *pointerClassMetaType = nullptr;
  MIRType *classinfoType = nullptr;
  MIRFunction *throwCastException = nullptr;
  MIRFunction *checkCastingNoArray = nullptr;
  MIRFunction *checkCastingArray = nullptr;
  const std::unordered_set<std::string> funcWithoutCastCheck{
#include "castcheck_whitelist.def"
  };
  MIRFunction *mccIsAssignableFrom = nullptr;
  MIRFunction *castExceptionFunc = nullptr;
};

class DoCheckCastGeneration : public ModulePhase {
 public:
  explicit DoCheckCastGeneration(ModulePhaseID id) : ModulePhase(id) {}

  ~DoCheckCastGeneration() = default;

  std::string PhaseName() const override {
    return "gencheckcast";
  }

  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override {
    OPT_TEMPLATE(CheckCastGenerator);
    mrm->InvalidAnalysisResult(MoPhase_ANNOTATIONANALYSIS, mod);
    return nullptr;
  }
};

class DoPreCheckCast : public ModulePhase {
 public:
  explicit DoPreCheckCast(ModulePhaseID id) : ModulePhase(id) {}
  ~DoPreCheckCast() = default;
  std::string PhaseName() const override {
    return "precheckcast";
  }
  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override {
    OPT_TEMPLATE(PreCheckCast);
    return nullptr;
  }
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_GEN_CHECK_CAST_H
