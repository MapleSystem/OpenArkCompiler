#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under the Mulan PSL v1.
# You can use this software according to the terms and conditions of the Mulan PSL v1.
# You may obtain a copy of Mulan PSL v1 at:
#
#     http://license.coscl.org.cn/MulanPSL
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v1 for more details.
#
#include <iostream>
#include <fstream>
#include "option.h"
#include "mir_function.h"
#include "module_phase.h"
#include "class_hierarchy.h"

namespace maple {

class HelloModulePhase : public ModulePhase {
 public:
  AnalysisResult *Run(MIRModule *module, ModuleResultMgr *moduleResMgr) override {
    std::cout << "HelloModulePhase run..." << std::endl;
    MemPool *memPool = memPoolCtrler.NewMemPool("classhierarchy mempool");
    KlassHierarchy *kh = memPool->New<KlassHierarchy>(module, memPool);
    kh->BuildHierarchy();
    kh->CountVirtualMethods();
    kh->Dump();
    return kh;
  }

  std::string PhaseName() const override {
    return "hellomodulephase";
  }
};

EXPORT_PHASE(HelloModulePhase)
}
