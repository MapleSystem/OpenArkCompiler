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
#include "ast_parser.h"
#include "mpl_logging.h"
#include "mir_module.h"
#include "mpl_logging.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "ast_util.h"
#include "ast_input.h"
#include "fe_manager.h"

namespace maple {
bool ASTParser::OpenFile() {
  astFile = std::make_unique<LibAstFile>();
  bool res = astFile->Open(fileName, 0, 0);
  if (!res) {
    return false;
  }
  astUnitDecl = astFile->GetAstUnitDecl();
  return true;
}

bool ASTParser::Verify() {
  return true;
}

ASTBinaryOperatorExpr *ASTParser::AllocBinaryOperatorExpr(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  if (bo.isAssignmentOp() && !bo.isCompoundAssignmentOp()) {
    if (bo.isCompoundAssignmentOp()) {
      auto *expr = ASTDeclsBuilder::ASTExprBuilder<ASTCompoundAssignOperatorExpr>(allocator);
      clang::BinaryOperator::Opcode opcode = clang::BinaryOperator::getOpForCompoundAssignment(bo.getOpcode());
      Opcode mirOpcode = ASTUtil::CvtBinaryOpcode(opcode);
      expr->SetOpForCompoundAssign(mirOpcode);
    } else {
      return ASTDeclsBuilder::ASTExprBuilder<ASTAssignExpr>(allocator);
    }
  }
  if (bo.getOpcode() == clang::BO_Comma) {
    return ASTDeclsBuilder::ASTExprBuilder<ASTBOComma>(allocator);
  }
  // [C++ 5.5] Pointer-to-member operators.
  if (bo.isPtrMemOp()) {
    return ASTDeclsBuilder::ASTExprBuilder<ASTBOPtrMemExpr>(allocator);
  }
  Opcode mirOpcode = ASTUtil::CvtBinaryOpcode(bo.getOpcode());
  CHECK_FATAL(mirOpcode != OP_undef, "Opcode not support!");
  auto *expr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  expr->SetOpcode(mirOpcode);
  return expr;
}

ASTStmt *ASTParser::ProcessFunctionBody(MapleAllocator &allocator, const clang::CompoundStmt &compoundStmt) {
  CHECK_FATAL(false, "NIY");
  return ProcessStmtCompoundStmt(allocator, compoundStmt);
}

ASTStmt *ASTParser::ProcessStmtCompoundStmt(MapleAllocator &allocator, const clang::CompoundStmt &cpdStmt) {
  ASTCompoundStmt *astCompoundStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCompoundStmt>(allocator);
  CHECK_FATAL(astCompoundStmt != nullptr, "astCompoundStmt is nullptr");
  clang::CompoundStmt::const_body_iterator it;
  ASTStmt *childStmt = nullptr;
  for (it = cpdStmt.body_begin(); it != cpdStmt.body_end(); ++it) {
    if (llvm::isa<clang::CompoundStmt>(*it)) {
      auto *tmpCpdStmt = llvm::cast<clang::CompoundStmt>(*it);
      childStmt = ProcessStmtCompoundStmt(allocator, *tmpCpdStmt);
    } else {
      childStmt = ProcessStmt(allocator, **it);
    }
    if (childStmt != nullptr) {
      astCompoundStmt->SetASTStmt(childStmt);
    } else {
      continue;
    }
  }
  return astCompoundStmt;
}

#define STMT_CASE(CLASS)          \
  case clang::Stmt::CLASS##Class: \
  return ProcessStmt##CLASS(allocator, llvm::cast<clang::CLASS>(stmt));

ASTStmt *ASTParser::ProcessStmt(MapleAllocator &allocator, const clang::Stmt &stmt) {
  switch (stmt.getStmtClass()) {
    STMT_CASE(UnaryOperator);
    STMT_CASE(BinaryOperator);
    STMT_CASE(CompoundAssignOperator);
    STMT_CASE(ImplicitCastExpr);
    STMT_CASE(ParenExpr);
    STMT_CASE(IntegerLiteral);
    STMT_CASE(VAArgExpr);
    STMT_CASE(ConditionalOperator);
    STMT_CASE(CharacterLiteral);
    STMT_CASE(StmtExpr);
    STMT_CASE(CallExpr);
    STMT_CASE(ReturnStmt);
    STMT_CASE(CompoundStmt);
    STMT_CASE(IfStmt);
    STMT_CASE(ForStmt);
    STMT_CASE(WhileStmt);
    STMT_CASE(DoStmt);
    STMT_CASE(BreakStmt);
    STMT_CASE(ContinueStmt);
    STMT_CASE(GotoStmt);
    STMT_CASE(SwitchStmt);
    STMT_CASE(CaseStmt);
    STMT_CASE(DefaultStmt);
    STMT_CASE(CStyleCastExpr);
    STMT_CASE(DeclStmt);
    STMT_CASE(NullStmt);
    default:
      return nullptr;
  }
}

ASTStmt *ASTParser::ProcessStmtUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &unaryOp) {
  ASTUnaryOperatorStmt *astUOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTUnaryOperatorStmt>(allocator);
  CHECK_FATAL(astUOStmt != nullptr, "astUOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &unaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOStmt->SetASTExpr(astExpr);
  return astUOStmt;
}

ASTStmt *ASTParser::ProcessStmtBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &binaryOp) {
  ASTBinaryOperatorStmt *astBOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTBinaryOperatorStmt>(allocator);
  CHECK_FATAL(astBOStmt != nullptr, "astBOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &binaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astBOStmt->SetASTExpr(astExpr);
  return astBOStmt;
}

ASTStmt *ASTParser::ProcessStmtCallExpr(MapleAllocator &allocator, const clang::CallExpr &callExpr) {
  ASTCallExprStmt *astCallExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCallExprStmt>(allocator);
  CHECK_FATAL(astCallExprStmt != nullptr, "astCallExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &callExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCallExprStmt->SetASTExpr(astExpr);
  return astCallExprStmt;
}

ASTStmt *ASTParser::ProcessStmtImplicitCastExpr(MapleAllocator &allocator,
                                                const clang::ImplicitCastExpr &implicitCastExpr) {
  ASTImplicitCastExprStmt *astImplicitCastExprStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTImplicitCastExprStmt>(allocator);
  CHECK_FATAL(astImplicitCastExprStmt != nullptr, "astImplicitCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &implicitCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImplicitCastExprStmt->SetASTExpr(astExpr);
  return astImplicitCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtParenExpr(MapleAllocator &allocator, const clang::ParenExpr &parenExpr) {
  ASTParenExprStmt *astParenExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTParenExprStmt>(allocator);
  CHECK_FATAL(astParenExprStmt != nullptr, "astCallExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &parenExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExprStmt->SetASTExpr(astExpr);
  return astParenExprStmt;
}

ASTStmt *ASTParser::ProcessStmtIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &integerLiteral) {
  ASTIntegerLiteralStmt *astIntegerLiteralStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIntegerLiteralStmt>(allocator);
  CHECK_FATAL(astIntegerLiteralStmt != nullptr, "astIntegerLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &integerLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astIntegerLiteralStmt->SetASTExpr(astExpr);
  return astIntegerLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &vAArgExpr) {
  ASTVAArgExprStmt *astVAArgExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTVAArgExprStmt>(allocator);
  CHECK_FATAL(astVAArgExprStmt != nullptr, "astVAArgExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &vAArgExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExprStmt->SetASTExpr(astExpr);
  return astVAArgExprStmt;
}

ASTStmt *ASTParser::ProcessStmtConditionalOperator(MapleAllocator &allocator,
                                                   const clang::ConditionalOperator &conditionalOperator) {
  ASTConditionalOperatorStmt *astConditionalOperatorStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTConditionalOperatorStmt>(allocator);
  CHECK_FATAL(astConditionalOperatorStmt != nullptr, "astConditionalOperatorStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &conditionalOperator);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperatorStmt->SetASTExpr(astExpr);
  return astConditionalOperatorStmt;
}

ASTStmt *ASTParser::ProcessStmtCharacterLiteral(MapleAllocator &allocator,
                                                const clang::CharacterLiteral &characterLiteral) {
  ASTCharacterLiteralStmt *astCharacterLiteralStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTCharacterLiteralStmt>(allocator);
  CHECK_FATAL(astCharacterLiteralStmt != nullptr, "astCharacterLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &characterLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCharacterLiteralStmt->SetASTExpr(astExpr);
  return astCharacterLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &cStyleCastExpr) {
  ASTCStyleCastExprStmt *astCStyleCastExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCStyleCastExprStmt>(allocator);
  CHECK_FATAL(astCStyleCastExprStmt != nullptr, "astCStyleCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cStyleCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCStyleCastExprStmt->SetASTExpr(astExpr);
  return astCStyleCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &stmtExpr) {
  ASTStmtExprStmt *astStmtExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTStmtExprStmt>(allocator);
  CHECK_FATAL(astStmtExprStmt != nullptr, "astStmtExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &stmtExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmtExprStmt->SetASTExpr(astExpr);
  return astStmtExprStmt;
}

ASTStmt *ASTParser::ProcessStmtCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &cpdAssignOp) {
  ASTCompoundAssignOperatorStmt *astCAOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCompoundAssignOperatorStmt>(allocator);
  CHECK_FATAL(astCAOStmt != nullptr, "astCAOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cpdAssignOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCAOStmt->SetASTExpr(astExpr);
  return astCAOStmt;
}

ASTStmt *ASTParser::ProcessStmtReturnStmt(MapleAllocator &allocator, const clang::ReturnStmt &retStmt) {
  ASTReturnStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTReturnStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, retStmt.getRetValue());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtIfStmt(MapleAllocator &allocator, const clang::IfStmt &ifStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, ifStmt.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(astExpr);
  ASTStmt *thenStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(ifStmt.getThen()));
  if (thenStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetThenStmt(thenStmt);
  if (ifStmt.hasElseStorage()) {
    ASTStmt *elseStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(ifStmt.getElse()));
    if (elseStmt == nullptr) {
      return nullptr;
    }
    astStmt->SetElseStmt(elseStmt);
  }
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtForStmt(MapleAllocator &allocator, const clang::ForStmt &forStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTForStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  if (forStmt.getInit() != nullptr) {
    ASTStmt *initStmt = ProcessStmt(allocator, *forStmt.getInit());
    if (initStmt == nullptr) {
      return nullptr;
    }
    astStmt->SetInitStmt(initStmt);
  }
  if (forStmt.getCond() != nullptr) {
    ASTExpr *condExpr = ProcessExpr(allocator, forStmt.getCond());
    if (condExpr == nullptr) {
      return nullptr;
    }
    astStmt->SetCondExpr(condExpr);
  }
  if (forStmt.getInc() != nullptr) {
    ASTExpr *incExpr = ProcessExpr(allocator, forStmt.getInc());
    if (incExpr == nullptr) {
      return nullptr;
    }
    astStmt->SetIncExpr(incExpr);
  }
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(forStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtWhileStmt(MapleAllocator &allocator, const clang::WhileStmt &whileStmt) {
  ASTWhileStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTWhileStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, whileStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(whileStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGotoStmt(MapleAllocator &allocator, const clang::GotoStmt &gotoStmt) {
  ASTGotoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLabelName(gotoStmt.getLabel()->getStmt()->getName());
  return astStmt;
}

bool ASTParser::HasDefault(const clang::Stmt &stmt) {
  if (llvm::isa<const clang::DefaultStmt>(stmt)) {
    return true;
  } else if (llvm::isa<const clang::CompoundStmt>(stmt)) {
    const auto *cpdStmt = llvm::cast<const clang::CompoundStmt>(&stmt);
    clang::CompoundStmt::const_body_iterator it;
    for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
      if (llvm::isa<clang::DefaultStmt>(*it)) {
        return true;
      } else if (llvm::isa<clang::CaseStmt>(*it)) {
        auto *caseStmt = llvm::cast<clang::CaseStmt>(*it);
        if (HasDefault(*caseStmt->getSubStmt())) {
          return true;
        }
      }
    }
  } else if (llvm::isa<const clang::CaseStmt>(stmt)) {
    const auto *caseStmt = llvm::cast<const clang::CaseStmt>(&stmt);
      if (HasDefault(*caseStmt->getSubStmt())) {
        return true;
      }
  }
  return false;
}

ASTStmt *ASTParser::ProcessStmtSwitchStmt(MapleAllocator &allocator, const clang::SwitchStmt &switchStmt) {
  // if switch cond expr has var decl, we need to handle it.
  ASTSwitchStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTSwitchStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTStmt *condStmt = switchStmt.getConditionVariableDeclStmt() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getConditionVariableDeclStmt());
  astStmt->SetCondStmt(condStmt);
  // switch cond expr
  ASTExpr *condExpr = switchStmt.getCond() == nullptr ? nullptr : ProcessExpr(allocator, switchStmt.getCond());
  astStmt->SetCondExpr(condExpr);
  // switch body stmt
  ASTStmt *bodyStmt = switchStmt.getBody() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  astStmt->SetHasDefault(HasDefault(*switchStmt.getBody()));
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDoStmt(MapleAllocator &allocator, const clang::DoStmt &doStmt) {
  ASTDoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, doStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(doStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtBreakStmt(MapleAllocator &allocator, const clang::BreakStmt &breakStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTBreakStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtCaseStmt(MapleAllocator &allocator, const clang::CaseStmt &caseStmt) {
  ASTCaseStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCaseStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLHS(ProcessExpr(allocator, caseStmt.getLHS()));
  astStmt->SetRHS(ProcessExpr(allocator, caseStmt.getRHS()));
  clang::Expr::EvalResult resL;
  (void)caseStmt.getLHS()->EvaluateAsInt(resL, *astFile->GetAstContext());
  astStmt->SetLCaseTag(resL.Val.getInt().getExtValue());
  if (caseStmt.getRHS() != nullptr) {
    clang::Expr::EvalResult resR;
    (void)caseStmt.getLHS()->EvaluateAsInt(resR, *astFile->GetAstContext());
    astStmt->SetRCaseTag(resR.Val.getInt().getExtValue());
  } else {
    astStmt->SetRCaseTag(resL.Val.getInt().getExtValue());
  }
  ASTStmt* subStmt = caseStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *caseStmt.getSubStmt());
  astStmt->SetSubStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDefaultStmt(MapleAllocator &allocator, const clang::DefaultStmt &defaultStmt) {
  ASTDefaultStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDefaultStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  auto *subStmt = defaultStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *defaultStmt.getSubStmt());
  astStmt->SetChildStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtNullStmt(MapleAllocator &allocator, const clang::NullStmt &nullStmt) {
  ASTNullStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTNullStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtContinueStmt(MapleAllocator &allocator, const clang::ContinueStmt &continueStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTContinueStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDeclStmt(MapleAllocator &allocator, const clang::DeclStmt &declStmt) {
  ASTDeclStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  if (declStmt.isSingleDecl()) {
    const clang::Decl *decl = declStmt.getSingleDecl();
    if (decl != nullptr) {
      ASTDecl *ad = ProcessDecl(allocator, *decl);
      if (ad != nullptr) {
        astStmt->SetSubDecl(ad);
      }
    }
  } else {
    // multiple decls
    clang::DeclGroupRef declGroupRef = declStmt.getDeclGroup();
    clang::DeclGroupRef::const_iterator it;
    for (it = declGroupRef.begin(); it != declGroupRef.end(); ++it) {
      ASTDecl *ad = ProcessDecl(allocator, **it);
      if (ad != nullptr) {
        astStmt->SetSubDecl(ad);
      }
    }
  }
  return astStmt;
}

#define EXPR_CASE(CLASS)          \
  case clang::Stmt::CLASS##Class: \
    return ProcessExpr##CLASS(allocator, llvm::cast<clang::CLASS>(*expr));
ASTExpr *ASTParser::ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  switch (expr->getStmtClass()) {
    EXPR_CASE(UnaryOperator);
    EXPR_CASE(NoInitExpr);
    EXPR_CASE(PredefinedExpr);
    EXPR_CASE(OpaqueValueExpr);
    EXPR_CASE(BinaryConditionalOperator);
    EXPR_CASE(CompoundLiteralExpr);
    EXPR_CASE(OffsetOfExpr);
    EXPR_CASE(InitListExpr);
    EXPR_CASE(BinaryOperator);
    EXPR_CASE(ImplicitValueInitExpr);
    EXPR_CASE(ArraySubscriptExpr);
    EXPR_CASE(UnaryExprOrTypeTraitExpr);
    EXPR_CASE(MemberExpr);
    EXPR_CASE(DesignatedInitUpdateExpr);
    EXPR_CASE(ImplicitCastExpr);
    EXPR_CASE(DeclRefExpr);
    EXPR_CASE(ParenExpr);
    EXPR_CASE(IntegerLiteral);
    EXPR_CASE(CharacterLiteral);
    EXPR_CASE(StringLiteral);
    EXPR_CASE(FloatingLiteral);
    EXPR_CASE(ConditionalOperator);
    EXPR_CASE(VAArgExpr);
    EXPR_CASE(GNUNullExpr);
    EXPR_CASE(SizeOfPackExpr);
    EXPR_CASE(UserDefinedLiteral);
    EXPR_CASE(ShuffleVectorExpr);
    EXPR_CASE(TypeTraitExpr);
    EXPR_CASE(ConstantExpr);
    EXPR_CASE(ImaginaryLiteral);
    EXPR_CASE(CallExpr);
    EXPR_CASE(CompoundAssignOperator);
    EXPR_CASE(StmtExpr);
    EXPR_CASE(CStyleCastExpr);
    EXPR_CASE(ArrayInitLoopExpr);
    EXPR_CASE(ArrayInitIndexExpr);
    EXPR_CASE(ExprWithCleanups);
    EXPR_CASE(MaterializeTemporaryExpr);
    EXPR_CASE(SubstNonTypeTemplateParmExpr);
    EXPR_CASE(DependentScopeDeclRefExpr);
    EXPR_CASE(AtomicExpr);
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
}

ASTUnaryOperatorExpr *ASTParser::AllocUnaryOperatorExpr(MapleAllocator &allocator, const clang::UnaryOperator &expr) {
  clang::UnaryOperator::Opcode clangOpCode = expr.getOpcode();
  switch (clangOpCode) {
    case clang::UO_Minus:     // "-"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOMinusExpr>(allocator);
    case clang::UO_Not:       // "~"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUONotExpr>(allocator);
    case clang::UO_LNot:      // "!"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOLNotExpr>(allocator);
    case clang::UO_PostInc:   // "++"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPostIncExpr>(allocator);
    case clang::UO_PostDec:   // "--"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPostDecExpr>(allocator);
    case clang::UO_PreInc:    // "++"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPreIncExpr>(allocator);
    case clang::UO_PreDec:    // "--"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPreDecExpr>(allocator);
    case clang::UO_AddrOf:    // "&"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOAddrOfExpr>(allocator);
    case clang::UO_Deref:     // "*"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUODerefExpr>(allocator);
    case clang::UO_Plus:      // "+"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPlusExpr>(allocator);
    case clang::UO_Real:      // "__real"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUORealExpr>(allocator);
    case clang::UO_Imag:      // "__imag"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOImagExpr>(allocator);
    case clang::UO_Extension: // "__extension__"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOExtensionExpr>(allocator);
    case clang::UO_Coawait:   // "co_await"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOCoawaitExpr>(allocator);
    default:
      CHECK_FATAL(false, "NYI");
  }
}

const clang::Expr *ASTParser::PeelParen(const clang::Expr &expr) {
  const clang::Expr *exprPtr = &expr;
  while (llvm::isa<clang::ParenExpr>(exprPtr) ||
         (llvm::isa<clang::UnaryOperator>(exprPtr) &&
          llvm::cast<clang::UnaryOperator>(exprPtr)->getOpcode() == clang::UO_Extension)) {
    if (llvm::isa<clang::ParenExpr>(exprPtr)) {
      exprPtr = llvm::cast<clang::ParenExpr>(exprPtr)->getSubExpr();
    } else {
      exprPtr = llvm::cast<clang::UnaryOperator>(exprPtr)->getSubExpr();
    }
  }
  return exprPtr;
}

ASTExpr *ASTParser::ProcessExprUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &uo) {
  ASTUnaryOperatorExpr *astUOExpr = AllocUnaryOperatorExpr(allocator, uo);
  CHECK_FATAL(astUOExpr != nullptr, "astUOExpr is nullptr");
  const clang::Expr *subExpr = PeelParen(*uo.getSubExpr());
  clang::UnaryOperator::Opcode clangOpCode = uo.getOpcode();
  MIRType *subType = astFile->CvtType(subExpr->getType());
  astUOExpr->SetSubType(subType);
  MIRType *uoType = astFile->CvtType(uo.getType());
  astUOExpr->SetUOType(uoType);
  if (clangOpCode == clang::UO_PostInc || clangOpCode == clang::UO_PostDec ||
      clangOpCode == clang::UO_PreInc || clangOpCode == clang::UO_PreDec) {
    const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(subExpr);
    const auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(declRefExpr->getDecl()->getCanonicalDecl());
    std::string refName = astFile->GetMangledName(*namedDecl);
    astUOExpr->SetRefName(refName);
    if (declRefExpr->getDecl()->getKind() == clang::Decl::Var) {
      const auto *varDecl = llvm::cast<clang::VarDecl>(declRefExpr->getDecl()->getCanonicalDecl());
      astUOExpr->SetGlobal(!varDecl->isLocalVarDeclOrParm());
    }
    if (subType->GetPrimType() == PTY_ptr) {
      int64 len;
      const clang::QualType qualType = subExpr->getType()->getPointeeType();
      if (astFile->CvtType(qualType)->GetPrimType() == PTY_ptr) {
        // GetAddr32 or GetAddr64 need check
        MIRType *pointeeType = GlobalTables::GetTypeTable().GetAddr64();
        len = static_cast<int64>(pointeeType->GetSize());
      } else {
        const clang::QualType desugaredType = qualType.getDesugaredType(*(astFile->GetContext()));
        len = astFile->GetContext()->getTypeSizeInChars(desugaredType).getQuantity();
      }
      astUOExpr->SetPointeeLen(len);
    }
  }

  ASTExpr *astExpr = ProcessExpr(allocator, subExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOExpr->SetUOExpr(astExpr);
  return astUOExpr;
}

ASTExpr *ASTParser::ProcessExprNoInitExpr(MapleAllocator &allocator, const clang::NoInitExpr &expr) {
  ASTNoInitExpr *astNoInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTNoInitExpr>(allocator);
  CHECK_FATAL(astNoInitExpr != nullptr, "astNoInitExpr is nullptr");
  clang::QualType qualType = expr.getType();
  MIRType *noInitType = astFile->CvtType(qualType);
  astNoInitExpr->SetNoInitType(noInitType);
  return astNoInitExpr;
}

ASTExpr *ASTParser::ProcessExprPredefinedExpr(MapleAllocator &allocator, const clang::PredefinedExpr &expr) {
  ASTPredefinedExpr *astPredefinedExpr = ASTDeclsBuilder::ASTExprBuilder<ASTPredefinedExpr>(allocator);
  CHECK_FATAL(astPredefinedExpr != nullptr, "astPredefinedExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getFunctionName());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astPredefinedExpr->SetASTExpr(astExpr);
  return astPredefinedExpr;
}

ASTExpr *ASTParser::ProcessExprOpaqueValueExpr(MapleAllocator &allocator, const clang::OpaqueValueExpr &expr) {
  ASTOpaqueValueExpr *astOpaqueValueExpr = ASTDeclsBuilder::ASTExprBuilder<ASTOpaqueValueExpr>(allocator);
  CHECK_FATAL(astOpaqueValueExpr != nullptr, "astOpaqueValueExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSourceExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astOpaqueValueExpr->SetASTExpr(astExpr);
  return astOpaqueValueExpr;
}

ASTExpr *ASTParser::ProcessExprBinaryConditionalOperator(MapleAllocator &allocator,
                                                         const clang::BinaryConditionalOperator &expr) {
  ASTBinaryConditionalOperator *astBinaryConditionalOperator =
      ASTDeclsBuilder::ASTExprBuilder<ASTBinaryConditionalOperator>(allocator);
  CHECK_FATAL(astBinaryConditionalOperator != nullptr, "astBinaryConditionalOperator is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, expr.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astBinaryConditionalOperator->SetCondExpr(condExpr);
  ASTExpr *falseExpr = ProcessExpr(allocator, expr.getFalseExpr());
  if (falseExpr == nullptr) {
    return nullptr;
  }
  astBinaryConditionalOperator->SetFalseExpr(falseExpr);
  astBinaryConditionalOperator->SetType(astFile->CvtType(expr.getType()));
  return astBinaryConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundLiteralExpr(MapleAllocator &allocator,
                                                   const clang::CompoundLiteralExpr &expr) {
  ASTCompoundLiteralExpr *astCompoundLiteralExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCompoundLiteralExpr>(allocator);
  CHECK_FATAL(astCompoundLiteralExpr != nullptr, "astCompoundLiteralExpr is nullptr");
  const clang::Expr *initExpr = expr.getInitializer();
  CHECK_FATAL(initExpr != nullptr, "initExpr is nullptr");
  clang::QualType qualType = initExpr->getType();
  astCompoundLiteralExpr->SetCompoundLiteralType(astFile->CvtType(qualType));

  const auto *initListExpr = llvm::dyn_cast<clang::InitListExpr>(initExpr);
  ASTExpr *astExpr = nullptr;
  if (initListExpr != nullptr) {
    astExpr = ProcessExpr(allocator, initListExpr);
  } else {
    astExpr = ProcessExpr(allocator, initExpr);
  }
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCompoundLiteralExpr->SetASTExpr(astExpr);
  return astCompoundLiteralExpr;
}

ASTExpr *ASTParser::ProcessExprInitListExpr(MapleAllocator &allocator, const clang::InitListExpr &expr) {
  ASTInitListExpr *astInitListExpr = ASTDeclsBuilder::ASTExprBuilder<ASTInitListExpr>(allocator);
  CHECK_FATAL(astInitListExpr != nullptr, "ASTInitListExpr is nullptr");
  MIRType *initListType = astFile->CvtType(expr.getType());
  astInitListExpr->SetInitListType(initListType);
  uint32 n = expr.getNumInits();
  clang::Expr * const *le = expr.getInits();
  for (uint32 i = 0; i < n; ++i) {
    const clang::Expr *eExpr = le[i];
    ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
    if (astExpr == nullptr) {
      return nullptr;
    }
    astInitListExpr->SetFillerExprs(astExpr);
  }
  return astInitListExpr;
}

ASTExpr *ASTParser::ProcessExprOffsetOfExpr(MapleAllocator &allocator, const clang::OffsetOfExpr &expr) {\
  ASTOffsetOfExpr *astOffsetOfExpr = ASTDeclsBuilder::ASTExprBuilder<ASTOffsetOfExpr>(allocator);
  CHECK_FATAL(astOffsetOfExpr != nullptr, "astOffsetOfExpr is nullptr");
  clang::FieldDecl *field = expr.getComponent(0).getField();
  const clang::QualType qualType = field->getParent()->getTypeForDecl()->getCanonicalTypeInternal();
  MIRType *structType = astFile->CvtType(qualType);
  astOffsetOfExpr->SetStructType(structType);
  std::string filedName = astFile->GetMangledName(*field);
  astOffsetOfExpr->SetFieldName(filedName);
  const auto *recordType = llvm::cast<clang::RecordType>(qualType);
  clang::RecordDecl *recordDecl = recordType->getDecl();
  const clang::ASTRecordLayout &recordLayout = astFile->GetContext()->getASTRecordLayout(recordDecl);

  clang::RecordDecl::field_iterator it;
  uint64_t filedIdx = 0;
  for (it = recordDecl->field_begin(); it != recordDecl->field_end(); ++it) {
    std::string name = (*it)->getNameAsString();
    if (name == filedName) {
      filedIdx = (*it)->getFieldIndex();
      break;
    }
  }
  CHECK_FATAL(it != recordDecl->field_end(), "cann't find field %s in the struct", filedName.c_str());
  uint64 offsetInBits = recordLayout.getFieldOffset(filedIdx);
  size_t offset = offsetInBits >> kBitToByteShift;
  astOffsetOfExpr->SetOffset(offset);

  return astOffsetOfExpr;
}

ASTExpr *ASTParser::ProcessExprVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &expr) {
  ASTVAArgExpr *astVAArgExpr = ASTDeclsBuilder::ASTExprBuilder<ASTVAArgExpr>(allocator);
  ASSERT(astVAArgExpr != nullptr, "astVAArgExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExpr->SetASTExpr(astExpr);
  return astVAArgExpr;
}

ASTExpr *ASTParser::ProcessExprImplicitValueInitExpr(MapleAllocator &allocator,
                                                     const clang::ImplicitValueInitExpr &expr) {
  auto *astImplicitValueInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTImplicitValueInitExpr>(allocator);
  CHECK_FATAL(astImplicitValueInitExpr != nullptr, "astImplicitValueInitExpr is nullptr");
  astImplicitValueInitExpr->SetType(astFile->CvtType(expr.getType()));
  return astImplicitValueInitExpr;
}

ASTExpr *ASTParser::ProcessExprStringLiteral(MapleAllocator &allocator, const clang::StringLiteral &expr) {
  auto *astStringLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTStringLiteral>(allocator);
  CHECK_FATAL(astStringLiteral != nullptr, "astStringLiteral is nullptr");
  astStringLiteral->SetType(astFile->CvtType(expr.getType()));
  astStringLiteral->SetLength(expr.getLength());
  std::vector<uint32_t> codeUnits;
  for (size_t i = 0; i < expr.getLength(); ++i) {
    codeUnits.emplace_back(expr.getCodeUnit(i));
  }
  codeUnits.emplace_back(0);
  astStringLiteral->SetCodeUnits(codeUnits);
  return astStringLiteral;
}

ASTExpr *ASTParser::ProcessExprArraySubscriptExpr(MapleAllocator &allocator, const clang::ArraySubscriptExpr &expr) {
  auto *astArraySubscriptExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArraySubscriptExpr>(allocator);
  CHECK_FATAL(astArraySubscriptExpr != nullptr, "astArraySubscriptExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astArraySubscriptExpr->SetBaseExpr(baseExpr);
  ASTExpr *idxExpr = ProcessExpr(allocator, expr.getIdx());
  if (idxExpr == nullptr) {
    return nullptr;
  }
  astArraySubscriptExpr->SetIdxExpr(idxExpr);
  return astArraySubscriptExpr;
}

ASTExpr *ASTParser::ProcessExprUnaryExprOrTypeTraitExpr(MapleAllocator &allocator,
                                                        const clang::UnaryExprOrTypeTraitExpr &expr) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  if (expr.isArgumentType()) {
    astExprUnaryExprOrTypeTraitExpr->SetIsType(true);
    astExprUnaryExprOrTypeTraitExpr->SetArgType(astFile->CvtType(expr.getArgumentType()));
  } else {
    ASTExpr *argExpr = ProcessExpr(allocator, expr.getArgumentExpr());
    if (argExpr == nullptr) {
      return nullptr;
    }
    astExprUnaryExprOrTypeTraitExpr->SetArgExpr(argExpr);
  }
  return astExprUnaryExprOrTypeTraitExpr;
}

ASTExpr *ASTParser::ProcessExprMemberExpr(MapleAllocator &allocator, const clang::MemberExpr &expr) {
  auto *astMemberExpr = ASTDeclsBuilder::ASTExprBuilder<ASTMemberExpr>(allocator);
  CHECK_FATAL(astMemberExpr != nullptr, "astMemberExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astMemberExpr->SetBaseExpr(baseExpr);
  astMemberExpr->SetBaseType(astFile->CvtType(expr.getBase()->getType()));
  astMemberExpr->SetMemberName(expr.getMemberDecl()->getNameAsString());
  astMemberExpr->SetMemberType(astFile->CvtType(expr.getMemberDecl()->getType()));
  astMemberExpr->SetIsArrow(expr.isArrow());
  return astMemberExpr;
}

ASTExpr *ASTParser::ProcessExprDesignatedInitUpdateExpr(MapleAllocator &allocator,
                                                        const clang::DesignatedInitUpdateExpr &expr) {
  auto *astDesignatedInitUpdateExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDesignatedInitUpdateExpr>(allocator);
  CHECK_FATAL(astDesignatedInitUpdateExpr != nullptr, "astDesignatedInitUpdateExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astDesignatedInitUpdateExpr->SetBaseExpr(baseExpr);
  clang::InitListExpr *initListExpr = expr.getUpdater();
  MIRType *initListType = astFile->CvtType(expr.getType());
  astDesignatedInitUpdateExpr->SetInitListType(initListType);
  ASTExpr *updaterExpr = ProcessExpr(allocator, initListExpr);
  if (updaterExpr == nullptr) {
    return nullptr;
  }
  astDesignatedInitUpdateExpr->SetUpdaterExpr(updaterExpr);
  return astDesignatedInitUpdateExpr;
}

ASTExpr *ASTParser::ProcessExprStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprConditionalOperator(MapleAllocator &allocator, const clang::ConditionalOperator &expr) {
  ASTConditionalOperator *astConditionalOperator = ASTDeclsBuilder::ASTExprBuilder<ASTConditionalOperator>(allocator);
  ASSERT(astConditionalOperator != nullptr, "astConditionalOperator is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetCondExpr(astExpr);
  ASTExpr *astTrueExpr = ProcessExpr(allocator, expr.getTrueExpr());
  if (astTrueExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetTrueExpr(astTrueExpr);
  ASTExpr *astFalseExpr = ProcessExpr(allocator, expr.getFalseExpr());
  if (astFalseExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetFalseExpr(astFalseExpr);
  astConditionalOperator->SetType(astFile->CvtType(expr.getType()));
  return astConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &expr) {
  auto *astCompoundAssignOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCompoundAssignOperatorExpr>(allocator);
  ASSERT(astCompoundAssignOperatorExpr != nullptr, "astCompoundAssignOperatorExpr is nullptr");
  clang::Expr *lExpr = expr.getLHS();
  if (lExpr != nullptr) {
    ASTExpr *astLExpr = ProcessExpr(allocator, lExpr);
    if (astLExpr != nullptr) {
      astCompoundAssignOperatorExpr->SetLeftExpr(astLExpr);
    } else {
      return nullptr;
    }
  }
  clang::Expr *rExpr = expr.getRHS();
  if (rExpr != nullptr) {
    ASTExpr *astRExpr = ProcessExpr(allocator, rExpr);
    if (astRExpr != nullptr) {
      astCompoundAssignOperatorExpr->SetRightExpr(astRExpr);
    } else {
      return nullptr;
    }
  }
  clang::BinaryOperator::Opcode op = clang::BinaryOperator::getOpForCompoundAssignment(expr.getOpcode());
  astCompoundAssignOperatorExpr->SetOpForCompoundAssign(ASTUtil::CvtBinaryOpcode(op));
  astCompoundAssignOperatorExpr->SetRetType(astFile->CvtType(expr.getComputationResultType()));
  return astCompoundAssignOperatorExpr;
}

ASTExpr *ASTParser::ProcessExprSizeOfPackExpr(MapleAllocator &allocator, const clang::SizeOfPackExpr &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprUserDefinedLiteral(MapleAllocator &allocator, const clang::UserDefinedLiteral &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprTypeTraitExpr(MapleAllocator &allocator, const clang::TypeTraitExpr &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprShuffleVectorExpr(MapleAllocator &allocator, const clang::ShuffleVectorExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprGNUNullExpr(MapleAllocator &allocator, const clang::GNUNullExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprConstantExpr(MapleAllocator &allocator, const clang::ConstantExpr &expr) {
  ASTConstantExpr *astConstantExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConstantExpr>(allocator);
  ASSERT(astConstantExpr != nullptr, "astConstantExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConstantExpr->SetASTExpr(astExpr);
  return astConstantExpr;
}

ASTExpr *ASTParser::ProcessExprImaginaryLiteral(MapleAllocator &allocator, const clang::ImaginaryLiteral &expr) {
  clang::QualType complexQualType = expr.getType().getCanonicalType();
  MIRType *complexType = astFile->CvtType(complexQualType);
  CHECK_NULL_FATAL(complexType);
  clang::QualType elemQualType = llvm::cast<clang::ComplexType>(complexQualType)->getElementType();
  MIRType *elemType = astFile->CvtType(elemQualType);
  CHECK_NULL_FATAL(elemType);
  ASTImaginaryLiteral *astImaginaryLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTImaginaryLiteral>(allocator);
  astImaginaryLiteral->SetComplexType(complexType);
  astImaginaryLiteral->SetElemType(elemType);
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImaginaryLiteral->SetASTExpr(astExpr);
  return astImaginaryLiteral;
}

ASTExpr *ASTParser::ProcessExprCallExpr(MapleAllocator &allocator, const clang::CallExpr &expr) {
  ASTCallExpr *astCallExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCallExpr>(allocator);
  ASSERT(astCallExpr != nullptr, "astCallExpr is nullptr");
  // callee
  ASTExpr *astCallee = ProcessExpr(allocator, expr.getCallee());
  if (astCallee == nullptr) {
    return nullptr;
  }
  astCallExpr->SetCalleeExpr(astCallee);
  // return
  MIRType *retType = astFile->CvtType(expr.getCallReturnType(*astFile->GetAstContext()));
  astCallExpr->SetRetType(retType);
  // args
  std::vector<ASTExpr*> args;
  for (uint32_t i = 0; i < expr.getNumArgs(); ++i) {
    const clang::Expr *subExpr = expr.getArg(i);
    ASTExpr *arg = ProcessExpr(allocator, subExpr);
    args.push_back(arg);
  }
  astCallExpr->SetArgs(args);
  // Obtain the function name directly
  const clang::FunctionDecl *funcDecl = expr.getDirectCallee();
  if (funcDecl != nullptr) {
    std::string funcName = astFile->GetMangledName(*funcDecl);
    if (!ASTUtil::IsValidName(funcName)) {
      ASTUtil::AdjustName(funcName);
    }
    astCallExpr->SetFuncName(funcName);
  } else {
    CHECK_FATAL(false, "funcDecl is nullptr NYI.");
  }
  return astCallExpr;
}

ASTExpr *ASTParser::ProcessExprParenExpr(MapleAllocator &allocator, const clang::ParenExpr &expr) {
  ASTParenExpr *astParenExpr = ASTDeclsBuilder::ASTExprBuilder<ASTParenExpr>(allocator);
  ASSERT(astParenExpr != nullptr, "astParenExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExpr->SetASTExpr(astExpr);
  return astParenExpr;
}

ASTExpr *ASTParser::ProcessExprCharacterLiteral(MapleAllocator &allocator, const clang::CharacterLiteral &expr) {
  ASTCharacterLiteral *astCharacterLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTCharacterLiteral>(allocator);
  ASSERT(astCharacterLiteral != nullptr, "astCharacterLiteral is nullptr");
  const clang::QualType qualType = expr.getType();
  const auto *type = llvm::cast<clang::BuiltinType>(qualType.getTypePtr());
  clang::BuiltinType::Kind kind = type->getKind();
  if (qualType->isPromotableIntegerType()) {
    kind = clang::BuiltinType::Int;
  }
  astCharacterLiteral->SetVal(expr.getValue());
  return astCharacterLiteral;
}

ASTExpr *ASTParser::ProcessExprIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &expr) {
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  ASSERT(astIntegerLiteral != nullptr, "astIntegerLiteral is nullptr");
  uint64 val = 0;
  MIRType *type = nullptr;
  llvm::APInt api = expr.getValue();
  if (api.getBitWidth() > kInt32Width) {
    val = *api.getRawData();
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt64() : GlobalTables::GetTypeTable().GetUInt64();
  } else {
    val = (*api.getRawData() & kInt32Mask);
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt32() : GlobalTables::GetTypeTable().GetUInt32();
  }
  astIntegerLiteral->SetVal(val);
  astIntegerLiteral->SetType(type->GetPrimType());
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ProcessExprFloatingLiteral(MapleAllocator &allocator, const clang::FloatingLiteral &expr) {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  ASSERT(astFloatingLiteral != nullptr, "astFloatingLiteral is nullptr");
  llvm::APFloat apf = expr.getValue();
  const llvm::fltSemantics &fltSem = expr.getSemantics();
  if ((&fltSem != &llvm::APFloat::IEEEsingle()) && (&fltSem != &llvm::APFloat::IEEEdouble())) {
    ASSERT(false, "unsupported floating literal");
  }
  double val;
  if (&fltSem == &llvm::APFloat::IEEEdouble()) {
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetFlag(false);
  } else {
    val = static_cast<double>(apf.convertToFloat());
    astFloatingLiteral->SetFlag(true);
  }
  astFloatingLiteral->SetVal(val);
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ProcessExprImplicitCastExpr(MapleAllocator &allocator, const clang::ImplicitCastExpr &expr) {
  ASTImplicitCastExpr *astImplicitCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTImplicitCastExpr>(allocator);
  CHECK_FATAL(astImplicitCastExpr != nullptr, "astImplicitCastExpr is nullptr");
  switch (expr.getCastKind()) {
    case clang::CK_IntegralRealToComplex:
    case clang::CK_NoOp:
    case clang::CK_ArrayToPointerDecay:
    case clang::CK_FunctionToPointerDecay:
    case clang::CK_FloatingCast:
    case clang::CK_IntegralCast:
    case clang::CK_LValueToRValue: {
      ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
      if (astExpr == nullptr) {
        return nullptr;
      }
      astImplicitCastExpr->SetASTExpr(astExpr);
      break;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return astImplicitCastExpr;
}

ASTExpr *ASTParser::ProcessExprDeclRefExpr(MapleAllocator &allocator, const clang::DeclRefExpr &expr) {
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  CHECK_FATAL(astRefExpr != nullptr, "astRefExpr is nullptr");
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass: {
      const auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(expr.getDecl()->getCanonicalDecl());
      std::string refName = astFile->GetMangledName(*namedDecl);
      clang::QualType qualType = expr.getDecl()->getType();
      MIRType *refType = astFile->CvtType(qualType);
      ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, fileName, refName, std::vector<MIRType*>{refType});
      if (expr.getDecl()->getKind() == clang::Decl::Var) {
        const auto *varDecl = llvm::cast<clang::VarDecl>(expr.getDecl()->getCanonicalDecl());
        astDecl->SetGlobal(!varDecl->isLocalVarDeclOrParm());
      } else {
        // For comes from `EnumConstant`
        astDecl->SetGlobal(expr.getDecl()->getParentFunctionOrMethod() == nullptr);
      }
      astRefExpr->SetASTDecl(astDecl);
      astRefExpr->SetType(refType);
      return astRefExpr;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  clang::QualType qualType = bo.getType();
  // Complex  type
  if (qualType->isAnyComplexType()) {
    CHECK_FATAL(false, "NIY");
  }

  MIRType *retType = astFile->CvtType(qualType);
  if (retType == nullptr) {
    return nullptr;
  }
  astBinOpExpr->SetRetType(retType);
  clang::Expr *lExpr = bo.getLHS();
  if (lExpr != nullptr) {
    ASTExpr *astLExpr = ProcessExpr(allocator, lExpr);
    if (astLExpr != nullptr) {
      astBinOpExpr->SetLeftExpr(astLExpr);
    } else {
      return nullptr;
    }
  }
  clang::Expr *rExpr = bo.getRHS();
  if (rExpr != nullptr) {
    ASTExpr *astRExpr = ProcessExpr(allocator, rExpr);
    if (astRExpr != nullptr) {
      astBinOpExpr->SetRightExpr(astRExpr);
    } else {
      return nullptr;
    }
  }
  return astBinOpExpr;
}

ASTDecl *ASTParser::GetAstDeclOfDeclRefExpr(MapleAllocator &allocator, const clang::Expr &expr) {
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass:
      return static_cast<ASTDeclRefExpr*>(ProcessExpr(allocator, &expr))->GetASTDecl();
    case clang::Stmt::ImplicitCastExprClass:
    case clang::Stmt::CXXStaticCastExprClass:
    case clang::Stmt::CXXReinterpretCastExprClass:
    case clang::Stmt::CStyleCastExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::CastExpr>(expr).getSubExpr());
    case clang::Stmt::ParenExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::ParenExpr>(expr).getSubExpr());
    case clang::Stmt::UnaryOperatorClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::UnaryOperator>(expr).getSubExpr());
    case clang::Stmt::ConstantExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::ConstantExpr>(expr).getSubExpr());
    default:
      break;
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &castExpr) {
  ASTCStyleCastExpr *astCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCStyleCastExpr>(allocator);
  clang::QualType fType = castExpr.getSubExpr()->getType();
  clang::QualType tType = castExpr.getType();
  bool shouldCastArr = false;
  if (fType->isPointerType() && tType->isPointerType()) {
    const clang::Type *fromType = fType->getPointeeType().getTypePtrOrNull();
    const clang::Type *toType = tType->getPointeeType().getTypePtrOrNull();
    bool asFlag = fromType != nullptr && toType != nullptr;
    CHECK_FATAL(asFlag, "ERROR:null pointer!");
    auto *implicit = llvm::dyn_cast<clang::ImplicitCastExpr>(castExpr.getSubExpr());
    if ((fromType->getTypeClass() == clang::Type::ConstantArray && toType->getTypeClass() == clang::Type::Builtin) ||
        (implicit != nullptr && implicit->getCastKind() == clang::CK_ArrayToPointerDecay)) {
      astCastExpr->SetDecl(GetAstDeclOfDeclRefExpr(allocator, *implicit));
      shouldCastArr = true;
    }
  }
  astCastExpr->SetSrcType(astFile->CvtType(fType));
  astCastExpr->SetDestType(astFile->CvtType(tType));
  astCastExpr->SetSubExpr(ProcessExpr(allocator, castExpr.getSubExpr()));
  astCastExpr->SetCanCastArray(shouldCastArr);
  return astCastExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitLoopExpr(MapleAllocator &allocator,
                                                 const clang::ArrayInitLoopExpr &arrInitLoopExpr) {
  ASTArrayInitLoopExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArrayInitLoopExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  ASTExpr *common = arrInitLoopExpr.getCommonExpr() == nullptr ? nullptr :
      ProcessExpr(allocator, arrInitLoopExpr.getCommonExpr());
  astExpr->SetCommonExpr(common);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitIndexExpr(MapleAllocator &allocator,
                                                  const clang::ArrayInitIndexExpr &arrInitIndexExpr) {
  ASTArrayInitIndexExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArrayInitIndexExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetPrimType(astFile->CvtType(arrInitIndexExpr.getType()));
  astExpr->SetValue("0");
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprAtomicExpr(MapleAllocator &allocator,
                                          const clang::AtomicExpr &atomicExpr) {
  ASTAtomicExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAtomicExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetObjExpr(ProcessExpr(allocator, atomicExpr.getPtr()));
  astExpr->SetType(astFile->CvtType(atomicExpr.getPtr()->getType()));
  astExpr->SetRefType(astFile->CvtType(atomicExpr.getPtr()->getType()->getPointeeType()));
  astExpr->SetValExpr1(ProcessExpr(allocator, atomicExpr.getVal1()));
  astExpr->SetValExpr2(ProcessExpr(allocator, atomicExpr.getVal2()));
  astExpr->SetVal1Type(astFile->CvtType(atomicExpr.getVal1()->getType()));
  astExpr->SetVal2Type(astFile->CvtType(atomicExpr.getVal2()->getType()));
  switch (atomicExpr.getOp()) {
    case clang::AtomicExpr::AO__atomic_add_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_add:
    case clang::AtomicExpr::AO__c11_atomic_fetch_add:
      astExpr->SetAtomicOp(kAtomicBinaryOpAdd);
      break;
    case clang::AtomicExpr::AO__atomic_sub_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_sub:
    case clang::AtomicExpr::AO__c11_atomic_fetch_sub:
      astExpr->SetAtomicOp(kAtomicBinaryOpSub);
      break;
    case clang::AtomicExpr::AO__atomic_and_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_and:
    case clang::AtomicExpr::AO__c11_atomic_fetch_and:
      astExpr->SetAtomicOp(kAtomicBinaryOpAnd);
      break;
    case clang::AtomicExpr::AO__atomic_or_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_or:
    case clang::AtomicExpr::AO__c11_atomic_fetch_or:
      astExpr->SetAtomicOp(kAtomicBinaryOpOr);
      break;
    case clang::AtomicExpr::AO__atomic_xor_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_xor:
    case clang::AtomicExpr::AO__c11_atomic_fetch_xor:
      astExpr->SetAtomicOp(kAtomicBinaryOpXor);
      break;
    case clang::AtomicExpr::AO__atomic_load_n:
    case clang::AtomicExpr::AO__c11_atomic_load:
      astExpr->SetAtomicOp(kAtomicOpLoad);
      break;
    case clang::AtomicExpr::AO__c11_atomic_store:
      astExpr->SetAtomicOp(kAtomicOpStore);
      break;
    case clang::AtomicExpr::AO__c11_atomic_exchange:
      astExpr->SetAtomicOp(kAtomicOpExchange);
      break;
    case clang::AtomicExpr::AO__atomic_compare_exchange_n:
    case clang::AtomicExpr::AO__c11_atomic_compare_exchange_weak:
    case clang::AtomicExpr::AO__c11_atomic_compare_exchange_strong:
      astExpr->SetAtomicOp(kAtomicOpCompareExchange);
      break;
    default:
      astExpr->SetAtomicOp(kAtomicOpLast);
      break;
  }
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprExprWithCleanups(MapleAllocator &allocator,
                                                const clang::ExprWithCleanups &cleanupsExpr) {
  ASTExprWithCleanups *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprWithCleanups>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  ASTExpr *sub = cleanupsExpr.getSubExpr() == nullptr ? nullptr : ProcessExpr(allocator, cleanupsExpr.getSubExpr());
  astExpr->SetSubExpr(sub);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprMaterializeTemporaryExpr(MapleAllocator &allocator,
                                                        const clang::MaterializeTemporaryExpr &matTempExpr) {
  // cxx feature
  (void)allocator;
  (void)matTempExpr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprSubstNonTypeTemplateParmExpr(MapleAllocator &allocator,
                                                            const clang::SubstNonTypeTemplateParmExpr &subTempExpr) {
  // cxx feature
  (void)allocator;
  (void)subTempExpr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprDependentScopeDeclRefExpr(MapleAllocator &allocator,
                                                         const clang::DependentScopeDeclRefExpr &depScopeExpr) {
  // cxx feature
  (void)allocator;
  (void)depScopeExpr;
  return nullptr;
}

bool ASTParser::PreProcessAST() {
  TraverseDecl(astUnitDecl, [&](clang::Decl *child) {
    switch (child->getKind()) {
      case clang::Decl::Var: {
        globalVarDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Function: {
        funcDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Record: {
        recordDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Typedef:
        break;
      case clang::Decl::Enum:
        globalEnumDecles.emplace_back(child);
        break;
      default: {
        WARN(kLncWarn, "Unsupported decl kind: %u", child->getKind());
      }
    }
  });
  return true;
}

#define DECL_CASE(CLASS)                                                       \
  case clang::Decl::CLASS:                                                     \
    return ProcessDecl##CLASS##Decl(allocator, llvm::cast<clang::CLASS##Decl>(decl));
ASTDecl *ASTParser::ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl) {
  switch (decl.getKind()) {
    DECL_CASE(Function);
    DECL_CASE(Field);
    DECL_CASE(Record);
    DECL_CASE(Var);
    DECL_CASE(Enum);
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return nullptr;
}

ASTDecl *ASTParser::ProcessDeclRecordDecl(MapleAllocator &allocator, const clang::RecordDecl &recDecl) {
  std::stringstream recName;
  clang::QualType qType = recDecl.getTypeForDecl()->getCanonicalTypeInternal();
  astFile->EmitTypeName(*qType->getAs<clang::RecordType>(), recName);
  MIRType *recType = astFile->CvtType(qType);
  if (recType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(recDecl, attrs, kPublic);
  std::string structName = recName.str();
  if (structName.empty() || !ASTUtil::IsValidName(structName)) {
    uint32 id = qType->getAs<clang::RecordType>()->getDecl()->getLocation().getRawEncoding();
    structName = astFile->GetOrCreateMappedUnnamedName(id);
  }
  ASTStruct *curStructOrUnion =
      ASTDeclsBuilder::ASTStructBuilder(allocator, fileName, structName, std::vector<MIRType*>{recType}, attrs);
  if (recDecl.isUnion()) {
    curStructOrUnion->SetIsUnion();
  }
  const auto *declContext = llvm::dyn_cast<clang::DeclContext>(&recDecl);
  if (declContext == nullptr) {
    return nullptr;
  }
  for (auto *loadDecl : declContext->decls()) {
    if (loadDecl == nullptr) {
      return nullptr;
    }
    auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(loadDecl);
    if (llvm::isa<clang::RecordDecl>(loadDecl)) {
      clang::RecordDecl *subRecordDecl = llvm::cast<clang::RecordDecl>(loadDecl->getCanonicalDecl());
      ASTStruct *sub = static_cast<ASTStruct*>(ProcessDeclRecordDecl(allocator, *subRecordDecl));
      if (sub == nullptr) {
        return nullptr;
      }
      astIn->AddASTStruct(sub);
    }

    if (llvm::isa<clang::FieldDecl>(loadDecl)) {
      ASTField *af = static_cast<ASTField*>(ProcessDeclFieldDecl(allocator, *fieldDecl));
      if (af == nullptr) {
        return nullptr;
      }
      curStructOrUnion->SetField(af);
    }
  }
  return curStructOrUnion;
}

ASTDecl *ASTParser::ProcessDeclFunctionDecl(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl) {
  std::string funcName = astFile->GetMangledName(funcDecl);
  if (funcName.empty()) {
    return nullptr;
  }
  if (!ASTUtil::IsValidName(funcName)) {
    ASTUtil::AdjustName(funcName);
  }
  clang::QualType qualType = funcDecl.getReturnType();
  MIRType *retType = astFile->CvtType(qualType);
  if (retType == nullptr) {
    return nullptr;
  }
  std::vector<MIRType*> typeDescIn;
  std::vector<std::string> parmNamesIn;
  typeDescIn.push_back(retType);
  unsigned int numParam = funcDecl.getNumParams();
  for (uint32_t i = 0; i < numParam; ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    const clang::QualType parmQualType = parmDecl->getType();
    std::string parmName = parmDecl->getNameAsString();
    if (parmName.length() == 0) {
      parmName = FEUtils::GetSequentialName0("arg|", i);
    }
    parmNamesIn.emplace_back(parmName);
    MIRType *paramType = astFile->CvtType(parmQualType);
    if (paramType == nullptr) {
      return nullptr;
    }
    typeDescIn.push_back(paramType);
  }
  GenericAttrs attrs;
  astFile->CollectFuncAttrs(funcDecl, attrs, kPublic);
  ASTFunc *astFunc = ASTDeclsBuilder::ASTFuncBuilder(allocator, fileName, funcName, typeDescIn, attrs, parmNamesIn);
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");
  if (funcDecl.hasBody()) {
    ASTStmt *astCompoundStmt = ProcessStmtCompoundStmt(allocator,
                                                       *llvm::cast<clang::CompoundStmt>(funcDecl.getBody()));
    if (astCompoundStmt != nullptr) {
      astFunc->SetCompoundStmt(astCompoundStmt);
    } else {
      return nullptr;
    }
  }
  return astFunc;
}

ASTDecl *ASTParser::ProcessDeclFieldDecl(MapleAllocator &allocator, const clang::FieldDecl &decl) {
  clang::QualType qualType = decl.getType();
  std::string fieldName = astFile->GetMangledName(decl);
  if (fieldName.empty()) {
    return nullptr;
  }
  MIRType *fieldType = astFile->CvtType(qualType);
  if (fieldType == nullptr) {
    return nullptr;
  }
  if (decl.isBitField()) {
    unsigned bitSize = decl.getBitWidthValue(*(astFile->GetContext()));
    MIRBitFieldType mirBFType(static_cast<uint8>(bitSize), fieldType->GetPrimType());
    auto bfTypeIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&mirBFType);
    fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(bfTypeIdx);
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(decl, attrs, kPublic);
  return ASTDeclsBuilder::ASTFieldBuilder(allocator, fileName, fieldName, std::vector<MIRType*>{fieldType}, attrs);
}

ASTDecl *ASTParser::ProcessDeclVarDecl(MapleAllocator &allocator, const clang::VarDecl &varDecl) {
  std::string varName = astFile->GetMangledName(varDecl);
  if (varName.empty()) {
    return nullptr;
  }
  clang::QualType qualType = varDecl.getType();
  MIRType *varType = astFile->CvtType(qualType);
  if (varType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(varDecl, attrs, kPublic);
  ASTVar *astVar = ASTDeclsBuilder::ASTVarBuilder(allocator, fileName, varName, std::vector<MIRType*>{varType}, attrs);
  if (varDecl.hasInit()) {
    auto initExpr = varDecl.getInit();
    auto astInitExpr = ProcessExpr(allocator, initExpr);
    if (initExpr->getStmtClass() == clang::Stmt::InitListExprClass) {
      static_cast<ASTInitListExpr*>(astInitExpr)->SetInitListVarName(varName);
    }
    astVar->SetInitExpr(astInitExpr);
  }
  return astVar;
}

ASTDecl *ASTParser::ProcessDeclEnumDecl(MapleAllocator &allocator, const clang::EnumDecl &enumDecl) {
  GenericAttrs attrs;
  astFile->CollectAttrs(*clang::dyn_cast<const clang:: NamedDecl>(&enumDecl), attrs, kPublic);
  const std::string &enumName = clang::dyn_cast<const clang:: NamedDecl>(&enumDecl)->getNameAsString();
  ASTLocalEnumDecl *localEnumDecl = ASTDeclsBuilder::ASTLocalEnumDeclBuilder(allocator, fileName, enumName,
      std::vector<MIRType*>{}, attrs);
  TraverseDecl(&enumDecl, [&](clang::Decl *child) {
    CHECK_FATAL(child->getKind() == clang::Decl::EnumConstant, "Unsupported decl kind: %u", child->getKind());
    GenericAttrs attrs0;
    astFile->CollectAttrs(*clang::dyn_cast<clang:: NamedDecl>(child), attrs0, kPublic);
    const std::string &varName = clang::dyn_cast<clang:: NamedDecl>(child)->getNameAsString();
    MIRType *mirType = astFile->CvtType(clang::dyn_cast<clang::ValueDecl>(child)->getType());
    ASTVar *astVar =
        ASTDeclsBuilder::ASTVarBuilder(allocator, fileName, varName, std::vector<MIRType*>{mirType}, attrs0);
    auto constExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    constExpr->SetVal(
        static_cast<uint64>(clang::dyn_cast<clang:: EnumConstantDecl>(child)->getInitVal().getExtValue()));
    constExpr->SetType(mirType->GetPrimType());
    astVar->SetInitExpr(constExpr);
    localEnumDecl->PushConstantVar(astVar);
  });
  return localEnumDecl;
}

bool ASTParser::RetrieveStructs(MapleAllocator &allocator, MapleList<ASTStruct*> &structs) {
  for (auto &decl : recordDecles) {
    clang::RecordDecl *recDecl = llvm::cast<clang::RecordDecl>(decl->getCanonicalDecl());
    if (!recDecl->isCompleteDefinition()) {
      continue;
    }
    ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *recDecl));
    if (curStructOrUnion == nullptr) {
      return false;
    }
    curStructOrUnion->SetGlobal(true);
    structs.emplace_back(curStructOrUnion);
  }
  return true;
}

bool ASTParser::RetrieveFuncs(MapleAllocator &allocator, MapleList<ASTFunc*> &funcs) {
  for (auto &func : funcDecles) {
    clang::FunctionDecl *funcDecl = llvm::cast<clang::FunctionDecl>(func);
    ASTFunc *af = static_cast<ASTFunc*>(ProcessDecl(allocator, *funcDecl));
    if (af == nullptr) {
      return false;
    }
    af->SetGlobal(true);
    funcs.emplace_back(af);
  }
  return true;
}

// seperate MP with astparser
bool ASTParser::RetrieveGlobalVars(MapleAllocator &allocator, MapleList<ASTVar*> &vars) {
  for (auto &decl : globalVarDecles) {
    clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(decl);
    ASTVar *val = static_cast<ASTVar*>(ProcessDecl(allocator, *varDecl));
    if (val == nullptr) {
      return false;
    }
    val->SetGlobal(true);
    vars.emplace_back(val);
  }
  return true;
}

bool ASTParser::ProcessGlobalEnums(MapleAllocator &allocator, MapleList<ASTVar*> &vars) {
  for (auto gEnumDecl : globalEnumDecles) {
    TraverseDecl(gEnumDecl, [&](clang::Decl *child) {
      CHECK_FATAL(child->getKind() == clang::Decl::EnumConstant, "Unsupported decl kind: %u", child->getKind());
      GenericAttrs attrs;
      astFile->CollectAttrs(*clang::dyn_cast<clang:: NamedDecl>(child), attrs, kPublic);
      const std::string &varName = clang::dyn_cast<clang:: NamedDecl>(child)->getNameAsString();
      MIRType *mirType = astFile->CvtType(clang::dyn_cast<clang::ValueDecl>(child)->getType());
      ASTVar *astVar =
          ASTDeclsBuilder::ASTVarBuilder(allocator, fileName, varName, std::vector<MIRType*>{mirType}, attrs);
      auto constExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      constExpr->SetVal(
          static_cast<uint64>(clang::dyn_cast<clang:: EnumConstantDecl>(child)->getInitVal().getExtValue()));
      constExpr->SetType(mirType->GetPrimType());
      astVar->SetInitExpr(constExpr);
      astVar->SetGlobal(true);
      vars.emplace_back(astVar);
    });
  }
  return true;
}

const std::string &ASTParser::GetSourceFileName() const {
  return fileName;
}

const uint32 ASTParser::GetFileIdx() const {
  return fileIdx;
}

void ASTParser::TraverseDecl(const clang::Decl *decl, std::function<void (clang::Decl*)> const &functor) {
  if (decl == nullptr) {
    return;
  }
  for (auto *child : clang::dyn_cast<const clang::DeclContext>(decl)->decls()) {
    SetSourceFileInfo(child);
    functor(child);
  }
}

void ASTParser::SetSourceFileInfo(clang::Decl *decl) {
  clang::FullSourceLoc fullLocation = astFile->GetAstContext()->getFullLoc(decl->getBeginLoc());
  if (fullLocation.isValid() && fullLocation.isFileID()) {
    const clang::FileEntry *fileEntry = fullLocation.getFileEntry();
    ASSERT_NOT_NULL(fileEntry);
    size_t oldSize = GlobalTables::GetStrTable().StringTableSize();
    GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileEntry->getName().data());
    // Not duplicate
    if (oldSize != GlobalTables::GetStrTable().StringTableSize()) {
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(idx, srcFileNum++));
    }
  }
}
}  // namespace maple