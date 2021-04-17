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
#ifndef MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#define MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
#include "ast_decl.h"
#include "mempool_allocator.h"

namespace maple {
ASTDecl *ASTDeclBuilder(MapleAllocator &allocator, const std::string &srcFile,
    const std::string &nameIn, const std::vector<MIRType*> &typeDescIn) {
  return allocator.GetMemPool()->New<ASTDecl>(srcFile, nameIn, typeDescIn);
}

ASTVar *ASTVarBuilder(MapleAllocator &allocator, const std::string &srcFile,
                      const std::string &varName, const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn) {
  return allocator.GetMemPool()->New<ASTVar>(srcFile, varName, desc, genAttrsIn);
}

ASTFunc *ASTFuncBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
                        const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn,
                        const std::vector<std::string> &parmNamesIn) {
  return allocator.GetMemPool()->New<ASTFunc>(srcFile, nameIn, typeDescIn, genAttrsIn, parmNamesIn);
}

template<typename T>
T *ASTStmtBuilder(MapleAllocator &allocator) {
  return allocator.GetMemPool()->New<T>();
}

template<typename T>
T *ASTExprBuilder(MapleAllocator &allocator) {
  return allocator.GetMemPool()->New<T>();
}

ASTStruct *ASTStructBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &nameIn,
    const std::vector<MIRType*> &typeDescIn, const GenericAttrs &genAttrsIn) {
  return allocator.GetMemPool()->New<ASTStruct>(srcFile, nameIn, typeDescIn, genAttrsIn);
}

ASTField *ASTFieldBuilder(MapleAllocator &allocator, const std::string &srcFile, const std::string &varName,
    const std::vector<MIRType*> &desc, const GenericAttrs &genAttrsIn) {
  return allocator.GetMemPool()->New<ASTField>(srcFile, varName, desc, genAttrsIn);
}
}  // namespace maple
#endif  // MPLFE_AST_INPUT_INCLUDE_AST_DECL_BUILDER_H
