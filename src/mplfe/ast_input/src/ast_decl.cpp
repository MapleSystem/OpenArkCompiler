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
#include "ast_decl.h"
#include "ast_parser.h"
#include "global_tables.h"
#include "ast_stmt.h"
#include "feir_var_name.h"
#include "feir_builder.h"

namespace maple {
// ---------- ASTDecl ---------
const std::string &ASTDecl::GetSrcFileName() const {
  return srcFileName;
}

const std::string &ASTDecl::GetName() const {
  return name;
}

const std::vector<MIRType*> &ASTDecl::GetTypeDesc() const {
  return typeDesc;
}

// ---------- ASTVar ----------
std::unique_ptr<FEIRVar> ASTVar::Translate2FEIRVar() {
  CHECK_FATAL(typeDesc.size() == 1, "Invalid ASTVar");
  return std::make_unique<FEIRVarName>(name, std::make_unique<FEIRTypeNative>(*(typeDesc[0])));
}

void ASTVar::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  if (GetInitExpr() != nullptr) {
    UniqueFEIRVar feirVar = Translate2FEIRVar();
    UniqueFEIRExpr expr = GetInitExpr()->Emit2FEExpr(stmts);
    if (expr != nullptr) { // InitListExpr array not emit here
      UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(feirVar), std::move(expr));
      stmts.emplace_back(std::move(stmt));
    }
  }
}

// ---------- ASTLocalEnumDecl ----------
void ASTLocalEnumDecl::GenerateInitStmtImpl(std::list<UniqueFEIRStmt> &stmts) {
  for (auto var : vars) {
    var->GenerateInitStmt(stmts);
  }
}

// ---------- ASTFunc ---------
void ASTFunc::SetCompoundStmt(ASTStmt *astCompoundStmt) {
  compound = astCompoundStmt;
}

const ASTStmt *ASTFunc::GetCompoundStmt() const {
  return compound;
}

std::vector<std::unique_ptr<FEIRVar>> ASTFunc::GenArgVarList() const {
  std::vector<std::unique_ptr<FEIRVar>> args;
  return args;
}

std::list<UniqueFEIRStmt> ASTFunc::EmitASTStmtToFEIR() const {
  std::list<UniqueFEIRStmt> stmts;
  const ASTStmt *astStmt = GetCompoundStmt();
  if (astStmt == nullptr) {
    return stmts;
  }
  const ASTCompoundStmt *astCpdStmt = static_cast<const ASTCompoundStmt*>(astStmt);
  const std::list<ASTStmt*> &astStmtList = astCpdStmt->GetASTStmtList();
  for (auto stmtNode : astStmtList) {
    std::list<UniqueFEIRStmt> childStmts = stmtNode->Emit2FEStmt();
    for (auto &stmt : childStmts) {
      // Link jump stmt not implemented yet
      stmts.emplace_back(std::move(stmt));
    }
  }
  return stmts;
}
// ---------- ASTStruct ----------
std::string ASTStruct::GetStructName(bool mapled) const {
  return mapled ? namemangler::EncodeName(name) : name;
}
}  // namespace maple