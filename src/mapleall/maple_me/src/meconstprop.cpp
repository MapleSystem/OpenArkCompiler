/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "meconstprop.h"
#include <fstream>
#include <iostream>
#include "clone.h"
#include "constantfold.h"
#include "mir_nodes.h"
#include "me_function.h"
#include "ssa_mir_nodes.h"
#include "mir_builder.h"

namespace maple {
void MeConstProp::IntraConstProp() const {}

void MeConstProp::InterConstProp() const {}

AnalysisResult *MeDoIntraConstProp::Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) {
  return nullptr;
}

AnalysisResult *MeDoInterConstProp::Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) {
  return nullptr;
}
}  // namespace maple
