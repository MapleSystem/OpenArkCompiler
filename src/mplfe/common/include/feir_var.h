/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_INCLUDE_FEIR_VAR_H
#define MPLFE_INCLUDE_FEIR_VAR_H
#include <memory>
#include "mir_type.h"
#include "mir_symbol.h"
#include "mir_module.h"
#include "mir_builder.h"
#include "feir_type.h"

namespace maple {
enum FEIRVarTransKind : uint8 {
  kFEIRVarTransDefault = 0,
  kFEIRVarTransDirect,
  kFEIRVarTransArrayDimIncr,
  kFEIRVarTransArrayDimDecr,
};

class FEIRVar;
class FEIRVarTrans {
 public:
  FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar);
  FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar, uint8 dimDelta);
  ~FEIRVarTrans() = default;
  UniqueFEIRType GetType(const UniqueFEIRType &type, PrimType primType = PTY_ref, bool usePtr = true);
  std::unique_ptr<FEIRVar> &GetVar() const {
    return var;
  }

  void SetTransKind(FEIRVarTransKind argKind) {
    kind = argKind;
  }

  FEIRVarTransKind GetTransKind() const {
    return kind;
  }

 private:
  FEIRVarTransKind kind;
  std::unique_ptr<FEIRVar> &var;
  union {
    uint8 dimDelta;
  } param;
};

using UniqueFEIRVarTrans = std::unique_ptr<FEIRVarTrans>;

enum FEIRVarKind : uint8 {
  kFEIRVarDefault = 0,
  kFEIRVarReg,
  kFEIRVarAccumulator,
  kFEIRVarName,
  kFEIRVarTypeScatter,
};

class FEIRVar {
 public:
  FEIRVar(FEIRVarKind argKind);
  FEIRVar(FEIRVarKind argKind, std::unique_ptr<FEIRType> argType);
  virtual ~FEIRVar() = default;
  void SetType(std::unique_ptr<FEIRType> argType);
  FEIRVarKind GetKind() const {
    return kind;
  }

  const UniqueFEIRType &GetType() const {
    return type;
  }

  const FEIRType &GetTypeRef() const {
    ASSERT(type != nullptr, "type is nullptr");
    return *type.get();
  }

  bool IsGlobal() const {
    return isGlobal;
  }

  void SetGlobal(bool arg) {
    isGlobal = arg;
  }

  bool IsDef() const {
    return isDef;
  }

  void SetDef(bool arg) {
    isDef = std::move(arg);
  }

  void SetTrans(UniqueFEIRVarTrans argTrans) {
    trans = std::move(argTrans);
  }

  const UniqueFEIRVarTrans &GetTrans() const {
    return trans;
  }

  MIRSymbol *GenerateGlobalMIRSymbol(MIRBuilder &builder) const {
    MIRSymbol *mirSym = GenerateGlobalMIRSymbolImpl(builder);
    mirSym->GetSrcPosition().SetFileNum(srcFileIdx);
    mirSym->GetSrcPosition().SetLineNum(srcFileLineNum);
    return mirSym;
  }

  MIRSymbol *GenerateLocalMIRSymbol(MIRBuilder &builder) const {
    MIRSymbol *mirSym = GenerateLocalMIRSymbolImpl(builder);
    mirSym->GetSrcPosition().SetFileNum(srcFileIdx);
    mirSym->GetSrcPosition().SetLineNum(srcFileLineNum);
    return mirSym;
  }

  MIRSymbol *GenerateMIRSymbol(MIRBuilder &builder) const {
    return GenerateMIRSymbolImpl(builder);
  }

  std::string GetName(const MIRType &mirType) const {
    return GetNameImpl(mirType);
  }

  std::string GetNameRaw() const {
    return GetNameRawImpl();
  }

  std::unique_ptr<FEIRVar> Clone() const {
    return CloneImpl();
  }

  bool EqualsTo(const std::unique_ptr<FEIRVar> &var) const {
    return EqualsToImpl(var);
  }

  size_t Hash() const {
    return HashImpl();
  }

  void SetAttrs(GenericAttrs &argGenericAttrs) {
    genAttrs = argGenericAttrs;
  }

  void SetSrcLOC(uint32 fileIdx, uint32 lineNum) {
    srcFileIdx = fileIdx;
    srcFileLineNum = lineNum;
  }

  uint32 GetSrcFileIdx() const {
    return srcFileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return srcFileLineNum;
  }

 protected:
  virtual MIRSymbol *GenerateGlobalMIRSymbolImpl(MIRBuilder &builder) const;
  virtual MIRSymbol *GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const;
  virtual MIRSymbol *GenerateMIRSymbolImpl(MIRBuilder &builder) const;
  virtual std::string GetNameImpl(const MIRType &mirType) const = 0;
  virtual std::string GetNameRawImpl() const = 0;
  virtual std::unique_ptr<FEIRVar> CloneImpl() const = 0;
  virtual bool EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const = 0;
  virtual size_t HashImpl() const = 0;

  FEIRVarKind kind : 6;
  bool isGlobal : 1;
  bool isDef : 1;
  UniqueFEIRType type;
  UniqueFEIRVarTrans trans;
  GenericAttrs genAttrs;
  uint32 srcFileIdx = 0;
  uint32 srcFileLineNum = 0;
};

using UniqueFEIRVar = std::unique_ptr<FEIRVar>;
// FEIRUseDefChain key is use, value is def set
using FEIRUseDefChain = std::map<UniqueFEIRVar*, std::set<UniqueFEIRVar*>>;
// FEIRUseDefChain key is def, value is use set
using FEIRDefUseChain = std::map<UniqueFEIRVar*, std::set<UniqueFEIRVar*>>;
}  // namespace maple
#endif  // MPLFE_INCLUDE_FEIR_VAR_H
