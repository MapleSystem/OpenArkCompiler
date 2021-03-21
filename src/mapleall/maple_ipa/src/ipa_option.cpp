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
#include "ipa_option.h"
#include "driver_option_common.h"
#include "mpl_logging.h"

namespace maple {
using namespace mapleOption;
enum OptionIndex {
  kIpaHelp = kCommonOptionEnd + 1,
  kIpaOptL1,
  kIpaOptL2,
  kIpaEffectIPA,
  kIpaInlineHint,
  kIpaQuiet,
};

const Descriptor kUsage[] = {
  { kIpaHelp,
    0,
    "h",
    "help",
    kBuildTypeAll,
    kArgCheckPolicyOptional,
    "  -h --help                   \tPrint usage and exit.Available command names:\n"
    "                              \tmplipa\n",
    "mplipa",
    {} },
  { kIpaOptL1,
    0,
    "",
    "O1",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "  --O1                        \tEnable basic inlining\n",
    "mplipa",
    {} },
  { kIpaOptL2,
    0,
    "",
    "O2",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "  --O2                        \tEnable greedy inlining\n",
    "mplipa",
    {} },
  { kIpaEffectIPA,
    0,
    "",
    "effectipa",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --effectipa                 \tEnable method side effect for ipa\n",
    "mplipa",
    {} },
  { kIpaInlineHint,
    0,
    "",
    "inlinefunclist",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --inlinefunclist=           \tInlining related configuration\n",
    "mplipa",
    {} },
  { kIpaQuiet,
    0,
    "",
    "quiet",
    kBuildTypeProduct,
    kArgCheckPolicyNone,
    "  --quiet                     \tDisable out debug info\n",
    "mplipa",
    {} },
  { kUnknown,
    0,
    "",
    "",
    kBuildTypeAll,
    kArgCheckPolicyNone,
    "",
    "mplipa",
    {} }
};

IpaOption &IpaOption::GetInstance() {
  static IpaOption instance;
  return instance;
}

IpaOption::IpaOption() {
  CreateUsages(kUsage);
}

bool PrintHelpAndExit(const OptionParser &optionParser) {
  optionParser.PrintUsage("mplipa");
  return false;
}

bool IpaOption::SolveOptions(const mapleOption::OptionParser &optionParser) const {
  bool result = true;
  for (const mapleOption::Option &opt : optionParser.GetOptions()) {
    switch (opt.Index()) {
      case kIpaHelp: {
        if (opt.Args().empty()) {
          result = PrintHelpAndExit(optionParser);
        }
        break;
      }
      case kIpaOptL1:
        break;
      case kIpaOptL2:
        break;
      case kIpaEffectIPA:
        break;
      case kIpaQuiet:
        MeOption::quiet = true;
        Options::quiet = true;
        break;
      case kIpaInlineHint:
        MeOption::inlineFuncList = opt.Args();
        break;
      default:
        LogInfo::MapleLogger() << "unhandled case in ParseCmdline\n";
        result = false;
    }
  }
  return result;
}

bool IpaOption::ParseCmdline(int argc, char **argv, std::vector<std::string> &fileNames) {
  OptionParser optionParser;
  optionParser.RegisteUsages(DriverOptionCommon::GetInstance());
  optionParser.RegisteUsages(IpaOption::GetInstance());
  int ret = optionParser.Parse(argc, argv, "mplipa");
  if (ret != kErrorNoError) {
    return false;
  }
  bool result = SolveOptions(optionParser);
  if (!result) {
    return result;
  }
  for (std::string optionArg : optionParser.GetNonOptions()) {
    fileNames.push_back(optionArg);
  }
  return true;
}
}  // namespace maple

