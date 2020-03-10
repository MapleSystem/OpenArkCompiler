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
