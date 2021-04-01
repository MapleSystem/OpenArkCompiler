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
#ifndef MAPLE_MPL2MPL_INCLUDE_OPENPROFILE_H
#define MAPLE_MPL2MPL_INCLUDE_OPENPROFILE_H
#include "mir_module.h"

namespace maple {
class DoOpenProfile : public ModulePhase {
 public:
  DoOpenProfile(ModulePhaseID id) : ModulePhase(id) {}

  AnalysisResult *Run(MIRModule *module, ModuleResultMgr*) override {
    uint32 dexNameIdx = module->GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    bool deCompressSucc = module->GetProfile().DeCompress(Options::profile, dexName);
    if (!deCompressSucc) {
      LogInfo::MapleLogger() << "WARN: DeCompress() failed in DoOpenProfile::Run()\n";
    }
    return nullptr;
  }

  std::string PhaseName() const override {
    return "openprofile";
  }
};
}  // namespace maple
#endif  // MAPLE_MPL2MPL_INCLUDE_OPENPROFILE_H
