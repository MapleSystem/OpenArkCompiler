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
#include "ast_expr.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"
#include "ast_util.h"
#include "ast_input.h"
#include "ast_stmt.h"
#include "ast_parser.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"
#include "fe_manager.h"
#include "mir_module.h"
#include "mpl_logging.h"

namespace maple {
std::unordered_map<std::string, ASTCallExpr::FuncPtrBuiltinFunc> ASTCallExpr::InitBuiltinFuncPtrMap() {
  std::unordered_map<std::string, FuncPtrBuiltinFunc> ans;
  ans["alloca"] = &ASTCallExpr::EmitBuiltinAlloca;
  ans["__builtin_ctz"] = &ASTCallExpr::EmitBuiltinCtz;
  ans["__builtin_clz"] = &ASTCallExpr::EmitBuiltinClz;
  ans["__builtin_alloca"] = &ASTCallExpr::EmitBuiltinAlloca;
  ans["__builtin_expect"] = &ASTCallExpr::EmitBuiltinExpect;
  ans["__builtin_va_start"] = &ASTCallExpr::EmitBuiltinVaStart;
  ans["__builtin_va_end"] = &ASTCallExpr::EmitBuiltinVaEnd;
  ans["__builtin_va_copy"] = &ASTCallExpr::EmitBuiltinVaCopy;
  ans["__builtin_prefetch"] = &ASTCallExpr::EmitBuiltinPrefetch;
  // vector builtinfunc
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)                                \
  ans["__builtin_mpl_"#STR] = &ASTCallExpr::EmitBuiltin##STR;
#include "intrinsic_vector.def"
#undef DEF_MIR_INTRINSIC

  return ans;
}

UniqueFEIRExpr ASTCallExpr::ProcessBuiltinFunc(std::list<UniqueFEIRStmt> &stmts, bool &isFinish) const {
  // process a kind of builtinFunc
  std::string prefix = "__builtin_mpl_vector_load";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    auto argExpr = args[0]->Emit2FEExpr(stmts);
    UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*mirType);
    isFinish = true;
    return FEIRBuilder::CreateExprIRead(std::move(type), argExpr->GetType()->Clone(), std::move(argExpr));
  }
  prefix = "__builtin_mpl_vector_store";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    auto arg1Expr = args[0]->Emit2FEExpr(stmts);
    auto arg2Expr = args[1]->Emit2FEExpr(stmts);
    UniqueFEIRType type = FEIRTypeHelper::CreateTypeNative(*mirType);
    auto stmt = FEIRBuilder::CreateStmtIAssign(arg1Expr->GetType()->Clone(), std::move(arg1Expr), std::move(arg2Expr));
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
    isFinish = true;
    return nullptr;
  }
  // process a single builtinFunc
  auto ptrFunc = builtingFuncPtrMap.find(funcName);
  if (ptrFunc != builtingFuncPtrMap.end()) {
    isFinish = true;
    return EmitBuiltinFunc(stmts);
  }
  isFinish = false;
  prefix = "__builtin";
  if (funcName.compare(0, prefix.size(), prefix) == 0) {
    WARN(kLncWarn, "BuiltinFunc (%s) has not been implemented, line: %d", funcName.c_str(), GetSrcFileLineNum());
  }
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinFunc(std::list<UniqueFEIRStmt> &stmts) const {
  return (this->*(builtingFuncPtrMap[funcName]))(stmts);
}

#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...)                                \
UniqueFEIRExpr ASTCallExpr::EmitBuiltin##STR(std::list<UniqueFEIRStmt> &stmts) const {             \
  auto feType = FEIRTypeHelper::CreateTypeNative(*mirType);                                        \
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;                                                 \
  for (auto arg : args) {                                                                          \
    argOpnds.push_back(arg->Emit2FEExpr(stmts));                                                   \
  }                                                                                                \
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feType), INTRN_##STR, argOpnds);      \
}
#include "intrinsic_vector.def"
#undef DEF_MIR_INTRINSIC

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaStart(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  auto exprArgList = std::make_unique<std::list<UniqueFEIRExpr>>();
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    exprArgList->emplace_back(std::move(expr));
  }
  // addrof va_list instead of dread va_list
  exprArgList->front()->SetAddrof(true);
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_va_start, nullptr /* type */, nullptr /* retVar */, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaEnd(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  ASSERT(args.size() == 1, "va_end expects 1 arguments");
  std::list<UniqueFEIRExpr> exprArgList;
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    // addrof va_list instead of dread va_list
    expr->SetAddrof(true);
    exprArgList.emplace_back(std::move(expr));
  }
  auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinVaCopy(std::list<UniqueFEIRStmt> &stmts) const {
  // args
  auto exprArgList = std::make_unique<std::list<UniqueFEIRExpr>>();
  UniqueFEIRType vaListType;
  for (auto arg : args) {
    UniqueFEIRExpr expr = arg->Emit2FEExpr(stmts);
    // addrof va_list instead of dread va_list
    expr->SetAddrof(true);
    vaListType = expr->GetType()->Clone();
    exprArgList->emplace_back(std::move(expr));
  }
  // Add the size of the va_list structure as the size to memcpy.
  UniqueFEIRExpr sizeExpr = FEIRBuilder::CreateExprConstI32(vaListType->GenerateMIRTypeAuto()->GetSize());
  exprArgList->emplace_back(std::move(sizeExpr));
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmt = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_C_memcpy, nullptr /* type */, nullptr /* retVar */, std::move(exprArgList));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinPrefetch(std::list<UniqueFEIRStmt> &stmts) const {
  // __builtin_prefetch is not supported, only parsing args including stmts
  for (int32 i = 0; i <= args.size() - 1; ++i) {
    (void)args[i]->Emit2FEExpr(stmts);
  }
  return nullptr;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinCtz(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
  if (mirType->GetSize() == 4) {
    // 32 bit
    return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_ctz32, argOpnds);
  }
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_ctz32, argOpnds);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinClz(std::list<UniqueFEIRStmt> &stmts) const {
  auto feTy = std::make_unique<FEIRTypeNative>(*mirType);
  std::vector<std::unique_ptr<FEIRExpr>> argOpnds;
  for (auto arg : args) {
    argOpnds.push_back(arg->Emit2FEExpr(stmts));
  }
  if (mirType->GetSize() == 4) {
    // 32 bit
    return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_clz32, argOpnds);
  }
  return std::make_unique<FEIRExprIntrinsicopForC>(std::move(feTy), INTRN_C_clz64, argOpnds);
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinAlloca(std::list<UniqueFEIRStmt> &stmts) const {
  auto arg = args[0]->Emit2FEExpr(stmts);
  auto alloca = std::make_unique<FEIRExprUnary>(OP_alloca, mirType, std::move(arg));
  return alloca;
}

UniqueFEIRExpr ASTCallExpr::EmitBuiltinExpect(std::list<UniqueFEIRStmt> &stmts) const {
  ASSERT(args.size() == 2, "__builtin_expect requires two arguments");
  auto arg1Expr = args[1]->Emit2FEExpr(stmts);
  std::list<std::unique_ptr<FEIRExpr>> argExprsIn;
  argExprsIn.push_back(std::move(arg1Expr));
  auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(argExprsIn));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return args[0]->Emit2FEExpr(stmts);
}

std::map<std::string, ASTParser::FuncPtrBuiltinFunc> ASTParser::InitBuiltinFuncPtrMap() {
  std::map<std::string, FuncPtrBuiltinFunc> ans;
  ans["__builtin_classify_type"] = &ASTParser::ParseBuiltinClassifyType;
  ans["__builtin_constant_p"] = &ASTParser::ParseBuiltinConstantP;
  ans["__builtin_signbit"] = &ASTParser::ParseBuiltinSignbit;
  ans["__builtin_isinf_sign"] = &ASTParser::ParseBuiltinIsinfsign;
  ans["__builtin_huge_val"] = &ASTParser::ParseBuiltinHugeVal;
  ans["__builtin_inff"] = &ASTParser::ParseBuiltinInff;
  ans["__builtin_nanf"] = &ASTParser::ParseBuiltinNanf;
  ans["__builtin_signbitf"] = &ASTParser::ParseBuiltinSignBitf;
  ans["__builtin_signbitl"] = &ASTParser::ParseBuiltinSignBitl;
  return ans;
}

ASTExpr *ASTParser::ParseBuiltinFunc(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  return (this->*(builtingFuncPtrMap[ss.str()]))(allocator, expr, ss);
}

ASTExpr *ASTParser::ParseBuiltinClassifyType(MapleAllocator &allocator, const clang::CallExpr &expr,
                                             std::stringstream &ss) const {
  (void)ss;
  clang::Expr::EvalResult res;
  bool success = expr.EvaluateAsInt(res, *(astFile->GetContext()));
  CHECK_FATAL(success, "Failed to evaluate __builtin_classify_type");
  llvm::APSInt apVal = res.Val.getInt();
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(apVal.getExtValue()));
  astIntegerLiteral->SetType(PTY_i32);
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinConstantP(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)ss;
  int constP = expr.getArg(0)->isConstantInitializer(*astFile->GetNonConstAstContext(), false) ? 1 : 0;
  // Pointers are not considered constant
  if (expr.getArg(0)->getType()->isPointerType() &&
      !llvm::isa<clang::StringLiteral>(expr.getArg(0)->IgnoreParenCasts())) {
    constP = 0;
  }
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntegerLiteral->SetVal(static_cast<uint64>(constP));
  astIntegerLiteral->SetType(astFile->CvtType(expr.getType())->GetPrimType());
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ParseBuiltinSignbit(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  ss << "__signbit";
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltinIsinfsign(MapleAllocator &allocator, const clang::CallExpr &expr,
                                          std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  if (astFile->CvtType(expr.getArg(0)->getType())->GetPrimType() == PTY_f64) {
    ss << "__isinf";
  } else if (astFile->CvtType(expr.getArg(0)->getType())->GetPrimType() == PTY_f32) {
    ss << "__isinff";
  } else {
    ASSERT(false, "Unsupported type passed to isinf");
  }
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltinHugeVal(MapleAllocator &allocator, const clang::CallExpr &expr,
                                        std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F64);
  astFloatingLiteral->SetVal(std::numeric_limits<double>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinInff(MapleAllocator &allocator, const clang::CallExpr &expr,
                                    std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F32);
  astFloatingLiteral->SetVal(std::numeric_limits<float>::infinity());
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinNanf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                     std::stringstream &ss) const {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetKind(F32);
  astFloatingLiteral->SetVal(nanf(""));
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ParseBuiltinSignBitf(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  ss << "__signbitf";
  return nullptr;
}

ASTExpr *ASTParser::ParseBuiltinSignBitl(MapleAllocator &allocator, const clang::CallExpr &expr,
                                         std::stringstream &ss) const {
  (void)allocator;
  (void)expr;
  ss.clear();
  ss.str(std::string());
  ss << "__signbitl";
  return nullptr;
}
} // namespace maple