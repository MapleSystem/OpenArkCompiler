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
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"

namespace maple {
// ---------- ASTExpr ----------
UniqueFEIRExpr ASTExpr::Emit2FEExpr() const {
  return Emit2FEExprImpl();
}

// ---------- ASTRefExpr ---------
UniqueFEIRExpr ASTRefExpr::Emit2FEExprImpl() const {
  PrimType primType = var->GetTypeDesc().front()->GetPrimType();
  // A method is required to determine whether the var is global, need to update
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarName(var->GetName(), primType, true, false);
  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
  return feirRefExpr;
}

// ---------- ASTCallExpr ----------
UniqueFEIRExpr ASTCallExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTImplicitCastExpr ----------
UniqueFEIRExpr ASTImplicitCastExpr::Emit2FEExprImpl() const {
  const ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr feirImplicitCastExpr = childExpr->Emit2FEExpr();
  return feirImplicitCastExpr;
}

// ---------- ASTUnaryOperatorExpr ----------
void ASTUnaryOperatorExpr::SetUOExpr(ASTExpr *astExpr) {
  expr = astExpr;
}

UniqueFEIRExpr ASTUOMinusExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUONotExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOLNotExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOPostIncExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOPostDecExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOPreIncExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOPreDecExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOAddrOfExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUODerefExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOPlusExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUORealExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOImagExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOExtensionExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOCoawaitExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTPredefinedExpr ----------
UniqueFEIRExpr ASTPredefinedExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTPredefinedExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOpaqueValueExpr ----------
UniqueFEIRExpr ASTOpaqueValueExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTOpaqueValueExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTBinaryConditionalOperator ----------
UniqueFEIRExpr ASTBinaryConditionalOperator::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTBinaryConditionalOperator::SetRetType(MIRType *returnType) {
  retType = returnType;
}
void ASTBinaryConditionalOperator::SetCondExpr(ASTExpr *condExpr) {
  cExpr = condExpr;
}
void ASTBinaryConditionalOperator::SetFalseExpr(ASTExpr *falseExpr) {
  fExpr = falseExpr;
}

// ---------- ASTNoInitExpr ----------
UniqueFEIRExpr ASTNoInitExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTNoInitExpr::SetNoInitType(MIRType *type) {
  noInitType = type;
}

// ---------- ASTCompoundLiteralExpr ----------
UniqueFEIRExpr ASTCompoundLiteralExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTCompoundLiteralExpr::SetCompoundLiteralType(MIRType *clType) {
  compoundLiteralType = clType;
}

void ASTCompoundLiteralExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOffsetOfExpr ----------
void ASTOffsetOfExpr::SetStructType(MIRType *stype) {
  structType = stype;
}

void ASTOffsetOfExpr::SetFieldName(std::string fName){
  fieldName = fName;
}

UniqueFEIRExpr ASTOffsetOfExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTInitListExpr ----------
UniqueFEIRExpr ASTInitListExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTInitListExpr::SetFillerExprs(ASTExpr *astExpr) {
  fillers.emplace_back(astExpr);
}

void ASTInitListExpr::SetInitListType(MIRType *type) {
  initListType = type;
}

// ---------- ASTBinaryOperator ----------
std::list<UniqueFEIRStmt> ASTBinaryOperatorStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

UniqueFEIRExpr ASTImplicitValueInitExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTStringLiteral::Emit2FEExprImpl() const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrof>(codeUnits);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr ASTArraySubscriptExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTExprUnaryExprOrTypeTraitExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTMemberExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTDesignatedInitUpdateExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTBOAddExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOMulExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBODivExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBORemExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOSubExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOShlExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOShrExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLTExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOGTExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLEExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOGEExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOEQExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBONEExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOAndExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOXorExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOOrExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLAndExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLOrExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOEqExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOAssign::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOComma::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOPtrMemD::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOPtrMemI::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTParenExpr ----------
UniqueFEIRExpr ASTParenExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTIntegerLiteral ----------
UniqueFEIRExpr ASTIntegerLiteral::Emit2FEExprImpl() const {
  UniqueFEIRExpr constExpr = std::make_unique<FEIRExprConst>(val, type);
  return constExpr;
}

// ---------- ASTFloatingLiteral ----------
UniqueFEIRExpr ASTFloatingLiteral::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCharacterLiteral ----------
UniqueFEIRExpr ASTCharacterLiteral::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTConditionalOperator ----------
UniqueFEIRExpr ASTConditionalOperator::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTConstantExpr ----------
UniqueFEIRExpr ASTConstantExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTImaginaryLiteral ----------
UniqueFEIRExpr ASTImaginaryLiteral::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCompoundAssignOperatorExpr ----------
UniqueFEIRExpr ASTCompoundAssignOperatorExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTVAArgExpr ----------
UniqueFEIRExpr ASTVAArgExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCStyleCastExpr ----------
UniqueFEIRExpr ASTCStyleCastExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTArrayInitLoopExpr ----------
UniqueFEIRExpr ASTArrayInitLoopExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTArrayInitIndexExpr ----------
UniqueFEIRExpr ASTArrayInitIndexExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTExprWithCleanups ----------
UniqueFEIRExpr ASTExprWithCleanups::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTMaterializeTemporaryExpr ----------
UniqueFEIRExpr ASTMaterializeTemporaryExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTSubstNonTypeTemplateParmExpr ----------
UniqueFEIRExpr ASTSubstNonTypeTemplateParmExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTDependentScopeDeclRefExpr ----------
UniqueFEIRExpr ASTDependentScopeDeclRefExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTAtomicExpr ----------
UniqueFEIRExpr ASTAtomicExpr::Emit2FEExprImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
}

