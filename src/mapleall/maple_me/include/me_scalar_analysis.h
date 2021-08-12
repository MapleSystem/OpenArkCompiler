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
#ifndef MAPLE_ME_INCLUDE_SCALAR_ANALYSIS_H
#define MAPLE_ME_INCLUDE_SCALAR_ANALYSIS_H
#include "me_function.h"
#include "me_irmap.h"
#include "me_loop_analysis.h"
#include "me_ir.h"

namespace maple {
class LoopScalarAnalysisResult;

enum CRNodeType {
  kCRConstNode,
  kCRVarNode,
  kCRAddNode,
  kCRMulNode,
  kCRDivNode,
  kCRNode,
  kCRUnKnown
};

class CRNode {
 public:
  CRNode(MeExpr *e, CRNodeType t) : expr(e), crType(t) {}
  virtual ~CRNode() = default;

  CRNodeType GetCRType() const {
    return crType;
  }

  MeExpr *GetExpr() {
    return expr;
  }

  const MeExpr *GetExpr() const {
    return expr;
  }

 private:
  MeExpr *expr;
  CRNodeType crType;
};

class CRUnKnownNode : public CRNode {
 public:
  explicit CRUnKnownNode(MeExpr *e) : CRNode(e, kCRUnKnown) {}
  ~CRUnKnownNode() = default;
};

class CRConstNode : public CRNode {
 public:
  CRConstNode(MeExpr *e, int v) : CRNode(e, kCRConstNode), value(v) {}
  ~CRConstNode() = default;

  int32 GetConstValue() const {
    return value;
  }

 private:
  int32 value;
};

class CRVarNode : public CRNode {
 public:
  explicit CRVarNode(MeExpr *e) : CRNode(e, kCRVarNode) {}
  ~CRVarNode() = default;
};

class CRAddNode : public CRNode {
 public:
  explicit CRAddNode(MeExpr *e) : CRNode(e, kCRAddNode) {}
  ~CRAddNode() = default;

  void SetOpnds(std::vector<CRNode*> &o) {
    opnds = o;
  }

  CRNode *GetOpnd(size_t i) const {
    return opnds.at(i);
  }

  std::vector<CRNode*> &GetOpnds() {
    return opnds;
  }

  const std::vector<CRNode*> &GetOpnds() const {
    return opnds;
  }

  size_t GetOpndsSize() const {
    return opnds.size();
  }

 private:
  std::vector<CRNode*> opnds;
};

class CRMulNode : public CRNode {
 public:
  explicit CRMulNode(MeExpr *e) : CRNode(e, kCRMulNode) {}
  ~CRMulNode() = default;

  const CRNode *GetOpnd(size_t i) const {
    return opnds.at(i);
  }

  CRNode *GetOpnd(size_t i) {
    return opnds.at(i);
  }

  size_t GetOpndsSize() const {
    return opnds.size();
  }

  void SetOpnds(std::vector<CRNode*> &o) {
    opnds = o;
  }

  std::vector<CRNode*> &GetOpnds() {
    return opnds;
  }

 private:
  std::vector<CRNode*> opnds;
};

class CRDivNode : public CRNode {
 public:
  CRDivNode(MeExpr *e, CRNode &l, CRNode &r) : CRNode(e, kCRDivNode), lhs(&l), rhs(&r) {}
  ~CRDivNode() = default;

  const CRNode *GetLHS() const {
    return lhs;
  }

  CRNode *GetLHS() {
    return lhs;
  }

  const CRNode *GetRHS() const {
    return rhs;
  }

  CRNode *GetRHS() {
    return rhs;
  }

 private:
  CRNode *lhs;
  CRNode *rhs;
};

class CR : public CRNode {
 public:
  explicit CR(MeExpr *e) : CRNode(e, kCRNode) {}
  ~CR() = default;

  void SetOpnds(std::vector<CRNode*> &o) {
    opnds = o;
  }

  const CRNode *GetOpnd(size_t i) const {
    return opnds.at(i);
  }

  CRNode *GetOpnd(size_t i) {
    return opnds.at(i);
  }

  void PushOpnds(CRNode &crNode) {
    opnds.push_back(&crNode);
  }

  void InsertOpnds(const std::vector<CRNode*>::iterator begin, const std::vector<CRNode*>::iterator end) {
    (void)opnds.insert(opnds.end(), begin, end);
  }

  size_t GetOpndsSize() const {
    return opnds.size();
  }

  const std::vector<CRNode*> &GetOpnds() const {
    return opnds;
  }

  std::vector<CRNode*> &GetOpnds() {
    return opnds;
  }

  CRNode *GetPolynomialsValueAtITerm(CRNode &iterCRNode, size_t num, LoopScalarAnalysisResult &scalarAnalysis) const;
  CRNode *ComputeValueAtIteration(uint32 i, LoopScalarAnalysisResult &scalarAnalysis) const;

 private:
  std::vector<CRNode*> opnds;
};

struct CompareCRNode {
  bool operator()(const CRNode *a, const CRNode *b) const {
    return a->GetCRType() < b->GetCRType();
  }
};

enum TripCountType {
  kConstCR,
  kVarCR,
  kVarCondition,
  kCouldNotUnroll,
  kCouldNotComputeCR
};

class LoopScalarAnalysisResult {
 public:
  static bool enableDebug;
  LoopScalarAnalysisResult(MeIRMap &map, LoopDesc &l) : irMap(&map), loop(&l) {}
  ~LoopScalarAnalysisResult() = default;

  bool IsAnalysised(MeExpr &expr) const {
    return expr2CR.find(&expr) != expr2CR.end();
  }

  CRNode *GetExpr2CR(MeExpr &expr) {
    CHECK_FATAL(expr2CR.find(&expr) != expr2CR.end(), "computedCR must exit");
    return expr2CR[&expr];
  }

  void InsertExpr2CR(MeExpr &expr, CRNode *cr) {
    if ((expr2CR.find(&expr) != expr2CR.end()) && (expr2CR[&expr]->GetCRType() != kCRUnKnown)) {
      CHECK_FATAL(false, "computedCR must not exit or kCRUnKnown");
    }
    expr2CR[&expr] = cr;
  }

  void Dump(const CRNode &crNode);
  void VerifyCR(const CRNode &crNode);
  bool HasUnknownCRNode(CRNode &crNode, CRNode *&result);
  CR *AddCRWithCR(CR &lhsCR, CR &rhsCR);
  CR *MulCRWithCR(const CR &lhsCR, const CR &rhsCR) const;
  MeExpr *TryToResolveVar(MeExpr &expr, std::set<MePhiNode*> &visitedPhi, MeExpr &dummyExpr);
  CRNode *GetCRAddNode(MeExpr *expr, std::vector<CRNode*> &crAddOpnds);
  CRNode *GetCRMulNode(MeExpr *expr, std::vector<CRNode*> &crMulOpnds);
  CRNode *GetOrCreateCR(MeExpr &expr, CRNode &start, CRNode &stride);
  CRNode *GetOrCreateCR(MeExpr *expr, std::vector<CRNode*> &crNodes);
  CRNode *GetOrCreateLoopInvariantCR(MeExpr &expr);
  CRNode *ChangeNegative2MulCRNode(CRNode &crNode);
  CRNode *GetOrCreateCRConstNode(MeExpr *expr, int32 value);
  CRNode *GetOrCreateCRVarNode(MeExpr &expr);
  CRNode *GetOrCreateCRAddNode(MeExpr *expr, std::vector<CRNode*> &crAddNodes);
  CRNode *GetOrCreateCRMulNode(MeExpr *expr, std::vector<CRNode*> &crMulNodes);
  CRNode *GetOrCreateCRDivNode(MeExpr *expr, CRNode &lhsCRNode, CRNode &rhsCRNode);
  CRNode *ComputeCRNodeWithOperator(MeExpr &expr, CRNode &lhsCRNode, CRNode &rhsCRNode, Opcode op);
  CRNode *CreateSimpleCRForPhi(MePhiNode &phiNode, VarMeExpr &startExpr, const VarMeExpr &backEdgeExpr);
  CRNode *CreateCRForPhi(MePhiNode &phiNode);
  CRNode *GetOrCreateCRNode(MeExpr &expr);
  TripCountType ComputeTripCount(MeFunction &func, uint32 &tripCountResult, CRNode *&conditionCRNode, CR *&itCR);

 private:
  MeIRMap *irMap;
  LoopDesc *loop;
  std::map<MeExpr*, CRNode*> expr2CR;
  std::set<std::unique_ptr<CRNode>> allCRNodes;
  void DumpTripCount(const CR &cr, int32 value, const MeExpr *expr);
  uint32 ComputeTripCountWithCR(const CR &cr, const OpMeExpr &opMeExpr, int32 value);
  uint32 ComputeTripCountWithSimpleConstCR(const OpMeExpr &opMeExpr, int32 value, int32 start, int32 stride) const;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SCALAR_ANALYSIS_H