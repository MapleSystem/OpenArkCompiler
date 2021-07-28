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
#ifndef MAPLE_ME_INCLUDE_ME_VALUE_RANGE_PROP_H
#define MAPLE_ME_INCLUDE_ME_VALUE_RANGE_PROP_H

#include "me_ir.h"
#include "me_function.h"
#include "me_cfg.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
namespace maple {
int64 GetMinNumber(PrimType primType);
int64 GetMaxNumber(PrimType primType);

class Bound {
 public:
  Bound() = default;
  Bound(MeExpr *argVar, int64 argConstant, PrimType pType) : var(argVar), constant(argConstant), primType(pType) {}
  Bound(MeExpr *argVar, PrimType pType) : var(argVar), constant(0), primType(pType) {}
  Bound(int64 argConstant, PrimType pType) : var(nullptr), constant(argConstant), primType(pType) {}
  ~Bound() = default;

  const MeExpr *GetVar() const {
    return var;
  }

  MeExpr *GetVar() {
    return var;
  }

  int64 GetConstant() const {
    return constant;
  }

  PrimType GetPrimType() const {
    return primType;
  }

  void SetPrimType(PrimType type) {
    primType = type;
  }

 private:
  MeExpr *var;
  int64 constant;
  PrimType primType;
};

enum RangeType {
  kLowerAndUpper,
  kSpecialUpperForLoop,
  kSpecialLowerForLoop,
  kOnlyHasLowerBound,
  kOnlyHasUpperBound,
  kNotEqual,
  kEqual
};

enum StrideType {
  kStrideIsConstant,
  kStrideIsValueRange,
  kCanNotCompute,
};

enum InRangeType {
  kLowerInRange,
  kUpperInRange,
  kInRange,
  kNotInRange,
};

using Lower = Bound;
using Upper = Bound;

struct RangePair {
  RangePair() = default;
  RangePair(Bound l, Bound u) : lower(l), upper(u) {};
  Lower lower;
  Upper upper;
};

class ValueRange {
 public:
  ValueRange(Bound bound, int64 argStride, RangeType type) : stride(argStride), rangeType(type) {
    range.bound = bound;
  }

  ValueRange(Bound lower, Bound upper, RangeType type) : rangeType(type) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  ValueRange(Bound bound, RangeType type) : rangeType(type) {
    range.bound = bound;
  }

  ~ValueRange() = default;

  void SetRangePair(Bound lower, Bound upper) {
    range.pair.lower = lower;
    range.pair.upper = upper;
  }

  const RangePair GetLowerAndupper() const {
    return range.pair;
  }

  void SetLower(Lower lower) {
    range.pair.lower = lower;
  }

  Bound GetLower() const {
    return range.pair.lower;
  }

  void SetUpper(Upper upper) {
    range.pair.upper = upper;
  }

  Bound GetUpper() {
    switch (rangeType) {
      case kLowerAndUpper:
      case kSpecialLowerForLoop:
      case kSpecialUpperForLoop:
        return range.pair.upper;
      case kOnlyHasUpperBound:
      case kEqual:
        return range.bound;
      case kOnlyHasLowerBound:
        return MaxBound(range.bound.GetPrimType());
      case kNotEqual:
        CHECK_FATAL(false, "can not be here");
    }
  }

  Bound GetLower() {
    switch (rangeType) {
      case kLowerAndUpper:
      case kSpecialLowerForLoop:
      case kSpecialUpperForLoop:
        return range.pair.lower;
      case kOnlyHasLowerBound:
      case kEqual:
        return range.bound;
      case kOnlyHasUpperBound:
        return MinBound(range.bound.GetPrimType());
      case kNotEqual:
        CHECK_FATAL(false, "can not be here");
    }
  }

  bool IfLowerEqualToUpper() const {
    return rangeType == kEqual ||
           (rangeType == kLowerAndUpper &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar() &&
            range.pair.lower.GetConstant() == range.pair.upper.GetConstant());
  }

  void SetBound(Bound argBound) {
    range.bound = argBound;
  }

  const Bound GetBound() const {
    return range.bound;
  }

  Bound GetBound() {
    return range.bound;
  }

  int64 GetStride() const {
    return stride;
  }

  RangeType GetRangeType() const {
    return rangeType;
  }

  void SetRangeType(RangeType type) {
    rangeType = type;
  }

  bool IsConstant() {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.lower.GetVar() == range.pair.upper.GetVar() &&
            range.pair.lower.GetConstant() == range.pair.upper.GetConstant());
  }

  bool IsConstantLowerAndUpper() {
    return (rangeType == kEqual && range.bound.GetVar() == nullptr) ||
           (rangeType == kLowerAndUpper && range.pair.lower.GetVar() == nullptr &&
            range.pair.upper.GetVar() == nullptr);
  }

  static Bound MinBound(PrimType pType) {
    return Bound(nullptr, GetMinNumber(pType), pType);
  }
  static Bound MaxBound(PrimType pType) {
    return Bound(nullptr, GetMaxNumber(pType), pType);
  }

 private:
  union {
    RangePair pair;
    Bound bound;
  } range;
  int64 stride = 0;
  RangeType rangeType;
};

class ValueRangePropagation {
 public:
  static bool isDebug;

  ValueRangePropagation(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom,
                        IdentifyLoops *argLoops, MemPool &pool, MapleMap<OStIdx, MapleSet<BBId>*> &candsTem)
      : func(meFunc), irMap(argIRMap), dom(argDom), memPool(pool), mpAllocator(&pool), loops(argLoops),
        caches(meFunc.GetCfg()->GetAllBBs().size()), analysisedArrayChecks(meFunc.GetCfg()->GetAllBBs().size()),
        cands(candsTem) {}
  ~ValueRangePropagation() = default;

  void DumpCahces();
  void Execute();

  bool IsCFGChange() const {
    return isCFGChange;
  }

  bool NeedUpdateSSA() const {
    return needUpdateSSA;
  }

 private:
  bool IsBiggerThanMaxInt64(ValueRange &valueRange) const {
    PrimType lowerPrim = valueRange.GetLower().GetPrimType();
    PrimType upperPrim = valueRange.GetUpper().GetPrimType();

    if (IsUnsignedInteger(lowerPrim) && GetPrimTypeSize(lowerPrim) == 4) { // 32bit
      lowerPrim = PTY_u32;
    }
    if (IsUnsignedInteger(upperPrim) && GetPrimTypeSize(upperPrim) == 4) { // 32bit
      upperPrim = PTY_u32;
    }
    if (IsUnsignedInteger(lowerPrim) && GetPrimTypeSize(lowerPrim) == 8) { // 64bit
      lowerPrim = PTY_u64;
    }
    if (IsUnsignedInteger(upperPrim) && GetPrimTypeSize(upperPrim) == 8) { // 64bit
      upperPrim = PTY_u64;
    }

    if (lowerPrim == PTY_u32) {
      if (static_cast<uint32>(valueRange.GetLower().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    } else if (lowerPrim == PTY_u64) {
      if (static_cast<uint64>(valueRange.GetLower().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    }

    if (upperPrim == PTY_u32) {
      if (static_cast<uint32>(valueRange.GetUpper().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    } else if (upperPrim == PTY_u64) {
      if (static_cast<uint64>(valueRange.GetUpper().GetConstant()) > GetMaxNumber(PTY_i64)) {
        return true;
      }
    }
    return false;
  }

  bool Insert2Caches(BBId bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
    if (IsBiggerThanMaxInt64(*valueRange)) {
      return false;
    }
    caches.at(bbID)[exprID] = std::move(valueRange);
    return true;
  }

  ValueRange *FindValueRangeInCaches(BBId bbID, int32 exprID) const {
    auto it = caches.at(bbID).find(exprID);
    if (it != caches.at(bbID).end()) {
      return it->second.get();
    }
    auto *domBB = dom.GetDom(bbID);
    return (domBB == nullptr || domBB->GetBBId() == 0) ? nullptr : FindValueRangeInCaches(domBB->GetBBId(), exprID);
  }

  void Insert2AnalysisedArrayChecks(BBId bbID, MeExpr &array, MeExpr &index) {
    if (analysisedArrayChecks.at(bbID).empty() ||
        analysisedArrayChecks.at(bbID).find(&array) == analysisedArrayChecks.at(bbID).end()) {
      analysisedArrayChecks.at(bbID)[&array] = std::set<MeExpr*>{ &index };
    } else {
      analysisedArrayChecks.at(bbID)[&array].insert(&index);
    }
  }

  void DealWithPhi(BB &bb, MePhiNode &mePhiNode);
  void DealWithCondGoto(BB &bb, MeStmt &stmt);
  bool OverflowOrUnderflow(PrimType primType, int64 lhs, int64 rhs);
  void DealWithAssign(const BB &bb, const MeStmt &stmt);
  bool IsConstant(const BB &bb, MeExpr &expr, int64 &constant);
  std::unique_ptr<ValueRange> CreateValueRangeForPhi(
      LoopDesc &loop, BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, ScalarMeExpr &lhsOfPhi);
  bool AddOrSubWithConstant(PrimType primType, Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res);
  std::unique_ptr<ValueRange> AddOrSubWithValueRange(Opcode op, ValueRange &valueRange, int64 rhsConstant);
  void DealWithAddOrSub(const BB &bb, const MeExpr &lhsVar, OpMeExpr &opMeExpr);
  bool CanComputeLoopIndVar(MeExpr &phiLHS, MeExpr &expr, int &constant);
  Bound Max(Bound leftBound, Bound rightBound);
  Bound Min(Bound leftBound, Bound rightBound);
  InRangeType InRange(const BB &bb, ValueRange &rangeTemp, ValueRange &range, bool lowerIsZero = false);
  std::unique_ptr<ValueRange> CombineTwoValueRange(ValueRange &leftRange, ValueRange &rightRange, bool merge = false);
  void DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs);
  void DealWithArrayCheck(BB &bb, MeStmt &meStmt, MeExpr &meExpr);
  void DealWithArrayCheck();
  bool IfAnalysisedBefore(BB &bb, MeStmt &stmt, NaryMeExpr &nMeExpr);
  std::unique_ptr<ValueRange> MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode);
  void ReplaceBoundForSpecialLoopRangeValue(LoopDesc &loop, ValueRange &valueRangeOfIndex, bool upperIsSpecial);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicIncreaseVar(
      LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicDecreaseVar(
      LoopDesc &loop, BB &exitBB, BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound);
  void DealWithOPLeOrLt(BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithOPGeOrGt(BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithCondGoto(BB &bb, MeExpr &opMeExpr, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  bool CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res);
  std::unique_ptr<ValueRange> CopyValueRange(ValueRange &valueRange, PrimType primType = PTY_begin);
  bool LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero);
  bool UpperInRange(const BB &bb, Bound upperTemp, Bound upper, bool upperIsArrayLength);
  void InsertCandsForSSAUpdate(OStIdx ostIdx, const BB &bb);
  void AnalysisUnreachableBBOrEdge(BB &bb, BB &unreachableBB, BB &succBB);
  void DealWithOPNeOrEq(Opcode op, BB &bb, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  void CreateValueRangeForNeOrEq(MeExpr &opnd, ValueRange &rightRange, BB &trueBranch);
  void DeleteUnreachableBBs();
  bool BrStmtInRange(BB &bb, ValueRange &leftRange, ValueRange &rightRange, Opcode op) const;
  void ChangeLoop2WontExit(LoopDesc &loop, BB &bb, BB &succBB, BB &unreachableBB);
  int64 GetRealValue(int64 value, PrimType primType) const;
  void UpdateTryAttribute(BB &bb);
  void GetTrueAndFalseBranch(Opcode op, BB &bb, BB *&trueBranch, BB *&falseBranch) const;
  Opcode GetTheOppositeOp(Opcode op) const;
  bool GetValueRangeOfCondGotoOpnd(BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd, ValueRange *&valueRange,
                                   std::unique_ptr<ValueRange> &rightRangePtr);

  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  MemPool &memPool;
  MapleAllocator mpAllocator;
  IdentifyLoops *loops;
  std::vector<std::map<int32, std::unique_ptr<ValueRange>>> caches;
  std::set<MeExpr*> lengthSet;
  std::vector<std::pair<MeStmt*, NaryMeExpr*>> arrayChecks;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedArrayChecks;
  std::map<MeExpr*, MeExpr*> length2Def;
  std::set<BB*> unreachableBBs;
  MapleMap<OStIdx, MapleSet<BBId>*> &cands;
  bool isCFGChange = false;
  bool needUpdateSSA = false;
};

class MeDoValueRangePropagation : public MeFuncPhase {
 public:
  explicit MeDoValueRangePropagation(MePhaseID id) : MeFuncPhase(id) {}
  ~MeDoValueRangePropagation() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *frm, ModuleResultMgr *mrm) override;

  std::string PhaseName() const override {
    return "valueRangePropagation";
  }
};
}  // namespace maple
#endif
