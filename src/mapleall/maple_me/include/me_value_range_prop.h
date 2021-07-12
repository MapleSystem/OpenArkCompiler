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
constexpr int64 kMin = std::numeric_limits<int64_t>::min();
constexpr int64 kMax = std::numeric_limits<int64_t>::max();

class Bound {
 public:
  Bound() = default;
  Bound(MeExpr *argVar, int64 argConstant) : var(argVar), constant(argConstant) {}
  Bound(MeExpr *argVar) : var(argVar), constant(0) {}
  Bound(int64 argConstant) : var(nullptr), constant(argConstant) {}
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

 private:
  MeExpr *var;
  int64 constant;
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
        return MaxBound();
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
        return MinBound();
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

  static Bound MinBound() {
    return Bound(nullptr, std::numeric_limits<int64_t>::min());
  }
  static Bound MaxBound() {
    return Bound(nullptr, std::numeric_limits<int64_t>::max());
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

  ValueRangePropagation(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom, IdentifyLoops *argLoops)
      : func(meFunc), irMap(argIRMap), dom(argDom), loops(argLoops), caches(meFunc.GetCfg()->GetAllBBs().size()),
        analysisedArrayChecks(meFunc.GetCfg()->GetAllBBs().size()){}
  ~ValueRangePropagation() = default;

  void DumpCahces();
  void Execute();

 private:
  void Insert2Caches(BBId bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
    caches.at(bbID)[exprID] = std::move(valueRange);
  }

  void UpdateCaches(BBId &bbID, int32 exprID, std::unique_ptr<ValueRange> valueRange) {
    CHECK_FATAL(caches.at(bbID).find(exprID) != caches.at(bbID).end(), "must found");
    caches.at(bbID)[exprID] = std::move(valueRange);
  }

  ValueRange *FindValueRangeInCaches(BBId bbID, int32 exprID) {
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

  void DealWithPhi(const BB &bb, MePhiNode &mePhiNode);
  void DealWithCondGoto(const BB &bb, MeStmt &stmt);
  bool OverflowOrUnderflow(int64 lhs, int64 rhs);
  void DealWithAssign(const BB &bb, const MeStmt &stmt);
  bool IsConstant(const BB &bb, MeExpr &expr, int64 &constant);
  std::unique_ptr<ValueRange> CreateValueRangeForPhi(
      LoopDesc &loop, const BB &bb, ScalarMeExpr &init, ScalarMeExpr &backedge, ScalarMeExpr &lhsOfPhi);
  bool AddOrSubWithConstant(Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res);
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
  MeExpr *ReplaceArrayExpr(MeExpr &rhs, MeExpr &naryMeExpr, MeStmt *ivarStmt);
  bool CleanABCInStmt(MeStmt &meStmt, NaryMeExpr &naryMeExpr);
  std::unique_ptr<ValueRange> MergeValueRangeOfPhiOperands(const BB &bb, MePhiNode &mePhiNode);
  void ReplaceBoundForSpecialLoopRangeValue(LoopDesc &loop, ValueRange &valueRangeOfIndex, bool upperIsSpecial);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicIncreaseVar(
      const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound);
  std::unique_ptr<ValueRange> CreateValueRangeForMonotonicDecreaseVar(const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1,
                                                      Bound &initBound);
  void DealWithOPLeOrLt(const BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithOPGeOrGt(const BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower,
                        const CondGotoMeStmt &brMeStmt);
  void DealWithCondGoto(const BB &bb, MeExpr &opMeExpr, ValueRange *leftRange, ValueRange &rightRange,
                        const CondGotoMeStmt &brMeStmt);
  bool CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res);
  std::unique_ptr<ValueRange> CopyValueRange(ValueRange &valueRange);
  bool LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero);
  bool UpperInRange(const BB &bb, Bound upperTemp, Bound upper);

  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  IdentifyLoops *loops;
  std::vector<std::map<int32, std::unique_ptr<ValueRange>>> caches;
  std::set<MeExpr*> lengthSet;
  std::vector<std::pair<MeStmt*, NaryMeExpr*>> arrayChecks;
  std::vector<std::map<MeExpr*, std::set<MeExpr*>>> analysisedArrayChecks;
  std::map<NaryMeExpr*, NaryMeExpr*> old2newArrays;
  std::map<MeExpr*, MeExpr*> length2Def;
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