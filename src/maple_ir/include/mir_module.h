/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MAPLE_IR_INCLUDE_MIR_MODULE_H
#define MAPLE_IR_INCLUDE_MIR_MODULE_H
#include "types_def.h"
#include "prim_types.h"
#include "intrinsics.h"
#include "opcodes.h"
#include "mpl_logging.h"
#include "muid.h"
#include "profile.h"
#if MIR_FEATURE_FULL
#include <string>
#include <unordered_set>
#include "mempool.h"
#include "mempool_allocator.h"
#include "maple_string.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
class CallInfo;  // circular dependency exists, no other choice
class MIRModule;  // circular dependency exists, no other choice
class MIRBuilder;  // circular dependency exists, no other choice
using MIRModulePtr = MIRModule*;
using MIRBuilderPtr = MIRBuilder*;

enum MIRFlavor {
  kFlavorUnknown,
  kFeProduced,
  kMeProduced,
  kBeLowered,
  kMmpl,
  kCmplV1,
  kCmpl  // == CMPLv2
};

enum MIRSrcLang {
  kSrcLangUnknown,
  kSrcLangC,
  kSrcLangJs,
  kSrcLangJava,
  kSrcLangCPlusPlus
};

// blksize gives the size of the memory block in bytes; there are (blksize+3)/4
// words; 1 bit for each word, so the bit vector's length in bytes is
// ((blksize+3)/4+7)/8
static inline uint32 BlockSize2BitVectorSize(uint32 blkSize) {
  uint32 bitVectorLen = ((blkSize + 3) / 4 + 7) / 8;
  return ((bitVectorLen + 3) >> 2) << 2;  // round up to word boundary
}

#if MIR_FEATURE_FULL
class MIRType;  // circular dependency exists, no other choice
class MIRFunction;  // circular dependency exists, no other choice
class MIRSymbol;  // circular dependency exists, no other choice
class MIRSymbolTable;  // circular dependency exists, no other choice
class MIRFloatConst;  // circular dependency exists, no other choice
class MIRDoubleConst;  // circular dependency exists, no other choice
class MIRBuilder;  // circular dependency exists, no other choice
class DebugInfo;  // circular dependency exists, no other choice
class BinaryMplt;  // circular dependency exists, no other choice
class EAConnectionGraph;  // circular dependency exists, no other choice
using MIRInfoPair = std::pair<GStrIdx, uint32>;
using MIRInfoVector = MapleVector<MIRInfoPair>;
using MIRDataPair = std::pair<GStrIdx, std::vector<uint8>>;
using MIRDataVector = MapleVector<MIRDataPair>;
constexpr int kMaxEncodedValueLen = 10;
struct EncodedValue {
  uint8 encodedValue[kMaxEncodedValueLen] = { 0 };
};

class MIRTypeNameTable {
 public:
  explicit MIRTypeNameTable(MapleAllocator &allocator)
      : gStrIdxToTyIdxMap(std::less<GStrIdx>(), allocator.Adapter()) {}

  ~MIRTypeNameTable() = default;

  const MapleMap<GStrIdx, TyIdx> &GetGStrIdxToTyIdxMap() const {
    return gStrIdxToTyIdxMap;
  }

  TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const {
    auto it = gStrIdxToTyIdxMap.find(idx);
    if (it == gStrIdxToTyIdxMap.end()) {
      return TyIdx(0);
    }
    return it->second;
  }

  void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx) {
    gStrIdxToTyIdxMap[gStrIdx] = tyIdx;
  }

  size_t Size() const {
    return gStrIdxToTyIdxMap.size();
  }
 private:
  MapleMap<GStrIdx, TyIdx> gStrIdxToTyIdxMap;
};

class MIRModule {
 public:
  bool firstInline = true;
  using CallSite = std::pair<CallInfo*, PUIdx>;

  explicit MIRModule(const std::string &fn = "");
  MIRModule(MIRModule &p) = delete;
  MIRModule &operator=(const MIRModule &module) = delete;
  ~MIRModule();

  const MemPool *GetMemPool() const {
    return memPool;
  }
  MemPool *GetMemPool() {
    return memPool;
  }
  MemPool *GetPragmaMemPool() {
    return pragmaMemPool;
  }
  MapleAllocator &GetPragmaMPAllocator() {
    return pragmaMemPoolAllocator;
  }
  const MapleAllocator &GetMPAllocator() const {
    return memPoolAllocator;
  }
  MapleAllocator &GetMPAllocator() {
    return memPoolAllocator;
  }

  const MapleVector<MIRFunction*> &GetFunctionList() const {
    return functionList;
  }
  MapleVector<MIRFunction*> &GetFunctionList() {
    return functionList;
  }

  const MIRFunction *GetFunction(size_t cnt) const {
    CHECK_FATAL(cnt < functionList.size(), "array index out of range");
    return functionList[cnt];
  }
  MIRFunction *GetFunction(size_t cnt) {
    CHECK_FATAL(cnt < functionList.size(), "array index out of range");
    return functionList[cnt];
  }

  MapleVector<MIRFunction*> &GetCompilationList() {
    return compilationList;
  }

  const MapleVector<std::string> &GetImportedMplt() const {
    return importedMplt;
  }
  void PushbackImportedMplt(const std::string &importFileName) {
    importedMplt.push_back(importFileName);
  }

  MIRTypeNameTable *GetTypeNameTab() {
    return typeNameTab;
  }

  const MapleVector<GStrIdx> &GetTypeDefOrder() const {
    return typeDefOrder;
  }
  void PushbackTypeDefOrder(GStrIdx gstrIdx) {
    typeDefOrder.push_back(gstrIdx);
  }

  void AddClass(TyIdx tyIdx);
  void RemoveClass(TyIdx tyIdx);

  void SetCurFunction(MIRFunction *f) {
    curFunction = f;
  }

  MIRSrcLang GetSrcLang() const {
    return srcLang;
  }

  const MapleSet<StIdx> &GetSymbolSet() const {
    return symbolSet;
  }

  Profile &GetProfile() {
    return profile;
  }

  void SetSomeSymbolNeedForDecl(bool s) {
    someSymbolNeedForwDecl = s;
  }

  MIRFunction *CurFunction() const {
    return curFunction;
  }

  MemPool *CurFuncCodeMemPool() const;
  MapleAllocator *CurFuncCodeMemPoolAllocator() const;
  MapleAllocator &GetCurFuncCodeMPAllocator() const;
  void AddExternStructType(TyIdx tyIdx);
  void AddExternStructType(const MIRType *t);
  void AddSymbol(StIdx stIdx);
  void AddSymbol(const MIRSymbol *s);
  void AddFunction(MIRFunction *pf) {
    functionList.push_back(pf);
    compilationList.push_back(pf);
  }

  void DumpGlobals(bool emitStructureType = true) const;
  void Dump(bool emitStructureType = true) const;
  void DumpToFile(const std::string &fileNameStr, bool emitStructureType = true) const;
  void DumpInlineCandidateToFile(const std::string &fileNameStr) const;
  const std::string &GetFileNameFromFileNum(uint32 fileNum) const;

  void DumpToHeaderFile(bool binaryMplt, const std::string &outputName = "");
  void DumpClassToFile(const std::string &path) const;
  void DumpFunctionList(bool skipBody = false) const;
  void DumpGlobalArraySymbol() const;
  void Emit(const std::string &outFileName) const;
  uint32 GetAndIncFloatNum() {
    return floatNum++;
  }

  void SetEntryFunction(MIRFunction *f) {
    entryFunc = f;
  }

  MIRFunction *GetEntryFunction() const {
    return entryFunc;
  }

  MIRFunction *FindEntryFunction();
  uint32 GetFileinfo(GStrIdx strIdx) const;
  void OutputAsciiMpl(const std::string &phaseName, bool emitStructureType = true);
  void OutputFunctionListAsciiMpl(const std::string &phaseName);
  const std::string &GetFileName() const {
    return fileName;
  }

  std::string GetFileNameAsPostfix() const;
  void SetFileName(const std::string &name) {
    fileName = name;
  }

  bool IsJavaModule() const {
    return srcLang == kSrcLangJava;
  }

  bool IsCModule() const {
    return srcLang == kSrcLangC || srcLang == kSrcLangCPlusPlus;
  }

  bool IsCharModule() const {
    return false;
  }

  void addSuperCall(const std::string &func) {
    superCallSet.insert(func);
  }

  bool findSuperCall(const std::string &func) const {
    return superCallSet.find(func) != superCallSet.end();
  }

  void SetFuncInfoPrinted() const;
  size_t GetOptFuncsSize() const {
    return optimizedFuncs.size();
  }

  void AddOptFuncs(MIRFunction *func) {
    return optimizedFuncs.push_back(func);
  }

  const MapleVector<MIRFunction*> &GetOptFuncs() const {
    return optimizedFuncs;
  }

  const MapleMap<PUIdx, MapleSet<FieldID>*> &GetPuIdxFieldInitializedMap() const {
    return puIdxFieldInitializedMap;
  }
  void SetPuIdxFieldSet(PUIdx puIdx, MapleSet<FieldID> *fieldIDSet) {
    puIdxFieldInitializedMap[puIdx] = fieldIDSet;
  }
  const auto &GetRealCaller() const {
    return realCaller;
  }

  auto &GetRealCaller() {
    return realCaller;
  }

  const MapleSet<FieldID> *GetPUIdxFieldInitializedMapItem(PUIdx key) const {
    auto it = puIdxFieldInitializedMap.find(key);
    if (it != puIdxFieldInitializedMap.end()) {
      return it->second;
    }
    return nullptr;
  }

  std::ostream &GetOut() const {
    return out;
  }

  const MIRBuilderPtr &GetMIRBuilder() const {
    return mirBuilder;
  }

  const std::string &GetEntryFuncName() const {
    return entryFuncName;
  }
  void SetEntryFuncName(const std::string &entryFunctionName) {
    entryFuncName = entryFunctionName;
  }

  TyIdx GetThrowableTyIdx() const {
    return throwableTyIdx;
  }
  void SetThrowableTyIdx(TyIdx throwableTypeIndex) {
    throwableTyIdx = throwableTypeIndex;
  }

  bool GetWithProfileInfo() const {
    return withProfileInfo;
  }
  void SetWithProfileInfo(bool withProfInfo) {
    withProfileInfo = withProfInfo;
  }

  BinaryMplt *GetBinMplt() {
    return binMplt;
  }
  void SetBinMplt(BinaryMplt *binaryMplt) {
    binMplt = binaryMplt;
  }

  bool IsInIPA() const {
    return inIPA;
  }
  bool IsWithMe() const {
    return withMe;
  }
  void SetWithMe(bool isWithMe) {
    withMe = isWithMe;
  }
  void SetInIPA(bool isInIPA) {
    inIPA = isInIPA;
  }

  const MIRInfoVector &GetFileInfo() const {
    return fileInfo;
  }
  void PushFileInfoPair(MIRInfoPair pair) {
    fileInfo.push_back(pair);
  }
  void SetFileInfo(const MIRInfoVector &fileInf) {
    fileInfo = fileInf;
  }

  const MapleVector<bool> &GetFileInfoIsString() const {
    return fileInfoIsString;
  }
  void SetFileInfoIsString(const MapleVector<bool> &fileInfoIsStr) {
    fileInfoIsString = fileInfoIsStr;
  }
  void PushFileInfoIsString(bool isString) {
    fileInfoIsString.push_back(isString);
  }

  const MIRDataVector &GetFileData() const {
    return fileData;
  }
  void PushbackFileData(const MIRDataPair &pair) {
    fileData.push_back(pair);
  }

  const MIRInfoVector &GetSrcFileInfo() const {
    return srcFileInfo;
  }
  void PushbackFileInfo(const MIRInfoPair &pair) {
    srcFileInfo.push_back(pair);
  }

  const MIRFlavor &GetFlavor() const {
    return flavor;
  }
  void SetFlavor(MIRFlavor flv) {
    flavor = flv;
  }

  void SetSrcLang(MIRSrcLang sourceLanguage) {
    srcLang = sourceLanguage;
  }

  void SetID(uint16 num) {
    id = num;
  }

  uint32 GetGlobalMemSize() const {
    return globalMemSize;
  }
  void SetGlobalMemSize(uint32 globalMemberSize) {
    globalMemSize = globalMemberSize;
  }

  uint8 *GetGlobalBlockMap() {
    return globalBlkMap;
  }
  void SetGlobalBlockMap(uint8 *globalBlockMap) {
    globalBlkMap = globalBlockMap;
  }

  uint8 *GetGlobalWordsTypeTagged() {
    return globalWordsTypeTagged;
  }
  void SetGlobalWordsTypeTagged(uint8 *globalWordsTyTagged) {
    globalWordsTypeTagged = globalWordsTyTagged;
  }

  uint8 *GetGlobalWordsRefCounted() {
    return globalWordsRefCounted;
  }
  void SetGlobalWordsRefCounted(uint8 *counted) {
    globalWordsRefCounted = counted;
  }

  void SetNumFuncs(uint32 numFunc) {
    numFuncs = numFunc;
  }

  MapleVector<GStrIdx> &GetImportFiles() {
    return importFiles;
  }

  void PushbackImportPath(GStrIdx path) {
    importPaths.push_back(path);
  }

  const MapleSet<uint32> &GetClassList() const {
    return classList;
  }

  const std::map<PUIdx, std::vector<CallInfo*>> &GetMethod2TargetMap() const {
    return method2TargetMap;
  }

  std::vector<CallInfo*> &GetMemFromMethod2TargetMap(PUIdx methodPuIdx) {
    return method2TargetMap[methodPuIdx];
  }

  void SetMethod2TargetMap(const std::map<PUIdx, std::vector<CallInfo*>> &map) {
    method2TargetMap = map;
  }

  void AddMemToMethod2TargetMap(PUIdx idx, const std::vector<CallInfo*> &callSite) {
    method2TargetMap[idx] = callSite;
  }

  bool HasTargetHash(PUIdx idx, uint32 key) const {
    auto it = method2TargetHash.find(idx);
    if (it == method2TargetHash.end()) {
        return false;
    }
    return it->second.find(key) != it->second.end();
  }
  void InsertTargetHash(PUIdx idx, uint32 key) {
    method2TargetHash[idx].insert(key);
  }
  void AddValueToMethod2TargetHash(PUIdx idx, const std::unordered_set<uint32> &value) {
    method2TargetHash[idx] = value;
  }

  const std::map<GStrIdx, EAConnectionGraph*> &GetEASummary() const {
    return eaSummary;
  }
  void SetEAConnectionGraph(GStrIdx funcNameIdx, EAConnectionGraph *eaCg) {
    eaSummary[funcNameIdx] = eaCg;
  }

 private:
  MemPool *memPool;
  MemPool *pragmaMemPool;
  MapleAllocator memPoolAllocator;
  MapleAllocator pragmaMemPoolAllocator;
  MapleVector<MIRFunction*> functionList;  // function table in the order of the appearance of function bodies; it
  // excludes prototype-only functions
  MapleVector<MIRFunction*> compilationList;  // functions in the order of to be compiled.
  MapleVector<std::string> importedMplt;
  MIRTypeNameTable *typeNameTab;
  MapleVector<GStrIdx> typeDefOrder;

  MapleSet<TyIdx> externStructTypeSet;
  MapleSet<StIdx> symbolSet;
  MapleVector<StIdx> symbolDefOrder;
  Profile profile;
  bool someSymbolNeedForwDecl = false;  // some symbols' addressses used in initialization

  std::ostream &out;
  MIRBuilder *mirBuilder;
  std::string entryFuncName = "";  // name of the entry function
  std::string fileName;
  TyIdx throwableTyIdx{0};  // a special type that is the base of java exception type. only used for java
  bool withProfileInfo = false;
  // for cg in mplt
  BinaryMplt *binMplt = nullptr;
  bool inIPA = false;
  bool withMe = true;
  MIRInfoVector fileInfo;              // store info provided under fileInfo keyword
  MapleVector<bool> fileInfoIsString;  // tells if an entry has string value
  MIRDataVector fileData;
  MIRInfoVector srcFileInfo;  // store info provided under srcFileInfo keyword
  MIRFlavor flavor = kFlavorUnknown;
  MIRSrcLang srcLang = kSrcLangUnknown;  // the source language
  uint16 id = 0xffff;
  uint32 globalMemSize = 0;  // size of storage space for all global variables
  uint8 *globalBlkMap = nullptr;   // the memory map of the block containing all the
  // globals, for specifying static initializations
  uint8 *globalWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word in globalBlkMap has typetag;
  // if yes, the typetag is the N+1th word; the
  // bitvector's size is given by
  // BlockSize2BitvectorSize(globalMemSize)
  uint8 *globalWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word points to a reference-counted
  // dynamic memory block; the bitvector's size
  // is given by BlockSize2BitvectorSize(globalMemSize)
  uint32 numFuncs = 0;  // because puIdx 0 is reserved, numFuncs is also the highest puIdx
  MapleVector<GStrIdx> importFiles;
  MapleVector<GStrIdx> importPaths;
  MapleSet<uint32> classList;

  std::map<PUIdx, std::vector<CallInfo*>> method2TargetMap;
  std::map<PUIdx, std::unordered_set<uint32>> method2TargetHash;
  std::map<GStrIdx, EAConnectionGraph*> eaSummary;

  MIRFunction *entryFunc = nullptr;
  uint32 floatNum = 0;
  MIRFunction *curFunction;
  MapleVector<MIRFunction*> optimizedFuncs;
  // Add the field for decouple optimization
  std::unordered_set<std::string> superCallSet;
  // record all the fields that are initialized in the constructor. module scope,
  // if puIdx doesn't appear in this map, it writes to all field id
  // if puIdx appears in the map, but it's corresponding MapleSet is nullptr, it writes nothing fieldID
  // if puIdx appears in the map, and the value of first corresponding MapleSet is 0, the puIdx appears in this module
  // and writes to all field id otherwise, it writes the field ids in MapleSet
  MapleMap<PUIdx, MapleSet<FieldID>*> puIdxFieldInitializedMap;
  std::map<std::pair<GStrIdx, GStrIdx>, GStrIdx> realCaller;
};
#endif  // MIR_FEATURE_FULL
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_MODULE_H
