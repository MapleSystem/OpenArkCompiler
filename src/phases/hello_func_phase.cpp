#include "me_phase.h"
#include "bb.h"
#include "me_option.h"
#include "dominance.h"
#include "me_function.h"
#include "dse.h"
#include "me_cfg.h"
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "me_dse.h"

namespace maple {

class HelloFuncPhase : public MeFuncPhase {
 public:
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override {
    CHECK_NULL_FATAL(func);
    std::cout << "HelloFuncPhase run..." << std::endl;
    (void)funcResMgr->GetAnalysisResult(MeFuncPhase_ALIASCLASS, func);
    auto *postDom = static_cast<Dominance*>(funcResMgr->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
    CHECK_NULL_FATAL(postDom);
    MeDSE dse(func, postDom, true);
    dse.RunDSE();
    func->Verify();
    return nullptr;
  }

  std::string PhaseName() const override {
    return "hellofuncphase";
  }
};

EXPORT_PHASE(HelloFuncPhase)

}
