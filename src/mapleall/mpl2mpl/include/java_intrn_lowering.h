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
#ifndef MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
#define MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
#include <map>
#include <unordered_set>
#include "phase_impl.h"
#include "module_phase.h"

namespace maple {
class JavaIntrnLowering : public FuncOptimizeImpl {
 public:
  JavaIntrnLowering(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~JavaIntrnLowering() = default;

  FuncOptimizeImpl *Clone() override {
    return new JavaIntrnLowering(*this);
  }

 private:
  void InitTypes();
  void InitFuncs();
  void InitLists();
  void ProcessStmt(StmtNode &stmt) override;
  void ProcessJavaIntrnMerge(StmtNode &assignNode, const IntrinsicopNode &intrinNode);
  BaseNode *JavaIntrnMergeToCvtType(PrimType dtyp, PrimType styp, BaseNode *src);
  void LoadClassLoaderInvocation(const std::string &list);
  void CheckClassLoaderInvocation(const CallNode &callNode) const;
  void DumpClassLoaderInvocation(const CallNode &callNode);
  void ProcessForNameClassLoader(CallNode &callnode);
  void ProcessJavaIntrnFillNewArray(IntrinsiccallNode &intrinCall);
  std::string outFileName;
  std::unordered_set<std::string> clInterfaceSet;
  std::multimap<std::string, std::string> clInvocationMap;
  MIRFunction *classForName1Func = nullptr;
  MIRFunction *classForName3Func = nullptr;
  MIRFunction *getCurrentClassLoaderFunc = nullptr;
  MIRType *classLoaderPointerToType = nullptr;
};

class DoJavaIntrnLowering : public ModulePhase {
 public:
  explicit DoJavaIntrnLowering(ModulePhaseID id) : ModulePhase(id) {}

  ~DoJavaIntrnLowering() = default;

  std::string PhaseName() const override {
    return "javaintrnlowering";
  }

  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override {
    OPT_TEMPLATE(JavaIntrnLowering);
    return nullptr;
  }
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_JAVA_INTRN_LOWERING_H
