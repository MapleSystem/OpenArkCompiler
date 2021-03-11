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
#ifndef MAPLE_IPA_OPTION_H
#define MAPLE_IPA_OPTION_H
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include "mir_parser.h"
#include "opcode_info.h"
#include "me_cfg.h"
#include "module_phase_manager.h"
#include "option_parser.h"
#include "interleaved_manager.h"
#include "option.h"
#include "inline.h"
#include "call_graph.h"

namespace maple {
class IpaOption : public maple::MapleDriverOptionBase {
 public:
  static IpaOption &GetInstance();

  ~IpaOption() = default;

  bool SolveOptions(const mapleOption::OptionParser &optionParser) const;

  bool ParseCmdline(int argc, char **argv, std::vector<std::string> &fileNames);

 private:
  IpaOption();
};
}  // namespace maple
#endif // MAPLE_IPA_OPTION_H