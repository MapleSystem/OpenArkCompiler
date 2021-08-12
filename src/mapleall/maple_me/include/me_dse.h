/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_MEDSE_H
#define MAPLE_ME_INCLUDE_MEDSE_H
#include <iostream>
#include "bb.h"
#include "me_phase_manager.h"
#include "dse.h"

namespace maple {
class MeDSE : public DSE {
 public:
  MeDSE(MeFunction &func, Dominance *dom, const AliasClass *aliasClass, bool enabledDebug)
      : DSE(std::vector<BB*>(func.GetCfg()->GetAllBBs().begin(), func.GetCfg()->GetAllBBs().end()),
            *func.GetCfg()->GetCommonEntryBB(), *func.GetCfg()->GetCommonExitBB(), *func.GetMeSSATab(),
            *dom, aliasClass, enabledDebug, MeOption::decoupleStatic, func.IsLfo()),
        func(func),
        cfg(func.GetCfg()) {}
  virtual ~MeDSE() = default;

  void RunDSE();

 private:
  MeFunction &func;
  MeCFG *cfg;
  void VerifyPhi() const;
};

MAPLE_FUNC_PHASE_DECLARE(MEDse, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEDSE_H
