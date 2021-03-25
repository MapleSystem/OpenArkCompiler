/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#include "preme.h"
#include "me_option.h"
#include "me_function.h"
#include "me_verify.h"
#include "mir_symbol_builder.h"

namespace maple {
void Preme::CreateMIRTypeForAddrof(const MIRFunction &func, const BaseNode *baseNode) const {
  if (baseNode == nullptr) {
    return;
  }
  if (baseNode->GetOpCode() == OP_block) {
    const BlockNode *blockNode = static_cast<const BlockNode*>(baseNode);
    for (auto &stmt : blockNode->GetStmtNodes()) {
      CreateMIRTypeForAddrof(func, &stmt);
    }
  } else if (baseNode->GetOpCode() == OP_if) {
    auto *ifStmtNode = static_cast<const IfStmtNode*>(baseNode);
    CreateMIRTypeForAddrof(func, ifStmtNode->GetThenPart());
    CreateMIRTypeForAddrof(func, ifStmtNode->GetElsePart());
  } else if (baseNode->GetOpCode() == OP_addrof) {
    const AddrofNode *addrNode = static_cast<const AddrofNode*>(baseNode);
    const MIRSymbol *mirSymbol = func.GetLocalOrGlobalSymbol(addrNode->GetStIdx());
    MIRPtrType ptrType(mirSymbol->GetTyIdx(), PTY_ptr);
    (void)GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  }
  // Search for nested dread node that may include a symbol index.
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    CreateMIRTypeForAddrof(func, baseNode->Opnd(i));
  }
}

void Preme::ProcessFunc(MIRFunction *func) {
  if (func == nullptr || func->IsEmpty()) {
    return;
  }
  StmtNode *stmt = func->GetBody()->GetFirst();
  while (stmt != nullptr) {
    CreateMIRTypeForAddrof(*func, stmt);
    stmt = stmt->GetNext();
  }
}

void DoPreme::CreateMIRTypeForLowerGlobalDreads() const {
  for (uint32 i = 0; i < MIRSymbolBuilder::Instance().GetSymbolTableSize(); ++i) {
    auto *symbol = MIRSymbolBuilder::Instance().GetSymbolFromStIdx(i);
    if (symbol == nullptr) {
      continue;
    }
    if (!symbol->IsVar()) {
      continue;
    }
    MIRPtrType ptrType(symbol->GetTyIdx(), PTY_ptr);
    if (symbol->IsVolatile()) {
      TypeAttrs attrs;
      attrs.SetAttr(ATTR_volatile);
      ptrType.SetTypeAttrs(attrs);
    }
    (void)GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
  }
}

AnalysisResult *DoPreme::Run(MIRModule *mod, ModuleResultMgr *mrm) {
  OPT_TEMPLATE(Preme);
  if (MeOption::optLevel == 2) {
    CreateMIRTypeForLowerGlobalDreads();
  }
  if (MeOption::meVerify) {
    VerifyGlobalTypeTable();
  }
  return nullptr;
}
}  // namespace maple
