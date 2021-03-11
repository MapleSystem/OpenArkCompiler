/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_JBC2MPL_OPTION_H
#define MAPLE_JBC2MPL_OPTION_H
#include "option_descriptor.h"

namespace maple {
enum JbcOptionIndex {
  kUseStringFactory,
  kOutMpl,
  kInClass,
  kInJar,
  kInApkMplt,
  kOutClassXML,
  kOutPath,
  // profiling
  kProfileLiteral,
  // options
  kGenMplt,
  kGenAsciiMplt,
  kTypeSameName,
  kEnableTypeInfer,
  kDumpLevel,
  kDumpTime,
  kDumpPolymorphic,
  kDebugJBCFuncName,
  kDebugDumpJBCFuncCFG,
  kDebugDumpJBCFuncLocalVar,
  kDebugDumpFEFuncStmt,
  kDebugDumpFEFuncCFG,
  kDebugDumpFEFuncDataFlow,
};

const mapleOption::Descriptor jbcUsage[] = {
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
    "========================================\n"
    " Usage: jbc2mpl [ options ]\n"
    " options:\n",
    "jbc2mpl",
    {} },
  { kUseStringFactory, 0, "", "use-string-factory", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -use-string-factory    : Replace String.<init> by StringFactory call",
    "jbc2mpl",
    {} },
  { kOutMpl, 0, "o", "out", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -o, -out output.mpl    : output mpl name",
    "jbc2mpl",
    {} },
  { kInClass, 0, "", "inclass", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -inclass file1.class,file2.class\n"
    "                         : input class files",
    "jbc2mpl",
    {} },
  { kInJar, 0, "", "injar", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -injar file1.jar,file2.jar\n"
    "                         : input jar files",
    "jbc2mpl",
    {} },
  { kInMplt, 0, "", "mplt", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -mplt file1.mplt,file2.mplt\n"
    "                         : import mplt files",
    "jbc2mpl",
    {} },
  { kInApkMplt, 0, "", "apkmplt", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -apkmplt file1.mplt,file2.mplt\n"
    "                         : import apk mplt files",
    "jbc2mpl",
    {} },
  { kOutClassXML, 0, "", "out-classXML", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -out-classXML classes.xml\n"
    "                         : output class in XML",
    "jbc2mpl",
    {} },
  { kOutPath, 0, "", "output", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -output path           : output path",
    "jbc2mpl",
    {} },
  { kGenMplt, 0, "t", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -t                     : Generate mplt only",
    "jbc2mpl",
    {} },
  { kGenAsciiMplt, 0, "", "asciimplt", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -asciimplt             : Generate ascii mplt",
    "jbc2mpl",
    {} },
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
    "========== Profiling ==========",
    "jbc2mpl",
    {} },
  { kProfileLiteral, 0, "", "litprofile", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -litprofile=litprofilefile\n"
    "                         : optimize based on literal profiling data",
    "jbc2mpl",
    {} },
  { kTypeSameName, 0, "", "same-name", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -same-name=strategy\n"
    "                         : strategy when type with the same name\n"
    "                         : strategy list\n"
    "                         :   ERROR:    print error and stop compiling (default)\n"
    "                         :   USELIB:   use first imported by sequence apkmplt-mplt-class/jar\n"
    "                         :   USEINPUT: use definition in class/jar",
    "jbc2mpl",
    {} },
  { kEnableTypeInfer, 0, "", "enable-typeinfer", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -enable-typeinfer      : enable type infer operation",
    "jbc2mpl",
    {} },
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
    "========== Debugging ==========",
    "jbc2mpl",
    {} },
  { kDumpLevel, 0, "", "dump-level", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -dump-level            : dump level",
    "jbc2mpl",
    {} },
  { kDumpTime, 0, "", "dump-time", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -dump-time             : dump time",
    "jbc2mpl",
    {} },
  { kDumpPolymorphic, 0, "", "dump-polymorphic", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -dump-polymorphic      : dump polymorphic methods",
    "jbc2mpl",
    {} },
  { kDebugJBCFuncName, 0, "", "debug-jbcfuncname", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyRequired,
    "  -debug-jbcfuncname=funcName\n"
    "                         : debug jbc func name",
    "jbc2mpl",
    {} },
  { kDebugDumpJBCFuncCFG, 0, "", "debug-dump-jbcfunc-cfg", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -debug-dump-jbcfunc-cfg\n"
    "                         : debug dump jbc func cfg",
    "jbc2mpl",
    {} },
  { kDebugDumpJBCFuncLocalVar, 0, "", "debug-dump-jbcfunc-localvar",
    mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -debug-dump-jbcfunc-localvar\n"
    "                         : debug dump jbc func local variable info",
    "jbc2mpl",
    {} },
  { kDebugDumpFEFuncStmt, 0, "", "debug-dump-fefunc-stmt", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -debug-dump-fefunc-stmt\n"
    "                         : debug dump fe func stmt",
    "jbc2mpl",
    {} },
  { kDebugDumpFEFuncCFG, 0, "", "debug-dump-fefunc-cfg", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -debug-dump-fefunc-cfg\n"
    "                         : debug dump fe func cfg",
    "jbc2mpl",
    {} },
  { kDebugDumpFEFuncDataFlow, 0, "", "debug-dump-fefunc-dataflow",
    mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "  -debug-dump-fefunc-dataflow\n"
    "                         : debug dump fe func dataflow",
    "jbc2mpl",
    {} },
  { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
    "",
    "jbc2mpl",
    {} }
};
} // namespace maple
#endif // MAPLE_JBC2MPL_OPTION_H
