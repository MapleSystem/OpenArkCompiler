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
#ifndef MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
#define MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
#include "module_phase.h"
#include "phase_impl.h"

namespace maple {
#ifdef USE_ARM32_MACRO
constexpr int kSlownativeFuncnum = 5;
#else
constexpr int kSlownativeFuncnum = 9;
#endif
// May not use Env or ClassObj
constexpr int kJniTypeNormal = 0;
// Not equal to real critical native, but no need for pre/post/eh.
// Need SetReliableUnwindContext, because the native code may call other java code
constexpr int kJniTypeMapleCriticalNative = 1;
// Equal to real critical native, func will be set critical attribute.
// In theory it's incorrect because passing incorrect args. Ex. Linux.getuid
constexpr int kJniTypeCriticalNative = 2;
// Equal to critical native but need to pass env and classObj. Ex. Character.isDigitImpl
constexpr int kJniTypeCriticalNeedArg = 3;
constexpr int kInvalidCode = 0x01;

class NativeFuncProperty {
 public:
  NativeFuncProperty() = default;
  ~NativeFuncProperty() = default;

 private:
  std::string javaFunc;
  std::string nativeFile;
  std::string nativeFunc;
  int jniType = kJniTypeNormal;
  int useEnv = 1;
  int useClassObj = 1;

  friend class NativeStubFuncGeneration;
};

class NativeStubFuncGeneration : public FuncOptimizeImpl {
 public:
  NativeStubFuncGeneration(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~NativeStubFuncGeneration() = default;

  void ProcessFunc(MIRFunction *func) override;
  void Finish() override;
  FuncOptimizeImpl *Clone() override {
    return new NativeStubFuncGeneration(*this);
  }

 private:
  bool IsStaticBindingListMode() const;
  bool ReturnsJstr(TyIdx retType) {
    return retType == jstrPointerTypeIdx;
  }
  void InitStaticBindingMethodList();
  bool IsStaticBindingMethod(const std::string &methodName) const;
  MIRFunction &GetOrCreateDefaultNativeFunc(MIRFunction &stubFunc);
  void GenerateRegisteredNativeFuncCall(MIRFunction &func, const MIRFunction &nativeFunc, MapleVector<BaseNode*> &args,
                                        const MIRSymbol *ret);
  StmtNode *CreateNativeWrapperCallNode(MIRFunction &func, BaseNode *funcPtr, MapleVector<BaseNode*> &args,
                                        const MIRSymbol *ret, bool needIndirectCall);
  void GenerateNativeWrapperFuncCall(MIRFunction &func, const MIRFunction &nativeFunc, MapleVector<BaseNode*> &args,
                                     const MIRSymbol *ret);
  void GenerateHelperFuncDecl();
  void GenerateRegTabEntry(const MIRFunction &func);
  void GenerateRegTableEntryType();
  void GenerateRegTable();
  void GenerateRegFuncTabEntryType();
  void GenerateRegFuncTabEntry();
  void GenerateRegFuncTab();
  // a static binding function list
  std::unordered_set<std::string> staticBindingMethodsSet;
  TyIdx jstrPointerTypeIdx = TyIdx(0);
  MIRAggConst *regTableConst = nullptr;
  MIRSymbol *regFuncSymbol = nullptr;
  MIRAggConst *regFuncTabConst = nullptr;
  MIRFunction *MRTPreNativeFunc = nullptr;
  MIRFunction *MRTPostNativeFunc = nullptr;
  MIRFunction *MRTDecodeRefFunc = nullptr;
  MIRFunction *MRTCheckThrowPendingExceptionFunc = nullptr;
  MIRFunction *MRTCallSlowNativeFunc[kSlownativeFuncnum] = { nullptr };  // for native func which args <=8, use x0-x7
  MIRFunction *MRTCallSlowNativeExtFunc = nullptr;
  MIRFunction *MCCSetReliableUnwindContextFunc = nullptr;
  static const std::string callSlowNativeFuncs[kSlownativeFuncnum];
};

class DoGenerateNativeStubFunc : public ModulePhase {
 public:
  explicit DoGenerateNativeStubFunc(ModulePhaseID id) : ModulePhase(id) {}

  ~DoGenerateNativeStubFunc() = default;

  std::string PhaseName() const override {
    return "GenNativeStubFunc";
  }

  AnalysisResult *Run(MIRModule *mod, ModuleResultMgr *mrm) override {
    bool origUsePreg = Options::usePreg;
    Options::usePreg = false;  // As a pre mpl2mpl phase, NativeStubFunc always use symbols
    OPT_TEMPLATE(NativeStubFuncGeneration);
    Options::usePreg = origUsePreg;
    return nullptr;
  }
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
