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
#include "me_value_range_prop.h"
#include "me_dominance.h"
#include "me_irmap.h"
namespace maple {
bool ValueRangePropagation::isDebug = false;
constexpr size_t kNumOperands = 2;
void ValueRangePropagation::Execute() {
  ValueRangePropagation::isDebug = false;
  for (auto *bb : dom.GetReversePostOrder()) {
    for (auto &stmt : bb->GetMeStmts()) {
      switch (stmt.GetOp()) {
        case OP_dassign:
        case OP_maydassign: {
          DealWithAssign(*bb, stmt);
          break;
        }
        case OP_brfalse:
        case OP_brtrue: {
          DealWithCondGoto(*bb, stmt);
          break;
        }
        case OP_igoto:
        default:
          break;
      }
    }
  }
}

bool ValueRangePropagation::OverflowOrUnderflow(int64 lhs, int64 rhs) {
  if (lhs == 0 || rhs == 0) {
    return false;
  }
  if (lhs > 0 && rhs <= (kMax - lhs)) {
    return false;
  }
  if (lhs < 0 && rhs >= (kMin - lhs)) {
    return false;
  }
  return true;
}

bool ValueRangePropagation::AddOrSubWithConstant(Opcode op, int64 lhsConstant, int64 rhsConstant, int64 &res) {
  bool overflowOrUnderflow = false;
  if (op == OP_add) {
    overflowOrUnderflow = OverflowOrUnderflow(lhsConstant, rhsConstant);
  } else {
    CHECK_FATAL(op == OP_sub, "must be sub");
    overflowOrUnderflow = OverflowOrUnderflow(lhsConstant, -rhsConstant);
  }
  if (!overflowOrUnderflow) {
    res = (op == OP_add) ? (lhsConstant + rhsConstant) : (lhsConstant - rhsConstant);
    return true;
  }
  return false;
}

bool ValueRangePropagation::CreateNewBoundWhenAddOrSub(Opcode op, Bound bound, int64 rhsConstant, Bound &res) {
  int64 constant = 0;
  if (AddOrSubWithConstant(op, bound.GetConstant(), rhsConstant, constant)) {
    res = Bound(bound.GetVar(), constant);
    return true;
  }
  return false;
}

bool ValueRangePropagation::IsConstant(const BB &bb, MeExpr &expr, int64 &value) {
  if (expr.GetMeOp() == kMeOpConst) {
    value = static_cast<ConstMeExpr&>(expr).GetIntValue();
    return true;
  }
  auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), expr.GetExprID());
  if (valueRange == nullptr) {
    if (expr.GetMeOp() == kMeOpVar && static_cast<VarMeExpr&>(expr).GetDefBy() == kDefByStmt &&
        static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS()->GetMeOp() == kMeOpConst) {
      value = static_cast<ConstMeExpr*>(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetRHS())->GetIntValue();
      Insert2Caches(static_cast<VarMeExpr&>(expr).GetDefStmt()->GetBB()->GetBBId(), expr.GetExprID(),
                    std::make_unique<ValueRange>(Bound(value), kEqual));
      return true;
    }
    return false;
  }
  if (valueRange->IsConstant()) {
    value = valueRange->GetBound().GetConstant();
    return true;
  }
  return false;
}

std::unique_ptr<ValueRange> ValueRangePropagation::AddOrSubWithValueRange(
    Opcode op, ValueRange &valueRange, int64 rhsConstant) {
  if (valueRange.GetRangeType() == kLowerAndUpper) {
    int64 res = 0;
    if (!AddOrSubWithConstant(op, valueRange.GetLower().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound lower = Bound(valueRange.GetLower().GetVar(), res);
    res = 0;
    if (!AddOrSubWithConstant(op, valueRange.GetUpper().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound upper = Bound(valueRange.GetUpper().GetVar(), res);
    return std::make_unique<ValueRange>(lower, upper, kLowerAndUpper);
  } else if (valueRange.GetRangeType() == kEqual) {
    int64 res = 0;
    if (!AddOrSubWithConstant(op, valueRange.GetBound().GetConstant(), rhsConstant, res)) {
      return nullptr;
    }
    Bound bound = Bound(valueRange.GetBound().GetVar(), res);
    return std::make_unique<ValueRange>(bound, valueRange.GetRangeType());
  }
  return nullptr;
}

void ValueRangePropagation::DealWithAddOrSub(const BB &bb, const MeExpr &lhsVar, OpMeExpr &opMeExpr) {
  auto *opnd0 = opMeExpr.GetOpnd(0);
  auto *opnd1 = opMeExpr.GetOpnd(1);
  int64 lhsConstant = 0;
  int64 rhsConstant = 0;
  bool lhsIsConstant = IsConstant(bb, *opnd0, lhsConstant);
  bool rhsIsConstant = IsConstant(bb, *opnd1, rhsConstant);
  std::unique_ptr<ValueRange> newValueRange;
  if (lhsIsConstant && rhsIsConstant) {
    int64 res = 0;
    if (AddOrSubWithConstant(opMeExpr.GetOp(), lhsConstant, rhsConstant, res)) {
      newValueRange = std::make_unique<ValueRange>(Bound(res), kEqual);
    }
  } else if (rhsIsConstant) {
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd0->GetExprID());
    if (valueRange == nullptr) {
      return;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, rhsConstant);
  } else if (lhsIsConstant && opMeExpr.GetOp() == OP_add) {
    auto *valueRange = FindValueRangeInCaches(bb.GetBBId(), opnd1->GetExprID());
    if (valueRange == nullptr) {
      return;
    }
    newValueRange = AddOrSubWithValueRange(opMeExpr.GetOp(), *valueRange, lhsConstant);
  }
  if (newValueRange != nullptr) {
    Insert2Caches(bb.GetBBId(), lhsVar.GetExprID(), std::move(newValueRange));
    Insert2Caches(bb.GetBBId(), opMeExpr.GetExprID(), std::move(newValueRange));
  }
}

void ValueRangePropagation::DealWithArrayLength(const BB &bb, MeExpr &lhs, MeExpr &rhs) {
  if (rhs.GetMeOp() == kMeOpVar) {
    auto *valueRangeOfLength = FindValueRangeInCaches(bb.GetBBId(), rhs.GetExprID());
    if (valueRangeOfLength == nullptr) {
      Insert2Caches(bb.GetBBId(), lhs.GetExprID(), std::make_unique<ValueRange>(Bound(&rhs), kEqual));
      Insert2Caches(bb.GetBBId(), rhs.GetExprID(), std::make_unique<ValueRange>(Bound(&rhs), kEqual));
    } else {
      Insert2Caches(bb.GetBBId(), lhs.GetExprID(), CopyValueRange(*valueRangeOfLength));
    }
  } else {
    CHECK_FATAL(rhs.GetMeOp() == kMeOpConst, "must be constant");
  }
  length2Def[&lhs] = &rhs;
  lengthSet.insert(&lhs);
  lengthSet.insert(&rhs);
}

std::unique_ptr<ValueRange> ValueRangePropagation::CopyValueRange(ValueRange &valueRange) {
  switch (valueRange.GetRangeType()) {
    case kEqual:
      return std::make_unique<ValueRange>(valueRange.GetBound(), kEqual);
    case kLowerAndUpper:
    case kSpecialUpperForLoop:
    case kSpecialLowerForLoop:
      return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetUpper(), kLowerAndUpper);
    case kOnlyHasLowerBound:
      return std::make_unique<ValueRange>(valueRange.GetLower(), valueRange.GetStride(), kOnlyHasLowerBound);
    case kOnlyHasUpperBound:
      return std::make_unique<ValueRange>(valueRange.GetUpper(), valueRange.GetStride(), kOnlyHasUpperBound);
    case kNotEqual:
      CHECK_FATAL(false, "can not be here");
      break;
  }
}

void ValueRangePropagation::DealWithAssign(const BB &bb, const MeStmt &stmt) {
  auto *lhs = stmt.GetLHS();
  auto *rhs = stmt.GetRHS();
  auto *existValueRange = FindValueRangeInCaches(bb.GetBBId(), rhs->GetExprID());
  if (existValueRange != nullptr) {
    Insert2Caches(bb.GetBBId(), lhs->GetExprID(), CopyValueRange(*existValueRange));
    return;
  }
  if (rhs->GetMeOp() == kMeOpOp) {
    auto *opMeExpr = static_cast<OpMeExpr*>(rhs);
    switch (rhs->GetOp()) {
      case OP_add:
      case OP_sub: {
        DealWithAddOrSub(bb, *lhs, *opMeExpr);
        break;
      }
      case OP_gcmallocjarray: {
        DealWithArrayLength(bb, *lhs, *opMeExpr->GetOpnd(0));
        break;
      }
      default:
        break;
    }
  } else if (rhs->GetMeOp() == kMeOpConst) {
    if (FindValueRangeInCaches(bb.GetBBId(), lhs->GetExprID()) != nullptr) {
      return;
    }
    std::unique_ptr<ValueRange> valueRange =
        std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr*>(rhs)->GetIntValue()), kEqual);
    Insert2Caches(bb.GetBBId(), lhs->GetExprID(), std::move(valueRange));
  } else if (rhs->GetMeOp() == kMeOpVar) {
    if (lengthSet.find(rhs) != lengthSet.end()) {
      std::unique_ptr<ValueRange> valueRange = std::make_unique<ValueRange>(Bound(rhs), kEqual);
      Insert2Caches(bb.GetBBId(), lhs->GetExprID(), std::move(valueRange));
      return;
    }
  } else if (rhs->GetMeOp() == kMeOpNary) {
    auto *nary = static_cast<NaryMeExpr*>(rhs);
    if (nary->GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) {
      ASSERT(nary->GetOpnds().size() == 1, "must be");
      if (IsPrimitivePureScalar(nary->GetOpnd(0)->GetPrimType())) {
        return;
      }
      ASSERT(nary->GetOpnd(0)->GetPrimType() == PTY_ref, "must be");
      DealWithArrayLength(bb, *lhs, *nary->GetOpnd(0));
    }
  }
}

bool ValueRangePropagation::CanComputeLoopIndVar(MeExpr &phiLHS, MeExpr &expr, int &constant) {
  auto *curExpr = &expr;
  while (true) {
    if (curExpr->GetMeOp() != kMeOpVar) {
      break;
    }
    VarMeExpr *varMeExpr = static_cast<VarMeExpr *>(curExpr);
    if (varMeExpr->GetDefBy() != kDefByStmt) {
      break;
    }
    MeStmt *defStmt = varMeExpr->GetDefStmt();
    if (defStmt->GetRHS()->GetOp() != OP_add && defStmt->GetRHS()->GetOp() != OP_sub) {
      break;
    }
    OpMeExpr &opMeExpr = static_cast<OpMeExpr &>(*defStmt->GetRHS());
    if (opMeExpr.GetOpnd(1)->GetMeOp() == kMeOpConst) {
      int64 res = 0;
      if (AddOrSubWithConstant(opMeExpr.GetOp(), constant,
                               static_cast<ConstMeExpr *>(opMeExpr.GetOpnd(1))->GetIntValue(), res)) {
        constant = res;
        curExpr = opMeExpr.GetOpnd(0);
        if (curExpr == &phiLHS) {
          return true;
        }
        continue;
      }
    }
    break;
  }
  return kCanNotCompute;
}
std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForMonotonicIncreaseVar(
    const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound) {
  int64 rightConstant = 0;
  if (opMeExpr.GetOp() == OP_lt || opMeExpr.GetOp() == OP_ge ||
      opMeExpr.GetOp() == OP_ne || opMeExpr.GetOp() == OP_eq) {
    rightConstant = -1;
  }
  Bound upperBound;
  int64 constantValue = 0;
  if (lengthSet.find(&opnd1) != lengthSet.end()) {
    upperBound = Bound(&opnd1, rightConstant);
    return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
  } else if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(OP_add, constantValue, rightConstant, res)) {
      upperBound = Bound(res);
    } else {
      return nullptr;
    }
    return std::make_unique<ValueRange>(initBound, upperBound, kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(initBound, Bound(&opnd1, rightConstant), kSpecialUpperForLoop);
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::CreateValueRangeForMonotonicDecreaseVar(
    const BB &bb, OpMeExpr &opMeExpr, MeExpr &opnd1, Bound &initBound) {
  int64 rightConstant = 0;
  if (opMeExpr.GetOp() == OP_le || opMeExpr.GetOp() == OP_gt ||
      opMeExpr.GetOp() == OP_ne || opMeExpr.GetOp() == OP_eq) {
    rightConstant = 1;
  }
  Bound lowerBound;
  int64 constantValue = 0;
  if (lengthSet.find(&opnd1) != lengthSet.end()) {
    lowerBound = Bound(&opnd1, rightConstant);
    return std::make_unique<ValueRange>(lowerBound, initBound, kLowerAndUpper);
  } else if (IsConstant(bb, opnd1, constantValue)) {
    int64 res = 0;
    if (AddOrSubWithConstant(OP_add, constantValue, rightConstant, res)) {
      lowerBound = Bound(res);
    } else {
      return nullptr;
    }
    return std::make_unique<ValueRange>(lowerBound, initBound, kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(Bound(&opnd1, rightConstant), initBound, kSpecialLowerForLoop);
  }
}

Bound ValueRangePropagation::Max(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.GetConstant() > rightBound.GetConstant()) ? leftBound : rightBound;
  } else {
    if (leftBound.GetVar() == nullptr && leftBound.GetConstant() == kMin &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.GetConstant() == kMin &&
        leftBound.GetConstant() < 1 && lengthSet.find(leftBound.GetVar()) != lengthSet.end()) {
      return leftBound;
    }
  }
  return leftBound;
}

Bound ValueRangePropagation::Min(Bound leftBound, Bound rightBound) {
  if (leftBound.GetVar() == rightBound.GetVar()) {
    return (leftBound.GetConstant() < rightBound.GetConstant()) ? leftBound : rightBound;
  } else {
    if (leftBound.GetVar() == nullptr && leftBound.GetConstant() == kMax &&
        rightBound.GetConstant() < 1 && lengthSet.find(rightBound.GetVar()) != lengthSet.end()) {
      return rightBound;
    }
    if (rightBound.GetVar() == nullptr && rightBound.GetConstant() == kMax &&
        leftBound.GetConstant() < 1 && lengthSet.find(leftBound.GetVar()) != lengthSet.end()) {
      return leftBound;
    }
  }
  return leftBound;
}

bool ValueRangePropagation::LowerInRange(const BB &bb, Bound lowerTemp, Bound lower, bool lowerIsZero) {
  if ((lowerIsZero && lowerTemp.GetVar() == nullptr) ||
      (!lowerIsZero && lowerTemp.GetVar() == lower.GetVar())) {
    int64 lowerConstant = lowerIsZero ? 0 : lower.GetConstant();
    return (lowerTemp.GetConstant() >= lowerConstant);
  } else {
    if (lowerTemp.GetVar() != nullptr) {
      auto *lowerVar = lowerTemp.GetVar();
      auto *lowerRangeValue = FindValueRangeInCaches(bb.GetBBId(), lowerVar->GetExprID());
      if (lowerRangeValue != nullptr && lowerRangeValue->IfLowerEqualToUpper() &&
          lowerRangeValue->GetLower().GetVar() != lowerVar) {
        if (lowerTemp.GetConstant() == 0) {
          return LowerInRange(bb, lowerRangeValue->GetLower(), lower, lowerIsZero);
        } else {
          Bound newLower;
          if (CreateNewBoundWhenAddOrSub(OP_add, lowerRangeValue->GetBound(),
                                         lowerTemp.GetConstant(), newLower)) {
            return LowerInRange(bb, newLower, lower, lowerIsZero);
          }
        }
      }
    }
  }
  return false;
}

bool ValueRangePropagation::UpperInRange(const BB &bb, Bound upperTemp, Bound upper) {
  if (upperTemp.GetVar() == upper.GetVar()) {
    return (upperTemp.GetConstant() < upper.GetConstant());
  } else {
    if (upperTemp.GetVar() != nullptr) {
      auto *upperVar = upperTemp.GetVar();
      auto *upperRangeValue = FindValueRangeInCaches(bb.GetBBId(), upperVar->GetExprID());
      if (upperRangeValue == nullptr) {
        auto *currVar = upperVar;
        while (length2Def.find(currVar) != length2Def.end()) {
          currVar = length2Def[currVar];
          if (currVar == upper.GetVar()) {
            Bound newUpper;
            if (CreateNewBoundWhenAddOrSub(
                OP_add, upper, upperTemp.GetConstant(), newUpper)) {
              return UpperInRange(bb, newUpper, upper);
            } else {
              return false;
            }
          }
        }
      } else if (upperRangeValue->IfLowerEqualToUpper() &&
                 upperRangeValue->GetUpper().GetVar() != upperVar) {
        if (upperTemp.GetConstant() == 0) {
          return UpperInRange(bb, upperRangeValue->GetUpper(), upper);
        } else {
          Bound newUpper;
          if (CreateNewBoundWhenAddOrSub(OP_add, upperRangeValue->GetBound(),
                                         upperTemp.GetConstant(), newUpper)) {
            return UpperInRange(bb, newUpper, upper);
          } else {
            return false;
          }
        }
      } else if (!upperRangeValue->IfLowerEqualToUpper()) {
        if (length2Def.find(upperVar) != length2Def.end() && length2Def[upperVar] == upper.GetVar()) {
          return (upperTemp.GetConstant() < upper.GetConstant());
        }
      }
    }
  }
  return false;
}

InRangeType ValueRangePropagation::InRange(const BB &bb, ValueRange &rangeTemp, ValueRange &range, bool lowerIsZero) {
  bool lowerInRange = LowerInRange(bb, rangeTemp.GetLower(), range.GetLower(), lowerIsZero);
  Bound upperBound;
  if (lowerIsZero && !range.IfLowerEqualToUpper()) {
    upperBound = range.GetLower();
  } else {
    upperBound = range.GetUpper();
  }
  bool upperInRange = UpperInRange(bb, rangeTemp.GetUpper(), upperBound);
  if (lowerInRange && upperInRange) {
    return kInRange;
  } else if (lowerInRange) {
    return kLowerInRange;
  } else if (upperInRange) {
    return kUpperInRange;
  } else {
    return kNotInRange;
  }
}

std::unique_ptr<ValueRange> ValueRangePropagation::CombineTwoValueRange(
    ValueRange &leftRange, ValueRange &rightRange, bool merge) {
  if (merge) {
    return std::make_unique<ValueRange>(Min(leftRange.GetLower(), rightRange.GetLower()),
                                        Max(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  } else {
    return std::make_unique<ValueRange>(Max(leftRange.GetLower(), rightRange.GetLower()),
                                        Min(leftRange.GetUpper(), rightRange.GetUpper()), kLowerAndUpper);
  }
}

void ValueRangePropagation::DealWithOPLeOrLt(
    const BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, const CondGotoMeStmt &brMeStmt) {
  const BB *trueBranch = nullptr;
  const BB *falseBranch = nullptr;
  if (brMeStmt.GetOp() == OP_brtrue) {
    trueBranch = bb.GetSucc(1);
    falseBranch = bb.GetSucc(0);
  } else {
    trueBranch = bb.GetSucc(0);
    falseBranch = bb.GetSucc(1);
  }
  MeExpr *opnd0 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(0);
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(), newRightUpper, kLowerAndUpper);
    Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(), kLowerAndUpper);
    Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(), std::move(newFalseBranchRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(), newRightUpper, kLowerAndUpper);
    if (InRange(bb, *leftRange, *newRightRange.get()) == kInRange) { // delete true branch
    } else {
      Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                    CombineTwoValueRange(*leftRange, *newRightRange));
    }
    newRightRange = std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(), kLowerAndUpper);
    if (InRange(bb, *leftRange, *newRightRange) == kInRange) { // delete false branch
    } else {
      Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
                    CombineTwoValueRange(*leftRange, *newRightRange));
    }
  }
}

void ValueRangePropagation::DealWithOPGeOrGt(
    const BB &bb, ValueRange *leftRange, Bound newRightUpper, Bound newRightLower, const CondGotoMeStmt &brMeStmt) {
  const BB *trueBranch = nullptr;
  const BB *falseBranch = nullptr;
  if (brMeStmt.GetOp() == OP_brtrue) {
    trueBranch = bb.GetSucc(1);
    falseBranch = bb.GetSucc(0);
  } else {
    trueBranch = bb.GetSucc(0);
    falseBranch = bb.GetSucc(1);
  }
  MeExpr *opnd0 = static_cast<OpMeExpr*>(brMeStmt.GetOpnd())->GetOpnd(0);
  if (leftRange == nullptr) {
    std::unique_ptr<ValueRange> newTrueBranchRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(), kLowerAndUpper);
    Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(), std::move(newTrueBranchRange));
    std::unique_ptr<ValueRange> newFalseBranchRange =
        std::make_unique<ValueRange>(ValueRange::MinBound(), newRightUpper, kLowerAndUpper);
    Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(), std::move(newFalseBranchRange));
  } else {
    std::unique_ptr<ValueRange> newRightRange =
        std::make_unique<ValueRange>(newRightLower, ValueRange::MaxBound(), kLowerAndUpper);
    if (InRange(bb, *leftRange, *newRightRange) == kInRange) { // delete true branch
    } else {
      Insert2Caches(trueBranch->GetBBId(), opnd0->GetExprID(),
                    CombineTwoValueRange(*leftRange, *newRightRange));
    }
    newRightRange = std::make_unique<ValueRange>(ValueRange::MinBound(), newRightUpper, kLowerAndUpper);
    if (InRange(bb, *leftRange, *newRightRange) == kInRange) { // delete false branch
    } else {
      Insert2Caches(falseBranch->GetBBId(), opnd0->GetExprID(),
                    CombineTwoValueRange(*leftRange, *newRightRange));
    }
  }
}

void ValueRangePropagation::DealWithCondGoto(
    const BB &bb, MeExpr &opMeExpr, ValueRange *leftRange, ValueRange &rightRange, const CondGotoMeStmt &brMeStmt) {
  auto newRightUpper = rightRange.GetUpper();
  auto newRightLower = rightRange.GetLower();
  if ((opMeExpr.GetOp() == OP_lt) ||
      (opMeExpr.GetOp() == OP_ge)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(OP_add, newRightUpper.GetConstant(), -1, constant)) {
      return;
    }
    newRightUpper = Bound(newRightUpper.GetVar(), constant);
  }
  if ((opMeExpr.GetOp() == OP_le) ||
      (opMeExpr.GetOp() == OP_gt)) {
    int64 constant = 0;
    if (!AddOrSubWithConstant(OP_add, newRightLower.GetConstant(), 1, constant)) {
      return;
    }
    newRightLower = Bound(newRightLower.GetVar(), constant);
  }
  if (opMeExpr.GetOp() == OP_lt || opMeExpr.GetOp() == OP_le) {
    DealWithOPLeOrLt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
  } else if (opMeExpr.GetOp() == OP_gt || opMeExpr.GetOp() == OP_ge) {
    DealWithOPGeOrGt(bb, leftRange, newRightUpper, newRightLower, brMeStmt);
  }
}

void ValueRangePropagation::DealWithCondGoto(const BB &bb, MeStmt &stmt) {
  CondGotoMeStmt &brMeStmt = static_cast<CondGotoMeStmt&>(stmt);
  const BB *trueBranch = nullptr;
  const BB *falseBranch = nullptr;
  if (brMeStmt.GetOp() == OP_brtrue) {
    trueBranch = bb.GetSucc(1);
    falseBranch = bb.GetSucc(0);
  } else {
    trueBranch = bb.GetSucc(0);
    falseBranch = bb.GetSucc(1);
  }
  const BB *brTarget = bb.GetSucc(1);
  CHECK_FATAL(brMeStmt.GetOffset() == brTarget->GetBBLabel(), "must be");
  auto *opMeExpr = static_cast<OpMeExpr*>(brMeStmt.GetOpnd());
  if (opMeExpr->GetNumOpnds() != kNumOperands) {
    return;
  }
  MeExpr *opnd0 = opMeExpr->GetOpnd(0);
  MeExpr *opnd1 = opMeExpr->GetOpnd(1);
  ValueRange *rightRange = FindValueRangeInCaches(bb.GetBBId(), opnd1->GetExprID());
  if (rightRange == nullptr) {
    if (opnd1->GetMeOp() == kMeOpConst) {
      std::unique_ptr<ValueRange> valueRangeTemp =
          std::make_unique<ValueRange>(Bound(static_cast<ConstMeExpr*>(opnd1)->GetIntValue()), kEqual);
      rightRange = valueRangeTemp.get();
      Insert2Caches(bb.GetBBId(), opnd1->GetExprID(), std::move(valueRangeTemp));
    } else {
      return;
    }
  }
  ValueRange *leftRange = FindValueRangeInCaches(bb.GetBBId(), opnd0->GetExprID());
  if (leftRange == nullptr && rightRange->GetRangeType() != kEqual) {
    return;
  }
  if (leftRange != nullptr && leftRange->GetRangeType() == kOnlyHasLowerBound &&
      rightRange->GetRangeType() == kOnlyHasUpperBound) { // deal with special case
  }
  DealWithCondGoto(bb, *opMeExpr, leftRange, *rightRange, brMeStmt);
}

AnalysisResult *MeDoValueRangePropagation::Run(MeFunction *func, MeFuncResultMgr *frm, ModuleResultMgr*) {
  ValueRangePropagation::isDebug = false;
  CHECK_FATAL(frm != nullptr, "frm is nullptr");
  auto *dom = static_cast<Dominance*>(frm->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *irMap = static_cast<MeIRMap*>(frm->GetAnalysisResult(MeFuncPhase_IRMAPBUILD, func));
  CHECK_FATAL(irMap != nullptr, "irMap phase has problem");
  frm->InvalidAnalysisResult(MeFuncPhase_MELOOP, func);
  IdentifyLoops *meLoop = static_cast<IdentifyLoops*>(frm->GetAnalysisResult(MeFuncPhase_MELOOP, func));
  if (ValueRangePropagation::isDebug) {
    std::cout << func->GetName() << std::endl;
    func->GetCfg()->DumpToFile("valuerange");
    func->Dump(false);
  }
  ValueRangePropagation valueRangePropagation(*func, *irMap, *dom, meLoop);
  valueRangePropagation.Execute();
  if (ValueRangePropagation::isDebug) {
    std::cout << "***************after value range prop***************" << std::endl;
    func->Dump(false);
  }
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "\n============== After boundary check optimization  =============" << std::endl;
    irMap->Dump();
  }
  return nullptr;
}
}  // namespace maple