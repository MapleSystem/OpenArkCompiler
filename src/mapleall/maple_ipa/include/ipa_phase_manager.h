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
#ifndef MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
#define MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_function.h"
#include "me_phase_manager.h"
#include "foo.h"

namespace maple {

/* ==== new phase manager ==== */
class IpaSccPM : public SccPM {
 public:
  explicit IpaSccPM(MemPool *memPool) : SccPM(memPool, &id) {}
  PHASECONSTRUCTOR(IpaSccPM);
  std::string PhaseName() const override;
  ~IpaSccPM() override {}

  void SetIpaInput(const std::string &str) {
    ipaInput = str;
  }

  bool PhaseRun(MIRModule &m) override;
 private:
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
  void DumpIR(MeFunction &f, const std::string phaseName, bool isBefore);
  void DoPhasesPopulate(const MIRModule &m);

  std::string ipaInput = "";
};
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
