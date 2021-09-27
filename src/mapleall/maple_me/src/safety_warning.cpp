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
#include "safety_warning.h"
#include "global_tables.h"
#include "inline.h"
#include "me_dominance.h"
#include "me_irmap_build.h"
#include "mpl_logging.h"
#include "opcode_info.h"
#include <regex>

namespace maple {
namespace safety {
/*
 * Should keep the same rule as "inline.h"
 * inlining begin: FUNC
 * secound inlining begin: FUNC
 * inlining end: FUNC    -- No inline begin for empty body function
 * secound inlining end: FUNC
 *
 */
static const std::regex kInlineRegex(".*inlining.*: FUNC (\\S+)");
}  // namespace safety

inline static bool IsInlineCommentStmt(const MeStmt &stmt, std::cmatch &match) {
  return stmt.GetOp() == OP_comment &&
         std::regex_match(static_cast<const CommentMeStmt &>(stmt).GetComment().c_str(), match, safety::kInlineRegex);
}

inline static void HandleAssertNonnull(const MeStmt &stmt, const MIRModule &mod) {
  auto srcPosition = stmt.GetSrcPosition();
  WARN(kLncWarn, "%s:%d warning: Dereference of nullable pointer",
       mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
}

inline static void HandleReturnAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  WARN(kLncWarn, "%s:%d warning: %s return nonnull but got nullable pointer",
       mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), func.GetName().c_str());
}

inline static void HandleAssignAssertNonnull(const MeStmt &stmt, const MIRModule &mod) {
  auto srcPosition = stmt.GetSrcPosition();
  WARN(kLncWarn, "%s:%d warning: nullable assignment of nonnull pointer",
       mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
}

static void HandleCallAssertNonnull(const MeStmt &stmt, const MIRModule &mod) {
  auto srcPosition = stmt.GetSrcPosition();
  MeStmt *callStmt = stmt.GetNext();
  std::cmatch inLineFunc;
  while (callStmt != nullptr && !kOpcodeInfo.IsCall(callStmt->GetOp())) {
    if (IsInlineCommentStmt(*callStmt, inLineFunc)) {
      break;
    }
    callStmt = callStmt->GetNext();
  }
  CHECK_FATAL(callStmt != nullptr, "Should find a call stmt or inline comment stmt after OP_callassertnonnull [%s:%d]",
              mod.GetFileName().c_str(), srcPosition.MplLineNum());
  std::string targetFuncName;
  if (callStmt->GetOp() == OP_comment) {
    targetFuncName = inLineFunc[1].str();
  } else {
    targetFuncName = static_cast<CallMeStmt *>(callStmt)->GetTargetFunction().GetName();
  }
  WARN(kLncWarn, "%s:%d warning: nullable pointer passed to %s that requires a nonnull argument",
       mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), targetFuncName.c_str());
}

void MESafetyWarning::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MESafetyWarning::PhaseRun(MeFunction &meFunction) {
  auto *dom = GET_ANALYSIS(MEDominance, meFunction);
  auto &mod = meFunction.GetMIRModule();
  auto fileInfos = mod.GetSrcFileInfo();
  for (auto *bb : dom->GetReversePostOrder()) {
    for (auto &stmt : bb->GetMeStmts()) {
      switch (stmt.GetOp()) {
        case OP_assertnonnull:
          HandleAssertNonnull(stmt, mod);
          break;
        case OP_returnassertnonnull:
          HandleReturnAssertNonnull(stmt, mod, meFunction);
          break;
        case OP_assignassertnonnull:
          HandleAssignAssertNonnull(stmt, mod);
          break;
        case OP_callassertnonnull:
          HandleCallAssertNonnull(stmt, mod);
          break;
        default:
          break;
      }
    }
  }
  return true;
}
}  // namespace maple
