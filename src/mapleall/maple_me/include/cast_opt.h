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
#ifndef MAPLE_ME_INCLUDE_CAST_OPT_H
#define MAPLE_ME_INCLUDE_CAST_OPT_H
#include "mir_nodes.h"
#include "me_ir.h"

namespace maple {
// The order matters
enum CastKind {
  CAST_intTrunc = 0,
  CAST_zext = 1,
  CAST_sext = 2,
  CAST_int2fp = 3,
  CAST_fp2int = 4,
  CAST_fpTrunc = 5,
  CAST_fpExt = 6,
  CAST_retype = 7,
  CAST_unknown = 8
};

struct CastInfo {
  explicit CastInfo(void *expr) : expr(expr) {}
  bool IsInvalid() const {
    return kind == CAST_unknown;
  }
  CastKind kind = CAST_unknown;  // CastInfo is invalid if kind is CAST_unknown
  PrimType srcType = PTY_begin;
  PrimType dstType = PTY_end;
  void *expr = nullptr;  // expr's type must be MeExpr* or BaseNode*
};

class CastOpt {
 public:
  static int IsEliminableCastPair(CastKind firstCastKind, CastKind secondCastKind,
                                  PrimType dstType, PrimType midType2, PrimType midType1, PrimType &srcType);
  static void DoComputeCastInfo(CastInfo &castInfo, bool isMeExpr);
  static bool IsExplicitCastOp(Opcode op);
  static bool IsImplicitCastOp(Opcode op);
  static bool IsCompareOp(Opcode op);
};

class MeCastOpt : public CastOpt {
 public:
  static MeExpr *SimplifyCast(IRMap &irMap, MeExpr *expr);
  static void SimplifyCastForAssign(MeStmt *assignStmt);
  static void ComputeCastInfo(CastInfo &castInfo);
  static MeExpr *CreateMeExprByCastKind(IRMap &irMap, CastKind castKind, PrimType srcType, PrimType dstType, MeExpr *opnd,
                                 TyIdx dstTyIdx = TyIdx(0));
  static MeExpr *SimplifyCastPair(IRMap &irMap, const CastInfo &firstCastInfo, const CastInfo &secondCastInfo);
  static MeExpr *SimplifyCastSingle(IRMap &irMap, const CastInfo &castInfo);
  static MeExpr *TransformCvtU1ToNe(IRMap &irMap, OpMeExpr *cvtExpr);
};

class MapleCastOpt : public CastOpt {
 public:
  static void ComputeCastInfo(CastInfo &castInfo);
  static BaseNode *CreateMapleExprByCastKind(MIRBuilder &mirBuilder, CastKind castKind, PrimType srcType, PrimType dstType, BaseNode *opnd,
                                      TyIdx dstTyIdx = TyIdx(0));
  static BaseNode *SimplifyCast(MIRBuilder &mirBuilder, BaseNode *expr);
  static BaseNode *SimplifyCastPair(MIRBuilder &mirBuidler, const CastInfo &firstCastInfo, const CastInfo &secondCastInfo);
  static BaseNode *SimplifyCastSingle(MIRBuilder &mirBuilder, const CastInfo &castInfo);
  static BaseNode *TransformCvtU1ToNe(MIRBuilder &mirBuilder, TypeCvtNode *cvtExpr);
};
}
#endif
