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
#ifndef MAPLE_DEX2MPL_OPTION_H
#define MAPLE_DEX2MPL_OPTION_H
#include "driver_option_common.h"

namespace maple {
enum Dex2mplOptionIndex {
  kDex2mplHelp = kCommonOptionEnd + 1,
  kOutput,
  kApkmplt,
  kDex2MplClass,
  kDex2MplClasslist,
  kFunc,
  kFunlist,
  kDex2MplInlineFunclist,
  kExtraFields,
  kEmitCxxHeader,
  kLitatter,
  kLitProfile,
  kSkipFieldlist,
  kLib,
  kExcludeClasslist,
  kIncludeClasslist,
  // OPTION
  kSkipInvokeCustom,
  kBinarymplt,
  kUnicode,
  kDex2MplOption,
  kMinupdate,
  kSkipCodeAnal,
  kMultiso,
  kMultisoMergeMpl,
  // OPTION (EMIT)
  kEmitClosure,
  kEmitTrace,
  kEmitMoredexinfo,
  kEmitClinit,
  kEmitClassDumper,
  // OPTION (NATIVE)
  kNonativeMethodDef,
  kNativeSetret,
  kNativeSetweak,
  // OPTION (GEN)
  kGenClassDefonly,
  kGenrtInterfaceCall,
  kGenJavaFuncMap,
  kGendeDaultFunch,
  kGendeDaultFuncc,
  // OPTION (STATIC)
  kGetStaticFields,
  kStaticFieldMap,
  kCollectClinit,
  kZeroCostClinit,
  // OPTION (CONTROL)
  kNoptrRef,
  kNocomment,
  kNoGcmalloc,
  kKeepDexname,
  kAddBarrier,
  kInsertLiteral,
  KAllHotLiteral,
  kFuncClosure,
  kExtraObjfields,
  kContainerPragma,
  kCvtName,
  kFastDataFlow,
  kDataFlowAlgorithm,
  kRefineCatch,
  kUseFullJavaFileName,
  // OPTION (OTHER)
  kBeta,
  kMaxID,
  kMinID,
  kDisableStrFac,
  // OPITIMIZATION
  kOptCheckCastoff,
  kOptEmitStmt,
  kOptStatistic,
  kOptSwitchDisable,
  // CHECKTOOL
  kChecktool,
  kCheckInvoke,
  kInvokeChecklist,
  kCheckIncomplete,
  kIncompleteWhiteList,
  kIncompleteWhiteListAuto,
  kIncompleteDetail,
  kCheckStaticString,
  kAddInterpreterInfo,

  // APK_OPTION
  kBlacklistInvoke,
  kGenstringFieldValue,
  kRcFieldattrWhiteList,
  kDexCatch,
  kAntiProguardFieldName,
  kDex2mplDecouple,
  kDex2mplAntiProguardAuto,
  // DAI 2.0
  kDex2mplDeferredVisit,
  kDeferredVisitWhiteList,
  kDeferredVisitStatic,
  kDeferredVisitCount,
  // opcode statistics
  kStatOpStatic,
  // MULTITHREAD
  kPidnum,
  // DEBUG
  kDebugDumpOption,
  kDebugLevel,
  kDexVersion,
  kDumpIncomplete,
  kDumpDataflow,
  kDumpClassdepOnly,
  kDex2mplDumpCFG,
  // Hidden API
  kHiddenApi,
  kHiddenApiBlackList,
  kHiddenApiGreyList,
  kHiddenApiDex,
  kHelpEnd,
};

const mapleOption::Descriptor dex2mplUsage[] = {
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "=============================================\n"
      " usage: dex2mpl foo.dex [ options ]\n"
      " options:\n", "dex2mpl", {} },
    // kHelp
    // kHelp, kVersion,
    { kDex2mplHelp, 0, "h", "help", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -h, --help          : print usage and exit.\n", "dex2mpl", {} },
    { kVersion, 0, "v", "version", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -v, --version       : print version and exit.\n", "dex2mpl", {} },
    // VALUE
    // OUTPUT, MPLT, APKMPLT, CLASS, CLASSLIST, FUNC, FUNCLIST, INLINEFUNCLIST,
    // EXTRAFIELDS, EMITCXXHEADER, LITATTR, LITPROFILE, SKIPFIELDLIST, LIB,
    // EXCLUDE_CLASSLIST,
    { kSkipInvokeCustom, 0, "skipcustom", "skipinvokecustom",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -skipcustom, --skipinvokecustom\n"
      "                       : skip compile invoke-custom instruction.\n", "dex2mpl", {} },
    { kOutput, 0, "", "output", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -output=/directoy/  : emit mpl and mplt to destination directoy\n"
      "                       : directoy must be exist.", "dex2mpl", {} },
    { kApkmplt, 0, "", "apkmplt", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -apkmplt=file1.mplt,file2.mplt\n"
      "                       : import apk mplt files ", "dex2mpl", {} },
    { kDex2MplClass, 0, "", "class", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -class=class1,class2,clss3\n"
      "                       : emit only these classes", "dex2mpl", {} },
    { kDex2MplClasslist, 0, "", "classlist", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -classlist=classlistfile\n"
      "                       : emit only classes in file\n"
      "                       : one class name each line", "dex2mpl", {} },
    { kExcludeClasslist, 0, "", "exclude_classlist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -exclude_classlist=exclude_classlistfile\n"
      "                       : exclude classes in file\n", "dex2mpl", {} },
    { kIncludeClasslist, 0, "", "include_classlist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -include_classlist=include_classlistfile\n"
      "                       : include classes in file\n", "dex2mpl", {} },
    { kFunc, 0, "", "func", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -func=func1,func2,func\n"
      "                       : emit only these functions", "dex2mpl", {} },
    { kFunlist, 0, "", "funclist", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -funclist=funclistfile\n"
      "                       : emit only functions in file\n"
      "                       : one function name each line", "dex2mpl", {} },
    { kDex2MplInlineFunclist, 0, "", "inlinefunclist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -inlinefunclist=funclistfile\n"
      "                       : hot functions for inline candidate \n"
      "                       : one function name each line", "dex2mpl", {} },
    { kExtraFields, 0, "", "extrafields", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -extrafields=extrafieldsfile\n"
      "                       : one field each line, in format:\n"
      "                       : class, name, type, attributes", "dex2mpl", {} },
    { kEmitCxxHeader, 0, "", "emitcxxheader", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -emitcxxheader=filename\n"
      "                       : emit to file 'filename' c/c++ struct/class defs\n"
      "                       : for classes listed in classlistfile;\n"
      "                       : it requires classlistfile given by -classlist", "dex2mpl", {} },
    { kLitatter, 0, "", "litattr", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -litattr=<attr>     : <attr>=s (for fstatic),w (for weak), "
      "                       :        g (for global)\n"
      "                       : set how literal\n"
      "                       : strings are stored. Weak is default.", "dex2mpl", {} },
    { kLitProfile, 0, "", "litprofile", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -litprofile=litprofilefile\n"
      "                       : optimize based on literal profiling data", "dex2mpl", {} },
    { kSkipFieldlist, 0, "", "skipfieldlist", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -skipfieldlist=fieldlistfile\n"
      "                       : skip static field initialization in file", "dex2mpl", {} },
    { kLib, 0, "", "lib", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -lib=libname        : library name", "dex2mpl", {} },
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
      "===== options =====", "dex2mpl", {} },

    // OPTION
    // BINARYMPLT, UNICODE, OPTION, kMinupdate, kSkipCodeAnal
    { kBinarymplt, 0, "asciimplt", "", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -asciimplt          : create an ascii mplt instead of binary", "dex2mpl", {} },
    { kUnicode, 0, "", "unicode", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -unicode=n          : n=8,16,32; default 16", "dex2mpl", {} },
    { kDex2MplOption, 0, "", "option", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -option=m           : option bit values\n"
      "                         CHECKMERGE            1\n"
      "                         CHECKTIME             2\n"
      "                         CHECKASSERTION        4\n"
      "                         ENABLEANNOTATION      8\n"
      "                         SKIP1MERGE           16\n"
      "                         FORCEEMITCODE        32\n"
      "                         DUMPCLASSDEPONLY     64\n"
      "                         USEPARENTFIELDS     128\n"
      "                         NOPARENTFIELDS      256\n"
      "                         SORTFIELDS          512", "dex2mpl", {} },
    { kMinupdate, 0, "", "minupdate", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -minupdate          : check if mplt is updated,\n"
      "                       : if not, skip mplt output.", "dex2mpl", {} },
    { kSkipCodeAnal, 0, "", "skip-codeanal", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -skip-codeanal      : skip ProcessClasses in check process", "dex2mpl", {} },
    { kMultiso, 0, "", "multiso", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -multiso            : enable multi so", "dex2mpl", {} },
    { kMultisoMergeMpl, 0, "", "multisoMergeMpl", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -multisoMergeMpl=classes2.mplt,classes3.mplt,classes999.mplt \n"
      "                       : file list represent this [files].mpl should merge. \n"
      "                       : this example indicates classes2.mpl, classes3.mpl\n"
      "                       : and classes999.mpl will be merge", "dex2mpl", {} },
    // OPTION (EMIT)
    // EMITCLOSURE, EMITTRACE, EMITMOREDEXINFO, EMITCLINIT, EMITCLASSDUMPER,
    { kEmitClosure, 0, "", "emitclosure", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -emitclosure        : emit related class list in fileinfo", "dex2mpl", {} },
    { kEmitTrace, 0, "", "emittrace", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -emittrace          : emit trace of enter/leave functions", "dex2mpl", {} },
    { kEmitMoredexinfo, 0, "", "emitmoredexinfo", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -emitmoredexinfo    : emit more dex info entries", "dex2mpl", {} },
    { kEmitClinit, 0, "", "emitclinit", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -emitclinit         : insert clinit by checking static variables", "dex2mpl", {} },
    { kEmitClassDumper, 0, "", "emitclassdumper", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -emitclassdumper    : emit functions to dump classes", "dex2mpl", {} },

    // OPTION (NATIVE)
    // NONATIVEMETHODDEF, NATIVESETRET, NATIVESETWEAK,
    { kNonativeMethodDef, 0, "", "nonativemethoddef",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -nonativemethoddef  : don't generate body for native method", "dex2mpl", {} },
    { kNativeSetret, 0, "", "nativesetret", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -nativesetret       : insert default return for native method", "dex2mpl", {} },
    { kNativeSetweak, 0, "", "nativesetweak", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -nativesetweak      : mark weak for native method", "dex2mpl", {} },

    // OPTION (GEN)
    // GENCLASSDEFONLY, GENRTINTERFACECALL, GENJAVAFUNCMAP, GENDEDAULTFUNCH,
    // GENDEDAULTFUNCC,
    { kGenClassDefonly, 0, "", "t", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -t                  : create foo.mplt class header file", "dex2mpl", {} },
    { kGenrtInterfaceCall, 0, "", "genrtinterfacecall",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -genrtinterfacecall : generate intrinsic for interface call", "dex2mpl", {} },
    { kGenJavaFuncMap, 0, "", "genjavafuncmap", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -genjavafuncmap     : generate map for java native\n"
      "                       : function mangled names", "dex2mpl", {} },
    { kGendeDaultFunch, 0, "", "gendefaultfunch", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -gendefaultfunch    : generate default header files\n"
      "                       : for the functions in funclist", "dex2mpl", {} },
    { kGendeDaultFuncc, 0, "", "gendefaultfuncc", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -gendefaultfuncc    : generate default c files\n"
      "                       : for the functions in funclist", "dex2mpl", {} },

    // OPTION (STATIC)
    // GETSTATICFIELDS, STATICFIELDMAP, COLLECTCLINIT, ZEROCOSTCLINIT,
    { kGetStaticFields, 0, "", "getstaticfields", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -getstaticfields    : emit static fields", "dex2mpl", {} },
    { kStaticFieldMap, 0, "", "genstaticfieldmap",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -genstaticfieldmap  : generate map for static fields", "dex2mpl", {} },
    { kCollectClinit, 0, "", "collectclinit", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -collectclinit      : through static variables", "dex2mpl", {} },
    { kZeroCostClinit, 0, "", "zerocostclinit",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyOptional,  // Notes: zero or one
      "   -zerocostclinit     : use zero-cost clinit", "dex2mpl", {} },

    // OPTION (CONTROL)
    // NOPTYREF, NOCOMMENT, NOGCMALLOC, KEEPDEXNAME, ADDBARRIER, INSERTLITERAL, SKIPKH,
    // FUNCCLOSURE, EXTRAOBJFIELDS, CONTAINERPRAGMA, CVTNAME, REFINECATCH
    { kNoptrRef, 0, "", "noref", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -noref              : do NOT use primary type PTY_ref", "dex2mpl", {} },
    { kNocomment, 0, "", "nocomment", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -nocomment          : not emit comment statements", "dex2mpl", {} },
    { kNoGcmalloc, 0, "", "nogcmalloc", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -nogcmalloc         : do NOT generate GCMalloc,\n"
      "                       : generate regular malloc instead", "dex2mpl", {} },
    { kKeepDexname, 0, "", "oldname", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -oldname            : use old name scheme", "dex2mpl", {} },
    { kAddBarrier, 0, "", "addbarrier", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -addbarrier         : add barrier when needed", "dex2mpl", {} },
    { kInsertLiteral, 0, "", "insertliteral", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -insertliteral      : enable literal profiling", "dex2mpl", {} },
    { KAllHotLiteral, 0, "", "allhotliteral", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -allhotliteral      : enable all literal hot", "dex2mpl", {} },
    { kFuncClosure, 0, "", "funcclosure", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -funcclosure        : use closure to expand the function list", "dex2mpl", {} },
    { kExtraObjfields, 0, "", "extraobjfields",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -extraobjfields     : insert two extra fields,\n"
      "                       : itab and vtab, into Object", "dex2mpl", {} },
    { kContainerPragma, 0, "", "containerpragma",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -containerpragma    : save container pragma for ALIAS vars in mplt", "dex2mpl", {} },
    { kCvtName, 0, "", "cvtname", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -cvtname            : convert to maple names in funclist file", "dex2mpl", {} },
    { kFastDataFlow, 0, "", "fast-dataflow",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -fast-dataflow      : use fast dataflow algorithm", "dex2mpl", {} },
    { kDataFlowAlgorithm, 0, "", "dataflow-algorithm",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -dataflow-algorithm=xxx\n"
      "                       : xxx is algorithm name\n"
      "                       : iterative \n"
      "                       : multilevel (default)", "dex2mpl", {} },
    { kRefineCatch, 0, "", "refine-catch", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -refine-catch       : refine catch will remove exception def for exception edge", "dex2mpl", {} },
    { kUseFullJavaFileName, 0, "", "usefulljavafilename",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -usefulljavafilename\n"
      "                       : usefulljavafilename will record full java source file name in .mpl", "dex2mpl", {} },
    // OPTION (OTHER)
    // BETA, MAXID, MINID, DISABLE_STRFAC
    { kBeta, 0, "", "beta", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -beta               : code that needs further work, \n"
      "                       : used to pass around partial work.", "dex2mpl", {} },
    { kMaxID, 0, "", "maxid", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNumeric,
      "   -maxid              :", "dex2mpl", {} },
    { kMinID, 0, "", "minid", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNumeric,
      "   -minid              :", "dex2mpl", {} },
    { kDisableStrFac, 0, "", "disable-strfac", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
    "   -disable-strfac     : disable StringFactory replace.", "dex2mpl", {} },
    // OPITIMIZATION
    // OPTCHECKCASTOFF, OPTEMITSTMT, OPTSTATISTIC,
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== OPTIMIZATION options =====", "dex2mpl", {} },
    { kOptCheckCastoff, 0, "", "optcheckcastoff", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -optcheckcastoff    : turn off CHECK_CAST optimization,\n"
        "                       : replace with a retype.", "dex2mpl", {} },
    { kOptEmitStmt, 0, "", "optemitstmt", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -optemitstmt        : emit stmts during optimization.", "dex2mpl", {} },
    { kOptStatistic, 0, "", "optstatistic", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyNone,
      "   -optstatistic       : dump optimization statistic info.", "dex2mpl", {} },
    { kOptSwitchDisable, 0, "", "opt-switch-disable", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -opt-switch-disable\n"
      "                       : turn off switch-combination optimization", "dex2mpl", {} },

    // CHECKTOOL
    // CHECKTOOL, CHECK_INVOKE, INVOKE_CHECKLIST, CHECK_INCOMPLETE
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== CHECKTOOL options =====", "dex2mpl", {} },
    { kChecktool, 0, "", "checktool", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -checktool          : enable check tool.", "dex2mpl", {} },
    { kCheckInvoke, 0, "", "check-invoke", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -check-invoke       : check call graph by invokeList.", "dex2mpl", {} },
    { kInvokeChecklist, 0, "", "invoke-checklist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -invoke-checklist=invoke.list\n"
      "                       : invoke check list file name.", "dex2mpl", {} },
    { kCheckIncomplete, 0, "", "check-incomplete", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -check-incomplete   : check incomplete type.", "dex2mpl", {} },
    { kIncompleteWhiteList, 0, "", "incomplete-whitelist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -incomplete-whitelist=incomplete.list\n"
      "                       : white list file for incomplete check.", "dex2mpl", {} },
    { kIncompleteWhiteListAuto, 0, "", "incomplete-whitelist-auto",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -incomplete-whitelist-auto\n"
      "                       : auto add incomplete type info whitelist", "dex2mpl", {} },
    { kIncompleteDetail, 0, "", "incomplete-detail", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -incomplete-detail  : dump incomplete detail.\n", "dex2mpl", {} },
    { kCheckStaticString, 0, "", "staticstringcheck", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -staticstringcheck  : check static string usage.", "dex2mpl", {} },
    { kAddInterpreterInfo, 0, "", "add-interpreter-info", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone,
      "   -add-interpreter-info  : add interpreter info.", "dex2mpl", {} },
    // APK_OPTION
    // BLACKLIST_INVOKE, GENSTRINGFIELDVALUE, ORINCATCH,
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== apk use only options =====", "dex2mpl", {} },
    { kGenstringFieldValue, 0, "", "gen-stringfieldvalue",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -gen-stringfieldvalue \n"
      "                       : generate _PTR_C_STR_ for string field with value.\n", "dex2mpl", {} },
    { kBlacklistInvoke, 0, "", "blacklist-invoke",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -blacklist-invoke=xxx.list\n"
      "                       : invoke blacklist file.", "dex2mpl", {} },
    { kRcFieldattrWhiteList, 0, "", "rc-fieldattr-whitelist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -rc-fieldattr-whitelist=file.list\n"
      "                       : RC fieldattr whitelist file name.", "dex2mpl", {} },
    { kDexCatch, 0, "", "dexcatch", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -dexcatch           : use dex catch exception list.", "dex2mpl", {} },
    { kAntiProguardFieldName, 0, "", "anti-proguard-info-fieldname",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -anti-proguard-info-fieldname=xxx.info\n"
      "                       : fieldname anti-proguard info file name (generated by gendeps)", "dex2mpl", {} },
    { kDex2mplDecouple, 0, "", "decouple", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -decouple           : enable decouple", "dex2mpl", {} },
    { kDex2mplAntiProguardAuto, 0, "", "anti-proguard-auto",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -anti-proguard-auto : enable auto anti-proguard for field name\n"
      "                         this option will disable -anti-proguard-info-fieldname", "dex2mpl", {} },
    // Hidden API
    { kHiddenApi, 0, "", "hiddenapi", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -hiddenapi          : enable hiddenapi.", "dex2mpl", {} },
    { kHiddenApiBlackList, 0, "", "hiddenapi-blacklist",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -hiddenapi-blacklist=xxx.list\n"
      "                       : hiddenapi-blacklist file name.", "dex2mpl", {} },
    { kHiddenApiGreyList, 0, "", "hiddenapi-greylist",
      mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -hiddenapi-greylist=xxx.list\n"
      "                       : hiddenapi-greylist file name.", "dex2mpl", {} },
    { kHiddenApiDex, 0, "", "hiddenapidex", mapleOption::kBuildTypeExperimental, mapleOption::kArgCheckPolicyRequired,
      "   -hiddenapidex=xxx.list\n"
      "                       : hiddenapidex list.", "dex2mpl", {} },
    // DAI 2.0
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== DAI =====", "dex2mpl", {} },
    { kDex2mplDeferredVisit, 0, "", "deferred-visit", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -deferred-visit     : generate deferred MCC call for undefined type", "dex2mpl", {} },
    { kDeferredVisitWhiteList, 0, "", "deferred-visit-whitelist",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyRequired,
      "   -deferred-visit-whitelist\n"
      "                       : whitelist file used to generate deferred visit.\n"
      "                       : each line represents a field or method name in format\n"
      "                       :   ClassName|FieldName|FieldTypeName\n"
      "                       :   ClassName|MethodName|MethodSignatureName\n"
      "                       : all names are in dex format", "dex2mpl", {} },
    { kDeferredVisitStatic, 0, "",
      "deferred-visit-static", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNumeric,
      "   -deferred-visit-static=mode\n"
      "                       : set the mode of DAI for static visit\n"
      "                           0: disable\n"
      "                           1: for static visit which is undefined\n"
      "                           2: for static visit which is undefined or has different dex/mpl class",
      "dex2mpl", {} },
    { kDeferredVisitCount, 0, "", "deferred-visit-count",
      mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -deferred-visit-count\n"
      "                       : count the DAI visit", "dex2mpl", {} },

    // opcode statistics
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== Opcode Statistics =====", "dex2mpl", {} },
    { kStatOpStatic, 0, "", "stat-op-static", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNone,
      "   -stat-op-static     : count the numbers of static opcode in different catagories", "dex2mpl", {} },

    // MULTITHREAD
    // PIDNUM,
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "===== MULTITHREAD options =====", "dex2mpl", {} },
    { kPidnum, 0, "", "j", mapleOption::kBuildTypeProduct, mapleOption::kArgCheckPolicyNumeric,
      "   -jn                 : using n processes to run the task in parallel", "dex2mpl", {} },

    // DEBUG
    // DEBUG_DUMP_OPTION, DEBUGLEVEL, DEXVERSION, DUMPINCOMPLETE,
    // DUMPDATAFLOW, DUMPCLASSDEPONLY, DUMPCFG,
    { kUnknown, 0, "", "", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyUnknown,
      "===== DEBUG options =====", "dex2mpl", {} },
    { kDebugDumpOption, 0, "", "dump-option", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dump-option        : dump command line options.n", "dex2mpl", {} },
    { kDebugLevel, 0, "", "d", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNumeric,
      "   -d=n                : debug print level n=1,2,3,...\n"
      "                       : (only in debug version)", "dex2mpl", {} },
    { kDexVersion, 0, "", "dex_version", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dex_version        : display dex file version", "dex2mpl", {} },
    { kDumpIncomplete, 0, "", "dump-incomplete", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dump-incomplete    : dump incomplete type messages.", "dex2mpl", {} },
    { kDumpDataflow, 0, "", "dumpdataflow", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dumpdataflow       : dump dataflow analysis info\n"
      "                       : (reaching definition).", "dex2mpl", {} },
    { kDumpClassdepOnly, 0, "", "dumpclassdeponly", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dumpclassdeponly   : same as -option=DUMPCLASSDEPONLY", "dex2mpl", {} },
    { kDex2mplDumpCFG, 0, "", "dumpcfg", mapleOption::kBuildTypeDebug, mapleOption::kArgCheckPolicyNone,
      "   -dumpcfg            : dump cfg", "dex2mpl", {} },
    { kHelpEnd, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyUnknown,
      "=============================================", "dex2mpl", {} },
    { kUnknown, 0, "", "", mapleOption::kBuildTypeAll, mapleOption::kArgCheckPolicyNone, "", "dex2mpl", {} }};
} // namespace maple
#endif // MAPLE_DEX2MPL_OPTION_H
