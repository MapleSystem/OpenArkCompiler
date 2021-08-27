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
#ifndef MAPLE_IPA_INCLUDE_FOO_H
#define MAPLE_IPA_INCLUDE_FOO_H
#include "mir_module.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "class_hierarchy_phase.h"
#include "call_graph.h"
#include "maple_phase.h"

namespace maple {

class Foo : public AnalysisResult {
 public:
  Foo(MIRModule *mod, MemPool *memPool, MIRBuilder &builder, KlassHierarchy *kh)
      : AnalysisResult(memPool), mirModule(mod), allocator(memPool), mirBuilder(builder), kh(kh) {}
  ~Foo() = default;
  void DoFoo(const maple::SCCNode &scc);

 private:
  MIRModule *mirModule;
  MapleAllocator allocator;
  MIRBuilder &mirBuilder;
  KlassHierarchy *kh;
};

MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCFoo, SCCNode)
  Foo *GetResult() {
    return foo;
  }
  Foo *foo = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_FOO_H
