/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "emit.h"
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "reflection_analysis.h"
#include "muid_replacement.h"
#include "metadata_layout.h"
#include "string_utils.h"
using namespace namemangler;

namespace {
using namespace maple;
constexpr uint32 kSizeOfHugesoRoutine = 3;
constexpr uint32 kFromDefIndexMask32Mod = 0x40000000;

int32 GetPrimitiveTypeSize(const std::string &name) {
  if (name.length() != 1) {
    return -1;
  }
  char typeName = name[0];
  switch (typeName) {
    case 'Z':
      return static_cast<int32>(GetPrimTypeSize(PTY_u1));
    case 'B':
      return static_cast<int32>(GetPrimTypeSize(PTY_i8));
    case 'S':
      return static_cast<int32>(GetPrimTypeSize(PTY_i16));
    case 'C':
      return static_cast<int32>(GetPrimTypeSize(PTY_u16));
    case 'I':
      return static_cast<int32>(GetPrimTypeSize(PTY_i32));
    case 'J':
      return static_cast<int32>(GetPrimTypeSize(PTY_i64));
    case 'F':
      return static_cast<int32>(GetPrimTypeSize(PTY_f32));
    case 'D':
      return static_cast<int32>(GetPrimTypeSize(PTY_f64));
    case 'V':
      return static_cast<int32>(GetPrimTypeSize(PTY_void));
    default:
      return -1;
  }
}
DBGDieAttr *LFindAttribute(MapleVector<DBGDieAttr*> &vec, DwAt key) {
  for (DBGDieAttr *at : vec)
    if (at->GetDwAt() == key) {
      return at;
    }
  return nullptr;
}

DBGAbbrevEntry *LFindAbbrevEntry(MapleVector<DBGAbbrevEntry*> &abbvec, unsigned int key) {
  for (DBGAbbrevEntry *daie : abbvec) {
    if (!daie) {
      continue;
    }
    if (daie->GetAbbrevId() == key) {
      return daie;
    }
  }
  ASSERT(0, "");
  return nullptr;
}

bool LShouldEmit(unsigned int dwform) {
  return dwform != DW_FORM_flag_present;
}

DBGDie *LFindChildDieWithName(DBGDie *die, DwTag tag, const GStrIdx key) {
  for (DBGDie *c : die->GetSubDieVec()) {
    if (c->GetTag() == tag) {
      for (DBGDieAttr *a : c->GetAttrVec()) {
        if (a->GetDwAt() == DW_AT_name) {
          if ((a->GetDwForm() == DW_FORM_string || a->GetDwForm() == DW_FORM_strp) && a->GetId() == key.GetIdx()) {
            return c;
          } else {
            break;
          }
        }
      }
    }
  }
  return nullptr;
}

DBGDieAttr *LFindDieAttr(DBGDie *die, DwAt attrname) {
  for (DBGDieAttr *attr : die->GetAttrVec()) {
    if (attr->GetDwAt() == attrname) {
      return attr;
    }
  }
  return nullptr;
}

static void LUpdateAttrValue(DBGDieAttr *attr, int64_t newval) {
  attr->SetI(int32_t(newval));
}
}

namespace maplebe {
using namespace maple;
using namespace cfi;

void Emitter::EmitLabelRef(LabelIdx labIdx) {
  PUIdx pIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
  const char *idx = strdup(std::to_string(pIdx).c_str());
  outStream << ".L." << idx << "__" << labIdx;
}

void Emitter::EmitStmtLabel(LabelIdx labIdx) {
  EmitLabelRef(labIdx);
  outStream << ":\n";
}

void Emitter::EmitLabelPair(const LabelPair &pairLabel) {
  ASSERT(pairLabel.GetEndOffset() || pairLabel.GetStartOffset(), "NYI");
  EmitLabelRef(pairLabel.GetEndOffset()->GetLabelIdx());
  outStream << " - ";
  EmitLabelRef(pairLabel.GetStartOffset()->GetLabelIdx());
  outStream << "\n";
}

void Emitter::EmitLabelForFunc(const MIRFunction *func, LabelIdx labidx) {
  const std::string idx = static_cast<const std::string>(strdup(std::to_string(func->GetPuidx()).c_str()));
  outStream << ".L." << idx << "__" << labidx;
}

AsmLabel Emitter::GetTypeAsmInfoName(PrimType primType) const {
  uint32 size = GetPrimTypeSize(primType);
  /* case x : x occupies bytes of pty */
  switch (size) {
    case k1ByteSize:
      return kAsmByte;
    case k2ByteSize:
#if TARGAARCH64 || TARGRISCV64
      return kAsmShort;
#else
      return kAsmValue;
#endif
    case k4ByteSize:
      return kAsmLong;
    case k8ByteSize:
      return kAsmQuad;
    default:
      ASSERT(false, "NYI");
      break;
  }
  return kAsmLong;
}

void Emitter::EmitFileInfo(const std::string &fileName) {
#if defined(_WIN32) || defined(DARWIN)
  char *curDirName = getcwd(nullptr, 0);
#else
  char *curDirName = get_current_dir_name();
#endif
  CHECK_FATAL(curDirName != nullptr, "null ptr check ");
  Emit(asmInfo->GetCmnt());
  std::string path(curDirName);
#ifdef _WIN32
  std::string cgFile(path.append("\\mplcg"));
#else
  std::string cgFile(path.append("/mplcg"));
#endif
  Emit(cgFile);
  Emit("\n");

  std::string compile("Compiling ");
  Emit(asmInfo->GetCmnt());
  Emit(compile);
  Emit("\n");

  std::string beOptions("Be options");
  Emit(asmInfo->GetCmnt());
  Emit(beOptions);
  Emit("\n");

  path = curDirName;
  path.append("/").append(fileName);
  /* strip path before out/ */
  std::string out = "/out/";
  size_t pos = path.find(out.c_str(), 0, out.length());
  if (pos != std::string::npos) {
    path.erase(0, pos + 1);
  }
  std::string irFile("\"");
  irFile.append(path).append("\"");
  Emit(asmInfo->GetFile());
  Emit(irFile);
  Emit("\n");

  /* save directory path in index 8 */
  SetFileMapValue(0, path);

  /* .file #num src_file_name */
  if (cg->GetCGOptions().WithLoc()) {
    /* .file 1 mpl_file_name */
    if (cg->GetCGOptions().WithAsm()) {
      Emit("\t// ");
    }
    Emit(asmInfo->GetFile());
    Emit("1 ");
    Emit(irFile);
    Emit("\n");
    SetFileMapValue(1, irFile);   /* save ir file in 1 */
    if (cg->GetCGOptions().WithSrc()) {
      /* insert a list of src files */
      int i = 2;
      for (auto it : cg->GetMIRModule()->GetSrcFileInfo()) {
        if (cg->GetCGOptions().WithAsm()) {
          Emit("\t// ");
        }
        Emit(asmInfo->GetFile());
        Emit(it.second).Emit(" \"");
        std::string kStr = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first);
        Emit(kStr);
        Emit("\"\n");
        SetFileMapValue(i++, kStr);
      }
    }
  }
  free(curDirName);
#if TARGARM32
  Emit("\t.syntax unified\n");
  /*
   * "The arm instruction set is a subset of
   *  the most commonly used 32-bit ARM instructions."
   * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0210c/CACBCAAE.html
   */
  Emit("\t.arm\n");
  Emit("\t.fpu vfpv4\n");
  Emit("\t.arch armv7-a\n");
  Emit("\t.eabi_attribute Tag_ABI_PCS_RW_data, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_PCS_RO_data, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_PCS_GOT_use, 2\n");
  if (CGOptions::GetABIType() == CGOptions::kABIHard) {
    Emit("\t.eabi_attribute Tag_ABI_VFP_args, 1\n");
  }
  Emit("\t.eabi_attribute Tag_ABI_FP_denormal, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_FP_exceptions, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_FP_number_model, 3\n");
  Emit("\t.eabi_attribute Tag_ABI_align_needed, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_align_preserved, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_enum_size, 2\n");
  Emit("\t.eabi_attribute 30, 6\n");
  Emit("\t.eabi_attribute Tag_CPU_unaligned_access, 1\n");
  Emit("\t.eabi_attribute Tag_ABI_PCS_wchar_t, 4\n");
#endif /* TARGARM32 */
}

void Emitter::EmitAsmLabel(AsmLabel label) {
  switch (label) {
    case kAsmData: {
      Emit(asmInfo->GetData());
      Emit("\n");
      return;
    }
    case kAsmByte: {
      Emit(asmInfo->GetByte());
      return;
    }
    case kAsmShort: {
      Emit(asmInfo->GetShort());
      return;
    }
    case kAsmValue: {
      Emit(asmInfo->GetValue());
      return;
    }
    case kAsmLong: {
      Emit(asmInfo->GetLong());
      return;
    }
    case kAsmQuad: {
      Emit(asmInfo->GetQuad());
      return;
    }
    case kAsmZero:
      Emit(asmInfo->GetZero());
      return;
    default:
      ASSERT(false, "should not run here");
      return;
  }
}

void Emitter::EmitAsmLabel(const MIRSymbol &mirSymbol, AsmLabel label) {
  MIRType *mirType = mirSymbol.GetType();
  std::string symName;
  if (mirSymbol.GetStorageClass() == kScPstatic && mirSymbol.IsLocal()) {
    PUIdx pIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
    symName = mirSymbol.GetName() + std::to_string(pIdx);
  } else {
    symName = mirSymbol.GetName();
  }

  if (Globals::GetInstance()->GetBECommon()->IsEmptyOfTypeAlignTable()) {
    ASSERT(false, "container empty check");
  }

  switch (label) {
    case kAsmGlbl: {
      Emit(asmInfo->GetGlobal());
      Emit(symName);
      Emit("\n");
      return;
    }
    case kAsmHidden: {
      Emit(asmInfo->GetHidden());
      Emit(symName);
      Emit("\n");
      return;
    }
    case kAsmLocal: {
      Emit(asmInfo->GetLocal());
      Emit(symName);
      Emit("\n");
      return;
    }
    case kAsmWeak: {
      Emit(asmInfo->GetWeak());
      Emit(symName);
      Emit("\n");
      return;
    }
    case kAsmComm: {
      std::string size;
      if (isFlexibleArray) {
        size = std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex()) + arraySize);
      } else {
        size = std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex()));
      }
      Emit(asmInfo->GetComm());
      Emit(symName);
      Emit(", ");
      Emit(size);
      Emit(", ");
#if PECOFF
#if TARGARM || TARGAARCH64 || TARGARK || TARGRISCV64
      std::string align = std::to_string(
          static_cast<int>(log2(Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirType->GetTypeIndex()))));
#else
      std::string align = std::to_string(
          Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirType->GetTypeIndex()));
#endif
      emit(align.c_str());
#else /* ELF */
      /* output align, symbol name begin with "classInitProtectRegion" align is 4096 */
      MIRTypeKind kind = mirSymbol.GetType()->GetKind();
      MIRStorageClass storage = mirSymbol.GetStorageClass();
      if (symName.find("classInitProtectRegion") == 0) {
        Emit(4096);
      } else if (((kind == kTypeStruct) || (kind == kTypeClass) || (kind == kTypeArray) || (kind == kTypeUnion)) &&
                 ((storage == kScGlobal) || (storage == kScPstatic) || (storage == kScFstatic))) {
        int32 align = Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirType->GetTypeIndex());
        if (kSizeOfPtr < align) {
          Emit(std::to_string(align));
        } else {
          Emit(std::to_string(k8ByteSize));
        }
      } else {
        Emit(std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirType->GetTypeIndex())));
      }
#endif
      Emit("\n");
      return;
    }
    case kAsmAlign: {
      std::string align;
      if (mirSymbol.GetType()->GetKind() == kTypeStruct ||
          mirSymbol.GetType()->GetKind() == kTypeClass ||
          mirSymbol.GetType()->GetKind() == kTypeArray ||
          mirSymbol.GetType()->GetKind() == kTypeUnion) {
        align = "3";
      } else {
#if TARGARM32 || TARGAARCH64 || TARGARK || TARGRISCV64
        align = std::to_string(static_cast<int>(
          log2(Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirSymbol.GetType()->GetTypeIndex()))));
#else
        align =
          std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeAlign(mirSymbol.GetType()->GetTypeIndex()));
#endif
      }
      Emit(asmInfo->GetAlign());
      Emit(align);
      Emit("\n");
      return;
    }
    case kAsmSyname: {
      Emit(symName);
      Emit(":\n");
      return;
    }
    case kAsmSize: {
      std::string size;
      if (isFlexibleArray) {
          size = std::to_string(
              Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex()) + arraySize);
      } else {
          size = std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex()));
      }
      Emit(asmInfo->GetSize());
      Emit(symName);
      Emit(", ");
      Emit(size);
      Emit("\n");
      return;
    }
    case kAsmType: {
      Emit(asmInfo->GetType());
      Emit(symName);
      Emit(",");
      Emit(asmInfo->GetAtobt());
      Emit("\n");
      return;
    }
    default:
      ASSERT(false, "should not run here");
      return;
  }
}

void Emitter::EmitNullConstant(uint32 size) {
  EmitAsmLabel(kAsmZero);
  Emit(std::to_string(size));
  Emit("\n");
}

void Emitter::EmitCombineBfldValue(StructEmitInfo &structEmitInfo) {
  uint8 charBitWidth = GetPrimTypeSize(PTY_i8) * kBitsPerByte;
  while (structEmitInfo.GetCombineBitFieldWidth() > charBitWidth) {
    /* fetch the lower 8 bits */
    uint64 tmp = structEmitInfo.GetCombineBitFieldValue() & 0x00000000000000ffUL;
    EmitAsmLabel(kAsmByte);
    Emit(std::to_string(tmp));
    Emit("\n");
    structEmitInfo.DecreaseCombineBitFieldWidth(charBitWidth);
    structEmitInfo.SetCombineBitFieldValue(structEmitInfo.GetCombineBitFieldValue() >> charBitWidth);
  }
  if (structEmitInfo.GetCombineBitFieldWidth() != 0) {
    EmitAsmLabel(kAsmByte);
    Emit(std::to_string(structEmitInfo.GetCombineBitFieldValue()));
    Emit("\n");
  }
  CHECK_FATAL(charBitWidth != 0, "divide by zero");
  if ((structEmitInfo.GetNextFieldOffset() % charBitWidth) != 0) {
    uint8 value = charBitWidth - (structEmitInfo.GetNextFieldOffset() % charBitWidth);
    structEmitInfo.IncreaseNextFieldOffset(value);
  }
  structEmitInfo.SetTotalSize(structEmitInfo.GetNextFieldOffset() / charBitWidth);
  structEmitInfo.SetCombineBitFieldValue(0);
  structEmitInfo.SetCombineBitFieldWidth(0);
}

void Emitter::EmitBitFieldConstant(StructEmitInfo &structEmitInfo, MIRConst &mirConst, const MIRType *nextType,
                                   uint64 fieldOffset) {
  MIRType &mirType = mirConst.GetType();
  if (fieldOffset > structEmitInfo.GetNextFieldOffset()) {
    uint8 curFieldOffset = structEmitInfo.GetNextFieldOffset() - structEmitInfo.GetCombineBitFieldWidth();
    structEmitInfo.SetCombineBitFieldWidth(fieldOffset - curFieldOffset);
    EmitCombineBfldValue(structEmitInfo);
    ASSERT(structEmitInfo.GetNextFieldOffset() <= fieldOffset,
           "structEmitInfo's nextFieldOffset should be <= fieldOffset");
    structEmitInfo.SetNextFieldOffset(fieldOffset);
  }
  uint32 fieldSize = static_cast<MIRBitFieldType&>(mirType).GetFieldSize();
  MIRIntConst &fieldValue = static_cast<MIRIntConst&>(mirConst);
  /* Truncate the size of FieldValue to the bit field size. */
  if (fieldSize < fieldValue.GetBitWidth()) {
    fieldValue.Trunc(fieldSize);
  }
  /* Clear higher Bits for signed value  */
  if (structEmitInfo.GetCombineBitFieldValue() != 0) {
    structEmitInfo.SetCombineBitFieldValue((~(~0ULL << structEmitInfo.GetCombineBitFieldWidth())) &
                                           structEmitInfo.GetCombineBitFieldValue());
  }
  structEmitInfo.SetCombineBitFieldValue(
      (static_cast<uint64>(fieldValue.GetValue()) << structEmitInfo.GetCombineBitFieldWidth()) +
      structEmitInfo.GetCombineBitFieldValue());
  structEmitInfo.IncreaseCombineBitFieldWidth(fieldSize);
  structEmitInfo.IncreaseNextFieldOffset(fieldSize);
  if ((nextType == nullptr) || (kTypeBitField != nextType->GetKind())) {
    /* emit structEmitInfo->combineBitFieldValue */
    EmitCombineBfldValue(structEmitInfo);
  }
}

void Emitter::EmitStr(const std::string& mplStr, bool emitAscii, bool emitNewline) {
  const char *str = mplStr.c_str();
  size_t len = mplStr.size();

  if (emitAscii) {
    Emit("\t.ascii\t\"");  /* Do not terminate with \0 */
  } else {
    Emit("\t.string\t\"");
  }

  /*
   * don't expand special character in a writeout to .s,
   * convert all \s to \\s in string for storing in .string
   */
  for (int i = 0; i < len; i++) {
    /* Referred to GNU AS: 3.6.1.1 Strings */
    constexpr int kBufSize = 5;
    constexpr int kFirstChar = 0;
    constexpr int kSecondChar = 1;
    constexpr int kThirdChar = 2;
    constexpr int kLastChar = 4;
    char buf[kBufSize];
    if (isprint(*str)) {
      buf[kFirstChar] = *str;
      buf[kSecondChar] = 0;
      if (*str == '\\' || *str == '\"') {
        buf[kFirstChar] = '\\';
        buf[kSecondChar] = *str;
        buf[kThirdChar] = 0;
      }
      Emit(buf);
    } else if (*str == '\b') {
      Emit("\\b");
    } else if (*str == '\n') {
      Emit("\\n");
    } else if (*str == '\r') {
      Emit("\\r");
    } else if (*str == '\t') {
      Emit("\\t");
    } else if (*str == '\0') {
      buf[kFirstChar] = '\\';
      buf[kSecondChar] = '0';
      buf[kThirdChar] = 0;
      Emit(buf);
    } else {
      /* all others, print as number */
      int ret = snprintf_s(buf, sizeof(buf), k4BitSize, "\\%03o", (*str) & 0xFF);
      if (ret < 0) {
        FATAL(kLncFatal, "snprintf_s failed");
      }
      buf[kLastChar] = '\0';
      Emit(buf);
    }
    str++;
  }

  Emit("\"");
  if (emitNewline) {
    Emit("\n");
  }
}

void Emitter::EmitStrConstant(const MIRStrConst &mirStrConst, bool isIndirect) {
  if (isIndirect) {
    uint32 strId = mirStrConst.GetValue().GetIdx();
    if (stringPtr[strId] == 0) {
      stringPtr[strId] = mirStrConst.GetValue();
    }
    Emit("\t.dword\t").Emit(".LSTR__").Emit(std::to_string(strId).c_str());
    return;
  }

  const std::string ustr = GlobalTables::GetUStrTable().GetStringFromStrIdx(mirStrConst.GetValue());
  size_t len = ustr.size();
  if (isFlexibleArray) {
    arraySize += static_cast<uint32>(len) + 1;
  }
  EmitStr(ustr, false, false);
}

void Emitter::EmitStr16Constant(const MIRStr16Const &mirStr16Const) {
  Emit("\t.byte ");
  /* note: for now, u16string is emitted 2 bytes without any \u indication */
  const std::u16string &str16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(mirStr16Const.GetValue());
  constexpr int bufSize = 9;
  char buf[bufSize];
  char16_t c = str16[0];
  /* fetch the type of char16_t c's top 8 bit data */
  int ret1 = snprintf_s(buf, sizeof(buf), bufSize - 1, "%d,%d", (c >> 8) & 0xFF, c & 0xFF);
  if (ret1 < 0) {
    FATAL(kLncFatal, "snprintf_s failed");
  }
  buf[bufSize - 1] = '\0';
  Emit(buf);
  for (uint32 i = 1; i < str16.length(); ++i) {
    c = str16[i];
    /* fetch the type of char16_t c's top 8 bit data */
    int ret2 = snprintf_s(buf, sizeof(buf), bufSize - 1, ",%d,%d", (c >> 8) & 0xFF, c & 0xFF);
    if (ret2 < 0) {
      FATAL(kLncFatal, "snprintf_s failed");
    }
    buf[bufSize - 1] = '\0';
    Emit(buf);
  }
  if ((str16.length() & 0x1) == 1) {
    Emit(",0,0");
  }
}

void Emitter::EmitScalarConstant(MIRConst &mirConst, bool newLine, bool flag32, bool isIndirect) {
  MIRType &mirType = mirConst.GetType();
  AsmLabel asmName = GetTypeAsmInfoName(mirType.GetPrimType());
  switch (mirConst.GetKind()) {
    case kConstInt: {
      MIRIntConst &intCt = static_cast<MIRIntConst&>(mirConst);
      uint32 sizeInBits = GetPrimTypeBitSize(mirType.GetPrimType());
      if (intCt.GetBitWidth() > sizeInBits) {
        intCt.Trunc(sizeInBits);
      }
      if (flag32) {
        EmitAsmLabel(AsmLabel::kAsmLong);
      } else {
        EmitAsmLabel(asmName);
      }
      Emit(intCt.GetValue());
      if (isFlexibleArray) {
        arraySize += (sizeInBits / kBitsPerByte);
      }
      break;
    }
    case kConstFloatConst: {
      MIRFloatConst &floatCt = static_cast<MIRFloatConst&>(mirConst);
      EmitAsmLabel(asmName);
      Emit(std::to_string(floatCt.GetIntValue()));
      if (isFlexibleArray) {
        arraySize += k4ByteFloatSize;
      }
      break;
    }
    case kConstDoubleConst: {
      MIRDoubleConst &doubleCt = static_cast<MIRDoubleConst&>(mirConst);
      EmitAsmLabel(asmName);
      Emit(std::to_string(doubleCt.GetIntValue()));
      if (isFlexibleArray) {
        arraySize += k8ByteDoubleSize;
      }
      break;
    }
    case kConstStrConst: {
      MIRStrConst &strCt = static_cast<MIRStrConst&>(mirConst);
      if (cg->GetMIRModule()->IsCModule()) {
        EmitStrConstant(strCt, isIndirect);
      } else {
        EmitStrConstant(strCt);
      }
      break;
    }
    case kConstStr16Const: {
      MIRStr16Const &str16Ct = static_cast<MIRStr16Const&>(mirConst);
      EmitStr16Constant(str16Ct);
      break;
    }
    case kConstAddrof: {
      MIRAddrofConst &symAddr = static_cast<MIRAddrofConst&>(mirConst);
      StIdx stIdx = symAddr.GetSymbolIndex();
      MIRSymbol *symAddrSym = stIdx.IsGlobal() ? GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx())
          : CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx());
      if (stIdx.IsGlobal() == false && symAddrSym->GetStorageClass() == kScPstatic) {
        PUIdx pIdx = GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
        Emit("\t.quad\t" + symAddrSym->GetName() + std::to_string(pIdx));
      } else {
        Emit("\t.quad\t" + symAddrSym->GetName());
      }
      if (symAddr.GetOffset() != 0) {
        Emit(" + ").Emit(symAddr.GetOffset());
      }
      break;
    }
    case kConstAddrofFunc: {
      MIRAddroffuncConst &funcAddr = static_cast<MIRAddroffuncConst&>(mirConst);
      MIRFunction *func = GlobalTables::GetFunctionTable().GetFuncTable().at(funcAddr.GetValue());
      MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
      (void)Emit("\t.quad\t" + symAddrSym->GetName());
      break;
    }
    case kConstLblConst: {
      MIRLblConst &lbl = static_cast<MIRLblConst&>(mirConst);
      Emit("\t.dword\t");
      EmitLabelRef(lbl.GetValue());
      break;
    }
    default:
      ASSERT(false, "NYI");
      break;
  }
  if (newLine) {
    Emit("\n");
  }
}

void Emitter::EmitAddrofFuncConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx) {
  MIRAddroffuncConst &funcAddr = static_cast<MIRAddroffuncConst&>(elemConst);
  const std::string stName = mirSymbol.GetName();
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcAddr.GetValue());
  const std::string &funcName = func->GetName();
  if ((idx == kFuncDefNameIndex) && mirSymbol.IsMuidFuncInfTab()) {
    Emit("\t.long\t.Label.name.");
    Emit(funcName + " - .");
    Emit("\n");
    return;
  }
  if ((idx == kFuncDefSizeIndex) && mirSymbol.IsMuidFuncInfTab()) {
    Emit("\t.long\t.Label.end.");
    Emit(funcName + " - ");
    Emit(funcName + "\n");
    return;
  }
  if ((idx == static_cast<uint32>(MethodProperty::kPaddrData)) && mirSymbol.IsReflectionMethodsInfo()) {
#ifdef USE_32BIT_REF
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    Emit(funcName + " - .\n");
    return;
  }
  if (((idx == static_cast<uint32>(MethodInfoCompact::kPaddrData)) && mirSymbol.IsReflectionMethodsInfoCompact()) ||
      ((idx == static_cast<uint32>(ClassRO::kClinitAddr)) && mirSymbol.IsReflectionClassInfoRO())) {
    Emit("\t.long\t");
    Emit(funcName + " - .\n");
    return;
  }

  if (mirSymbol.IsReflectionMethodAddrData()) {
#ifdef USE_32BIT_REF
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    Emit(funcName + " - .\n");
    return;
  }

  if (idx == kFuncDefAddrIndex && mirSymbol.IsMuidFuncDefTab()) {
#if defined(USE_32BIT_REF)
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
      /*
       * Check enum BindingState defined in Mpl_Binding.h,
       * 6 means kBindingStateMethodDef:6 offset away from base __BindingProtectRegion__.
       */
#if defined(USE_32BIT_REF)
      Emit("0x6\n"); /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateMethodDef:6. */
#else
      Emit("__BindingProtectRegion__ + 6\n");
#endif  /* USE_32BIT_REF */
    } else {
#if defined(USE_32BIT_REF)
#if defined(MPL_LNK_ADDRESS_VIA_BASE)
      Emit(funcName + "\n");
#else  /* MPL_LNK_ADDRESS_VIA_BASE */
      Emit(funcName + "-.\n");
#endif /* MPL_LNK_ADDRESS_VIA_BASE */
#else  /* USE_32BIT_REF */
      Emit(funcName + "\n");
#endif /* USE_32BIT_REF */
    }
    return;
  }

  if (idx == kFuncDefAddrIndex && mirSymbol.IsMuidFuncDefOrigTab()) {
    if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
#if defined(USE_32BIT_REF)
      Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
      Emit("\t.quad\t");
#else
      Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
#if defined(USE_32BIT_REF)
#if defined(MPL_LNK_ADDRESS_VIA_BASE)
      Emit(funcName + "\n");
#else /* MPL_LNK_ADDRESS_VIA_BASE */
      Emit(funcName + "-.\n");
#endif /* MPL_LNK_ADDRESS_VIA_BASE */
#else /* USE_32BIT_REF */
      Emit(funcName + "\n");
#endif /* USE_32BIT_REF */
    }
    return;
  }

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif
  Emit(funcName);
  if ((stName.find(VTAB_PREFIX_STR) == 0) || (stName.find(ITAB_PREFIX_STR) == 0) ||
      (stName.find(ITAB_CONFLICT_PREFIX_STR) == 0)) {
    Emit(" - .\n");
    return;
  }
  if (cg->GetCGOptions().GeneratePositionIndependentExecutable()) {
    Emit(" - ");
    Emit(stName);
  }
  Emit("\n");
}

void Emitter::EmitAddrofSymbolConst(const MIRSymbol &mirSymbol, MIRConst &elemConst, size_t idx) {
  MIRAddrofConst &symAddr = static_cast<MIRAddrofConst&>(elemConst);
  const std::string stName = mirSymbol.GetName();

  MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(symAddr.GetSymbolIndex().Idx());
  const std::string &symAddrName = symAddrSym->GetName();

  if (((idx == static_cast<uint32>(FieldProperty::kPOffset)) && mirSymbol.IsReflectionFieldsInfo()) ||
      mirSymbol.IsReflectionFieldOffsetData()) {
#if USE_32BIT_REF
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    Emit(symAddrName + " - .\n");
    return;
  }

  if (((idx == static_cast<uint32>(FieldPropertyCompact::kPOffset)) && mirSymbol.IsReflectionFieldsInfoCompact())  ||
      ((idx == static_cast<uint32>(MethodProperty::kSigName)) && mirSymbol.IsReflectionMethodsInfo()) ||
      ((idx == static_cast<uint32>(MethodSignatureProperty::kParameterTypes)) &&
      mirSymbol.IsReflectionMethodSignature())) {
    Emit("\t.long\t");
    Emit(symAddrName + " - .\n");
    return;
  }

  if (((idx == static_cast<uint32>(MethodProperty::kDeclarclass)) ||
      (idx == static_cast<uint32>(MethodProperty::kPaddrData))) && mirSymbol.IsReflectionMethodsInfo()) {
#if USE_32BIT_REF
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    if (idx == static_cast<uint32>(MethodProperty::kDeclarclass)) {
      Emit(symAddrName + " - .\n");
    } else {
      Emit(symAddrName + " - . + 2\n");
    }
    return;
  }

  if ((idx == static_cast<uint32>(MethodInfoCompact::kPaddrData)) && mirSymbol.IsReflectionMethodsInfoCompact()) {
    Emit("\t.long\t");
    Emit(symAddrName + " - . + 2\n");
    return;
  }

  if ((idx == static_cast<uint32>(FieldProperty::kDeclarclass)) && mirSymbol.IsReflectionFieldsInfo()) {
#if USE_32BIT_REF
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    Emit(symAddrName + " - .\n");
    return;
  }

  if ((idx == kDataDefAddrIndex) && (mirSymbol.IsMuidDataUndefTab() || mirSymbol.IsMuidDataDefTab())) {
    if (symAddrSym->IsReflectionClassInfo()) {
      Emit(".LDW.ref." + symAddrName + ":\n");
    }
    Emit(kPtrPrefixStr + symAddrName + ":\n");
#if defined(USE_32BIT_REF)
    Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */
    if (mirSymbol.IsMuidDataUndefTab()) {
      if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
        if (symAddrSym->IsReflectionClassInfo()) {
          /*
           * Check enum BindingState defined in Mpl_Binding.h,
           * 1 means kBindingStateCinfUndef:1 offset away from base __BindingProtectRegion__.
           */
#if defined(USE_32BIT_REF)
          Emit("0x1\n"); /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateCinfUndef:1. */
#else
          Emit("__BindingProtectRegion__ + 1\n");
#endif  /* USE_32BIT_REF */
        } else {
          /*
           * Check enum BindingState defined in Mpl_Binding.h,
           * 3 means kBindingStateDataUndef:3 offset away from base __BindingProtectRegion__.
           */
#if defined(USE_32BIT_REF)
          Emit("0x3\n"); /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateDataUndef:3. */
#else
          Emit("__BindingProtectRegion__ + 3\n");
#endif  /* USE_32BIT_REF */
        }
      } else {
        Emit("0\n");
      }
    } else {
      if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
        if (symAddrSym->IsReflectionClassInfo()) {
          /*
           * Check enum BindingState defined in Mpl_Binding.h,
           * 2 means kBindingStateCinfDef:2 offset away from base __BindingProtectRegion__.
           */
#if defined(USE_32BIT_REF)
          Emit("0x2\n"); /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateCinfDef:2. */
#else
          Emit("__BindingProtectRegion__ + 2\n");
#endif  /* USE_32BIT_REF */
        } else {
          /*
           * Check enum BindingState defined in Mpl_Binding.h,
           * 4 means kBindingStateDataDef:4 offset away from base __BindingProtectRegion__.
           */
#if defined(USE_32BIT_REF)
          Emit("0x4\n"); /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateDataDef:4. */
#else
          Emit("__BindingProtectRegion__ + 4\n");
#endif  /* USE_32BIT_REF */
        }
      } else {
#if defined(USE_32BIT_REF)
#if defined(MPL_LNK_ADDRESS_VIA_BASE)
        Emit(symAddrName + "\n");
#else /* MPL_LNK_ADDRESS_VIA_BASE */
        Emit(symAddrName + "-.\n");
#endif /* MPL_LNK_ADDRESS_VIA_BASE */
#else /* USE_32BIT_REF */
        Emit(symAddrName + "\n");
#endif /* USE_32BIT_REF */
      }
    }
    return;
  }

  if (idx == kDataDefAddrIndex && mirSymbol.IsMuidDataDefOrigTab()) {
    if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
#if defined(USE_32BIT_REF)
      Emit("\t.long\t");
#else

#if TARGAARCH64 || TARGRISCV64
      Emit("\t.quad\t");
#else
      Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */

#if defined(USE_32BIT_REF)
#if defined(MPL_LNK_ADDRESS_VIA_BASE)
      Emit(symAddrName + "\n");
#else /* MPL_LNK_ADDRESS_VIA_BASE */
      Emit(symAddrName + "-.\n");
#endif /* MPL_LNK_ADDRESS_VIA_BASE */
#else /* USE_32BIT_REF */
      Emit(symAddrName + "\n");
#endif /* USE_32BIT_REF */
    }
    return;
  }

  if (StringUtils::StartsWith(stName, kLocalClassInfoStr)) {
#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif
    Emit(symAddrName);
    Emit(" - . + ").Emit(kDataRefIsOffset);
    Emit("\n");
    return;
  }
#ifdef USE_32BIT_REF
  if (mirSymbol.IsReflectionHashTabBucket() || (stName.find(ITAB_PREFIX_STR) == 0) ||
      (mirSymbol.IsReflectionClassInfo() && (idx == static_cast<uint32>(ClassProperty::kInfoRo)))) {
    Emit("\t.word\t");
  } else {
#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif
  }
#else

#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif

#endif  /* USE_32BIT_REF */

  if ((stName.find(ITAB_CONFLICT_PREFIX_STR) == 0) || (stName.find(ITAB_PREFIX_STR) == 0)) {
    Emit(symAddrName + " - .\n");
    return;
  }
  if (mirSymbol.IsMuidRangeTab()) {
    if (idx == kRangeBeginIndex) {
      Emit(symAddrSym->GetMuidTabName() + "_begin\n");
    } else {
      Emit(symAddrSym->GetMuidTabName() + "_end\n");
    }
    return;
  }

  if (symAddrName.find(GCTIB_PREFIX_STR) == 0) {
    Emit(cg->FindGCTIBPatternName(symAddrName));
  } else {
    Emit(symAddrName);
  }

  if ((((idx == static_cast<uint32>(ClassRO::kIfields)) || (idx == static_cast<uint32>(ClassRO::kMethods))) &&
       mirSymbol.IsReflectionClassInfoRO()) ||
       mirSymbol.IsReflectionHashTabBucket()) {
    Emit(" - .");
    if (symAddrSym->IsReflectionFieldsInfoCompact() ||
        symAddrSym->IsReflectionMethodsInfoCompact()) {
      /* Mark the least significant bit as 1 for compact fieldinfo */
      Emit(" + ").Emit(MethodFieldRef::kMethodFieldRefIsCompact);
    }
  } else if (mirSymbol.IsReflectionClassInfo()) {
    if ((idx == static_cast<uint32>(ClassProperty::kItab)) ||
        (idx == static_cast<uint32>(ClassProperty::kVtab)) ||
        (idx == static_cast<uint32>(ClassProperty::kInfoRo))) {
      Emit(" - . + ").Emit(kDataRefIsOffset);
    } else if (idx == static_cast<uint32>(ClassProperty::kGctib)) {
      if (cg->FindGCTIBPatternName(symAddrName).find(REF_PREFIX_STR) == 0) {
        Emit(" - . + ").Emit(kGctibRefIsIndirect);
      } else {
        Emit(" - .");
      }
    }
  } else if (mirSymbol.IsReflectionClassInfoRO()) {
    if (idx == static_cast<uint32>(ClassRO::kSuperclass)) {
      Emit(" - . + ").Emit(kDataRefIsOffset);
    }
  }

  if (cg->GetCGOptions().GeneratePositionIndependentExecutable()) {
    Emit(" - ");
    Emit(stName);
  }
  Emit("\n");
}

MIRAddroffuncConst *Emitter::GetAddroffuncConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst) {
  MIRAddroffuncConst *innerFuncAddr = nullptr;
  size_t addrIndex = mirSymbol.IsReflectionMethodsInfo() ? static_cast<size_t>(MethodProperty::kPaddrData) :
                                                           static_cast<size_t>(MethodInfoCompact::kPaddrData);
  MIRConst *pAddrConst = aggConst.GetConstVecItem(addrIndex);
  if (pAddrConst->GetKind() == kConstAddrof) {
    /* point addr data. */
    MIRAddrofConst *pAddr = safe_cast<MIRAddrofConst>(pAddrConst);
    MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(pAddr->GetSymbolIndex().Idx());
    MIRAggConst *methodAddrAggConst = safe_cast<MIRAggConst>(symAddrSym->GetKonst());
    MIRAggConst *addrAggConst = safe_cast<MIRAggConst>(methodAddrAggConst->GetConstVecItem(0));
    MIRConst *funcAddrConst = addrAggConst->GetConstVecItem(0);
    if (funcAddrConst->GetKind() == kConstAddrofFunc) {
      /* func sybmol. */
      innerFuncAddr = safe_cast<MIRAddroffuncConst>(funcAddrConst);
    } else if (funcAddrConst->GetKind() == kConstInt) {
      /* def table index, replaced by def table for lazybinding. */
      std::string funcDefTabName = namemangler::kMuidFuncDefTabPrefixStr + cg->GetMIRModule()->GetFileNameAsPostfix();
      MIRSymbol *funDefTabSy = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
          GlobalTables::GetStrTable().GetStrIdxFromName(funcDefTabName));
      MIRAggConst &funDefTabAggConst = static_cast<MIRAggConst&>(*funDefTabSy->GetKonst());
      MIRIntConst *funcAddrIndexConst = safe_cast<MIRIntConst>(funcAddrConst);
      int64 indexDefTab = funcAddrIndexConst->GetValue();
      MIRAggConst *defTabAggConst = safe_cast<MIRAggConst>(funDefTabAggConst.GetConstVecItem(indexDefTab));
      MIRConst *funcConst = defTabAggConst->GetConstVecItem(0);
      if (funcConst->GetKind() == kConstAddrofFunc) {
        innerFuncAddr = safe_cast<MIRAddroffuncConst>(funcConst);
      }
    }
  } else if (pAddrConst->GetKind() == kConstAddrofFunc) {
    innerFuncAddr = safe_cast<MIRAddroffuncConst>(pAddrConst);
  }
  return innerFuncAddr;
}

int64 Emitter::GetFieldOffsetValue(const std::string &className, const MIRIntConst &intConst,
                                   const std::map<GStrIdx, MIRType*> &strIdx2Type) {
  int64 idx = intConst.GetValue();
  bool isDefTabIndex = static_cast<uint64>(idx) & 0x1;
  int64 fieldIdx = static_cast<uint64>(idx) >> 1;
  if (isDefTabIndex) {
    /* it's def table index. */
    return fieldIdx;
  } else {
    /* really offset. */
    uint8 charBitWidth = GetPrimTypeSize(PTY_i8) * kBitsPerByte;
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(className);
    auto it = strIdx2Type.find(strIdx);
    CHECK_FATAL(it->second != nullptr, "valid iterator check");
    ASSERT(it != strIdx2Type.end(), "Can not find type");
    MIRType &ty = *it->second;
    MIRStructType &structType = static_cast<MIRStructType&>(ty);
    std::pair<int32, int32> fieldOffsetPair =
        Globals::GetInstance()->GetBECommon()->GetFieldOffset(structType, fieldIdx);
    int64 fieldOffset = fieldOffsetPair.first * static_cast<int64>(charBitWidth) + fieldOffsetPair.second;
    return fieldOffset;
  }
}

void Emitter::InitRangeIdx2PerfixStr() {
  rangeIdx2PrefixStr[RangeIdx::kVtabAndItab] = kMuidVtabAndItabPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kItabConflict] = kMuidItabConflictPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kVtabOffset] = kMuidVtabOffsetPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kFieldOffset] = kMuidFieldOffsetPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kValueOffset] = kMuidValueOffsetPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kLocalClassInfo] = kMuidLocalClassInfoStr;
  rangeIdx2PrefixStr[RangeIdx::kConststr] = kMuidConststrPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kSuperclass] = kMuidSuperclassPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kGlobalRootlist] = kMuidGlobalRootlistPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kClassmetaData] = kMuidClassMetadataPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kClassBucket] = kMuidClassMetadataBucketPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kJavatext] = kMuidJavatextPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kDataSection] = kMuidDataSectionStr;
  rangeIdx2PrefixStr[RangeIdx::kJavajni] = kRegJNITabPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kJavajniFunc] = kRegJNIFuncTabPrefixStr;
  rangeIdx2PrefixStr[RangeIdx::kDecoupleStaticKey] = kDecoupleStaticKeyStr;
  rangeIdx2PrefixStr[RangeIdx::kDecoupleStaticValue] = kDecoupleStaticValueStr;
  rangeIdx2PrefixStr[RangeIdx::kBssStart] = kBssSectionStr;
  rangeIdx2PrefixStr[RangeIdx::kLinkerSoHash] = kLinkerHashSoStr;
  rangeIdx2PrefixStr[RangeIdx::kArrayClassCache] = kArrayClassCacheTable;
  rangeIdx2PrefixStr[RangeIdx::kArrayClassCacheName] = kArrayClassCacheNameTable;
}

void Emitter::EmitIntConst(const MIRSymbol &mirSymbol, MIRAggConst &aggConst, uint32 itabConflictIndex,
                           const std::map<GStrIdx, MIRType*> &strIdx2Type, size_t idx) {
  MIRConst *elemConst = aggConst.GetConstVecItem(idx);
  const std::string stName = mirSymbol.GetName();

  MIRIntConst *intConst = safe_cast<MIRIntConst>(elemConst);
  ASSERT(intConst != nullptr, "Uexpected const type");

  /* ignore abstract function addr */
  if ((idx == static_cast<uint32>(MethodInfoCompact::kPaddrData)) && mirSymbol.IsReflectionMethodsInfoCompact()) {
    return;
  }

  if (((idx == static_cast<uint32>(MethodProperty::kVtabIndex)) && (mirSymbol.IsReflectionMethodsInfo())) ||
      ((idx == static_cast<uint32>(MethodInfoCompact::kVtabIndex)) && mirSymbol.IsReflectionMethodsInfoCompact())) {
    MIRAddroffuncConst *innerFuncAddr = GetAddroffuncConst(mirSymbol, aggConst);
    if (innerFuncAddr != nullptr) {
      Emit(".Label.name." + GlobalTables::GetFunctionTable().GetFunctionFromPuidx(
          innerFuncAddr->GetValue())->GetName());
      Emit(":\n");
    }
  }
  /* refer to DeCouple::GenOffsetTableType */
  constexpr int fieldTypeIdx = 2;
  constexpr int methodTypeIdx = 2;
  bool isClassInfo = (idx == static_cast<uint32>(ClassRO::kClassName) ||
                      idx == static_cast<uint32>(ClassRO::kAnnotation)) && mirSymbol.IsReflectionClassInfoRO();
  bool isMethodsInfo = (idx == static_cast<uint32>(MethodProperty::kMethodName) ||
                        idx == static_cast<uint32>(MethodProperty::kSigName) ||
                        idx == static_cast<uint32>(MethodProperty::kAnnotation)) && mirSymbol.IsReflectionMethodsInfo();
  bool isFieldsInfo = (idx == static_cast<uint32>(FieldProperty::kTypeName) ||
                       idx == static_cast<uint32>(FieldProperty::kName) ||
                       idx == static_cast<uint32>(FieldProperty::kAnnotation)) && mirSymbol.IsReflectionFieldsInfo();
  bool isMethodSignature = (idx == static_cast<uint32>(MethodSignatureProperty::kSignatureOffset)) &&
                            mirSymbol.IsReflectionMethodSignature();
  /* RegisterTable has been Int Array, visit element instead of field. */
  bool isInOffsetTab = (idx == 1 || idx == methodTypeIdx) &&
                       (StringUtils::StartsWith(stName, kVtabOffsetTabStr) ||
                        StringUtils::StartsWith(stName, kFieldOffsetTabStr));
  /* The 1 && 2 of Decouple static struct is the string name */
  bool isStaticStr = (idx == 1 || idx == 2) && aggConst.GetConstVec().size() == kSizeOfDecoupleStaticStruct &&
                     StringUtils::StartsWith(stName, kDecoupleStaticKeyStr);
  /* process conflict table index larger than itabConflictIndex * 2 + 2 element */
  bool isConflictPerfix = (idx >= (static_cast<uint64>(itabConflictIndex) * 2 + 2)) && (idx % 2 == 0) &&
                          StringUtils::StartsWith(stName, ITAB_CONFLICT_PREFIX_STR);
  bool isArrayClassCacheName = mirSymbol.IsArrayClassCacheName();
  if (isClassInfo || isMethodsInfo || isFieldsInfo || mirSymbol.IsRegJNITab() || isInOffsetTab ||
      isStaticStr || isConflictPerfix || isArrayClassCacheName || isMethodSignature) {
    /* compare with all 1s */
    uint32 index = static_cast<uint32>((safe_cast<MIRIntConst>(elemConst))->GetValue()) & 0xFFFFFFFF;
    bool isHotReflectStr = (index & 0x00000003) != 0;     /* use the last two bits of index in this expression */
    std::string hotStr;
    if (isHotReflectStr) {
      uint32 tag = (index & 0x00000003) - kCStringShift;  /* use the last two bits of index in this expression */
      if (tag == kLayoutBootHot) {
        hotStr = kReflectionStartHotStrtabPrefixStr;
      } else if (tag == kLayoutBothHot) {
        hotStr = kReflectionBothHotStrTabPrefixStr;
      } else {
        hotStr = kReflectionRunHotStrtabPrefixStr;
      }
    }
    std::string reflectStrTabPrefix = isHotReflectStr ? hotStr : kReflectionStrtabPrefixStr;
    std::string strTabName = reflectStrTabPrefix + cg->GetMIRModule()->GetFileNameAsPostfix();
    /* left shift 2 bit to get low 30 bit data for MIRIntConst */
    elemConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(index >> 2, elemConst->GetType());
    intConst = safe_cast<MIRIntConst>(elemConst);
    aggConst.SetItem(idx, intConst, aggConst.GetFieldIdItem(idx));
#ifdef USE_32BIT_REF
    if (stName.find(ITAB_CONFLICT_PREFIX_STR) == 0) {
      EmitScalarConstant(*elemConst, false, true);
    } else {
      EmitScalarConstant(*elemConst, false);
    }
#else
    EmitScalarConstant(*elemConst, false);
#endif  /* USE_32BIT_REF */
    Emit("+" + strTabName);
    if (mirSymbol.IsRegJNITab() || mirSymbol.IsReflectionMethodsInfo() || mirSymbol.IsReflectionFieldsInfo() ||
        mirSymbol.IsArrayClassCacheName() || mirSymbol.IsReflectionMethodSignature()) {
      Emit("-.");
    }
    if (StringUtils::StartsWith(stName, kDecoupleStaticKeyStr)) {
      Emit("-.");
    }
    if (mirSymbol.IsReflectionClassInfoRO()) {
      if (idx == static_cast<uint32>(ClassRO::kAnnotation)) {
        Emit("-.");
      } else if (idx == static_cast<uint32>(ClassRO::kClassName)) {
        /* output in hex format to show it is a flag of bits. */
        std::stringstream ss;
        ss << std::hex << "0x" << MByteRef::kPositiveOffsetBias;
        Emit(" - . + " + ss.str());
      }
    }
    if (StringUtils::StartsWith(stName, ITAB_PREFIX_STR)) {
      Emit("-.");
    }
    if (StringUtils::StartsWith(stName, ITAB_CONFLICT_PREFIX_STR)) {
      /* output in hex format to show it is a flag of bits. */
      std::stringstream ss;
      ss << std::hex << "0x" << MByteRef32::kPositiveOffsetBias;
      Emit(" - . + " + ss.str());
    }
    if ((idx == 1 || idx == methodTypeIdx) && StringUtils::StartsWith(stName, kVtabOffsetTabStr)) {
      Emit("-.");
    }
    if ((idx == 1 || idx == fieldTypeIdx) && StringUtils::StartsWith(stName, kFieldOffsetTabStr)) {
      Emit("-.");
    }
    Emit("\n");
  } else if (idx == kFuncDefAddrIndex && mirSymbol.IsMuidFuncUndefTab()) {
#if defined(USE_32BIT_REF)
    Emit("\t.long\t");
#else
    Emit("\t.quad\t");
#endif  /* USE_32BIT_REF */
    if (CGOptions::IsLazyBinding() && !cg->IsLibcore()) {
      /*
       * Check enum BindingState defined in Mpl_Binding.h,
       * 5 means kBindingStateMethodUndef:5 offset away from base __BindingProtectRegion__.
       */
#if defined(USE_32BIT_REF)
      Emit("0x5\n");  /* Fix it in runtime, __BindingProtectRegion__ + kBindingStateMethodUndef:5. */
#else
      Emit("__BindingProtectRegion__ + 5\n");
#endif  /* USE_32BIT_REF */
    } else {
      Emit("0\n");
    }
  } else if (idx == static_cast<uint32>(FieldProperty::kPClassType) && mirSymbol.IsReflectionFieldsInfo()) {
#ifdef USE_32BIT_REF
    Emit("\t.long\t");
    const int width = 4;
#else
    Emit("\t.quad\t");
    const int width = 8;
#endif  /* USE_32BIT_REF */
    uint32 muidDataTabAddr = static_cast<uint32>((safe_cast<MIRIntConst>(elemConst))->GetValue());
    if (muidDataTabAddr != 0) {
      bool isDefTabIndex = (muidDataTabAddr & kFromDefIndexMask32Mod) == kFromDefIndexMask32Mod;
      std::string muidDataTabPrefix = isDefTabIndex ? kMuidDataDefTabPrefixStr : kMuidDataUndefTabPrefixStr;
      std::string muidDataTabName = muidDataTabPrefix + cg->GetMIRModule()->GetFileNameAsPostfix();
      (void)Emit(muidDataTabName + "+");
      uint32 muidDataTabIndex = muidDataTabAddr & 0x3FFFFFFF; /* high 2 bit is the mask of muid tab */
      (void)Emit(std::to_string(muidDataTabIndex * width));
      (void)Emit("-.\n");
    } else {
      (void)Emit(muidDataTabAddr);
      Emit("\n");
    }
    return;
  } else if (mirSymbol.IsRegJNIFuncTab()) {
    std::string strTabName = kRegJNITabPrefixStr + cg->GetMIRModule()->GetFileNameAsPostfix();
    EmitScalarConstant(*elemConst, false);
#ifdef TARGARM32
    (void)Emit("+" + strTabName).Emit("+").Emit(MByteRef::kPositiveOffsetBias).Emit("-.\n");
#else
    Emit("+" + strTabName + "\n");
#endif
  } else if (mirSymbol.IsReflectionMethodAddrData()) {
    int64 defTabIndex = intConst->GetValue();
#ifdef USE_32BIT_REF
    Emit("\t.long\t");
#else
    Emit("\t.quad\t");
#endif  /* USE_32BIT_REF */
    Emit(std::to_string(defTabIndex) + "\n");
  } else if (mirSymbol.IsReflectionFieldOffsetData()) {
    /* Figure out instance field offset now. */
    size_t prefixStrLen = strlen(kFieldOffsetDataPrefixStr);
    size_t pos = stName.find("_FieldID_");
    std::string typeName = stName.substr(prefixStrLen, pos - prefixStrLen);
#ifdef USE_32BIT_REF
    std::string widthFlag = ".long";
#else
    std::string widthFlag = ".quad";
#endif  /* USE_32BIT_REF */
    int64 fieldOffset = GetFieldOffsetValue(typeName, *intConst, strIdx2Type);
    int64 fieldIdx = intConst->GetValue();
    bool isDefTabIndex = static_cast<uint64>(fieldIdx) & 0x1;
    if (isDefTabIndex) {
      /* it's def table index. */
      Emit("\t//  " + typeName + " static field, data def table index " + std::to_string(fieldOffset) + "\n");
    } else {
      /* really offset. */
      fieldIdx = static_cast<uint64>(fieldIdx) >> 1;
      Emit("\t//  " + typeName + "\t field" + std::to_string(fieldIdx) + "\n");
    }
    Emit("\t" + widthFlag + "\t" + std::to_string(fieldOffset) + "\n");
  } else if (((idx == static_cast<uint32>(FieldProperty::kPOffset)) && mirSymbol.IsReflectionFieldsInfo()) ||
             ((idx == static_cast<uint32>(FieldPropertyCompact::kPOffset)) &&
              mirSymbol.IsReflectionFieldsInfoCompact())) {
    std::string typeName;
    std::string widthFlag;
#ifdef USE_32BIT_REF
    const int width = 4;
#else
    const int width = 8;
#endif  /* USE_32BIT_REF */
    if (mirSymbol.IsReflectionFieldsInfo()) {
      typeName = stName.substr(strlen(kFieldsInfoPrefixStr));
#ifdef USE_32BIT_REF
      widthFlag = ".long";
#else
      widthFlag = ".quad";
#endif  /* USE_32BIT_REF */
    } else {
      size_t prefixStrLen = strlen(kFieldsInfoCompactPrefixStr);
      typeName = stName.substr(prefixStrLen);
      widthFlag = ".long";
    }
    int64 fieldIdx = intConst->GetValue();
    MIRSymbol *pOffsetData = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kFieldOffsetDataPrefixStr + typeName));
    if (pOffsetData != nullptr) {
      fieldIdx *= width;
      std::string fieldOffset = kFieldOffsetDataPrefixStr + typeName;
      Emit("\t" + widthFlag + "\t" + std::to_string(fieldIdx) + " + " + fieldOffset + " - .\n");
    } else {
      /* pOffsetData null, means FieldMeta.offset is really offset */
      int64 fieldOffset = GetFieldOffsetValue(typeName, *intConst, strIdx2Type);
      Emit("\t//  " + typeName + "\t field" + std::to_string(fieldIdx) + "\n");
      Emit("\t" + widthFlag + "\t" + std::to_string(fieldOffset) + "\n");
    }
  } else if ((idx == static_cast<uint32>(ClassProperty::kObjsize)) && mirSymbol.IsReflectionClassInfo()) {
    std::string delimiter = "$$";
    std::string typeName =
        stName.substr(strlen(CLASSINFO_PREFIX_STR), stName.find(delimiter) - strlen(CLASSINFO_PREFIX_STR));
    uint32 objSize = 0;
    std::string comments;

    if (typeName.size() > 1 && typeName[0] == '$') {
      /* fill element size for array class; */
      std::string newTypeName = typeName.substr(1);
      /* another $(arraysplitter) */
      if (newTypeName.find("$") == std::string::npos) {
        CHECK_FATAL(false, "can not find $ in std::string");
      }
      typeName = newTypeName.substr(newTypeName.find("$") + 1);
      int32 pTypeSize;

      /* we only need to calculate primitive type in arrays. */
      if ((pTypeSize = GetPrimitiveTypeSize(typeName)) != -1) {
        objSize = static_cast<uint32>(pTypeSize);
      }
      comments = "// elemobjsize";
    } else {
      comments = "// objsize";
    }

    if (!objSize) {
      GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
      auto it = strIdx2Type.find(strIdx);
      ASSERT(it != strIdx2Type.end(), "Can not find type");
      MIRType *mirType = it->second;

      objSize = Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex());
    }
    /* objSize should not exceed 16 bits */
    CHECK_FATAL(objSize <= 0xffff, "Error:the objSize is too large");
    Emit("\t.short\t" + std::to_string(objSize) + comments + "\n");
  } else if (mirSymbol.IsMuidRangeTab()) {
    MIRIntConst *subIntCt = safe_cast<MIRIntConst>(elemConst);
    int flag = subIntCt->GetValue();
    InitRangeIdx2PerfixStr();
    if (rangeIdx2PrefixStr.find(flag) == rangeIdx2PrefixStr.end()) {
      EmitScalarConstant(*elemConst, false);
      Emit("\n");
      return;
    }
    std::string prefix = rangeIdx2PrefixStr[flag];
#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad\t");
#else
    Emit("\t.word\t");
#endif
    if (idx == kRangeBeginIndex) {
      Emit(prefix + "_begin\n");
    } else {
      Emit(prefix + "_end\n");
    }
  } else {
#ifdef USE_32BIT_REF
    if (StringUtils::StartsWith(stName, ITAB_CONFLICT_PREFIX_STR) || StringUtils::StartsWith(stName, ITAB_PREFIX_STR) ||
        StringUtils::StartsWith(stName, VTAB_PREFIX_STR)) {
      EmitScalarConstant(*elemConst, false, true);
    } else {
      EmitScalarConstant(*elemConst, false);
    }
#else
    EmitScalarConstant(*elemConst, false);
#endif  /* USE_32BIT_REF */
    Emit("\n");
  }
}

void Emitter::EmitConstantTable(const MIRSymbol &mirSymbol, MIRConst &mirConst,
                                const std::map<GStrIdx, MIRType*> &strIdx2Type) {
  const std::string stName = mirSymbol.GetName();
  MIRAggConst &aggConst = static_cast<MIRAggConst&>(mirConst);
  uint32 itabConflictIndex = 0;
  for (size_t i = 0; i < aggConst.GetConstVec().size(); ++i) {
    MIRConst *elemConst = aggConst.GetConstVecItem(i);
    if (i == 0 && StringUtils::StartsWith(stName, ITAB_CONFLICT_PREFIX_STR)) {
#ifdef USE_32BIT_REF
      itabConflictIndex = static_cast<uint64>((safe_cast<MIRIntConst>(elemConst))->GetValue()) & 0xffff;
#else
      itabConflictIndex = static_cast<uint64>((safe_cast<MIRIntConst>(elemConst))->GetValue()) & 0xffffffff;
#endif
    }
    if (IsPrimitiveScalar(elemConst->GetType().GetPrimType())) {
      if (elemConst->GetKind() == kConstAddrofFunc) {     /* addroffunc const */
        EmitAddrofFuncConst(mirSymbol, *elemConst, i);
      } else if (elemConst->GetKind() == kConstAddrof) {  /* addrof symbol const */
        EmitAddrofSymbolConst(mirSymbol, *elemConst, i);
      } else {                                            /* intconst */
        EmitIntConst(mirSymbol, aggConst, itabConflictIndex, strIdx2Type, i);
      }
    } else if (elemConst->GetType().GetKind() == kTypeArray || elemConst->GetType().GetKind() == kTypeStruct) {
      if (StringUtils::StartsWith(mirSymbol.GetName(), namemangler::kOffsetTabStr) && (i == 0 || i == 1)) {
        /* EmitOffsetValueTable */
#ifdef USE_32BIT_REF
        Emit("\t.long\t");
#else
        Emit("\t.quad\t");
#endif
        if (i == 0) {
          (void)Emit(namemangler::kVtabOffsetTabStr + cg->GetMIRModule()->GetFileNameAsPostfix() + " - .\n");
        } else {
          (void)Emit(namemangler::kFieldOffsetTabStr + cg->GetMIRModule()->GetFileNameAsPostfix() + " - .\n");
        }
      } else {
        EmitConstantTable(mirSymbol, *elemConst, strIdx2Type);
      }
    }
  }
}

void Emitter::EmitArrayConstant(MIRConst &mirConst) {
  MIRType &mirType = mirConst.GetType();
  MIRAggConst &arrayCt = static_cast<MIRAggConst&>(mirConst);
  MIRArrayType &arrayType = static_cast<MIRArrayType&>(mirType);
  size_t uNum = arrayCt.GetConstVec().size();
  uint32 dim = arrayType.GetSizeArrayItem(0);
  TyIdx scalarIdx = arrayType.GetElemTyIdx();
  if (uNum == 0 && dim) {
    MIRType *subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(scalarIdx);
    while (subTy->GetKind() == kTypeArray) {
      MIRArrayType *aSubTy = static_cast<MIRArrayType *>(subTy);
      if (aSubTy->GetSizeArrayItem(0) > 0) {
        dim *= (aSubTy->GetSizeArrayItem(0));
      }
      scalarIdx = aSubTy->GetElemTyIdx();
      subTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(scalarIdx);
    }
  }
  for (size_t i = 0; i < uNum; ++i) {
    MIRConst *elemConst = arrayCt.GetConstVecItem(i);
    if (IsPrimitiveScalar(elemConst->GetType().GetPrimType())) {
      if (cg->GetMIRModule()->IsCModule()) {
        bool strLiteral = false;
        if (arrayType.GetDim() == 1) {
          MIRType *ety = arrayType.GetElemType();
          if (ety->GetPrimType() == PTY_i8 || ety->GetPrimType() == PTY_u8) {
            strLiteral = true;
          }
        }
        EmitScalarConstant(*elemConst, true, false, strLiteral == false);
      } else {
        EmitScalarConstant(*elemConst);
      }
    } else if (elemConst->GetType().GetKind() == kTypeArray) {
      EmitArrayConstant(*elemConst);
    } else if (elemConst->GetType().GetKind() == kTypeStruct || elemConst->GetType().GetKind() == kTypeClass ||
               elemConst->GetType().GetKind() == kTypeUnion) {
      EmitStructConstant(*elemConst);
    } else {
      ASSERT(false, "should not run here");
    }
  }
  int64 iNum = (arrayType.GetSizeArrayItem(0) > 0) ? (static_cast<int64>(arrayType.GetSizeArrayItem(0))) - uNum : 0;
  if (iNum > 0) {
    if (!cg->GetMIRModule()->IsCModule()) {
      CHECK_FATAL(!Globals::GetInstance()->GetBECommon()->IsEmptyOfTypeSizeTable(), "container empty check");
      CHECK_FATAL(!arrayCt.GetConstVec().empty(), "container empty check");
    }
    if (uNum > 0) {
      uint64 unInSizeInByte = static_cast<uint64>(iNum) * static_cast<uint64>(
          Globals::GetInstance()->GetBECommon()->GetTypeSize(arrayCt.GetConstVecItem(0)->GetType().GetTypeIndex()));
      if (unInSizeInByte != 0) {
        EmitNullConstant(unInSizeInByte);
      }
    } else {
      uint64 size = Globals::GetInstance()->GetBECommon()->GetTypeSize(scalarIdx.GetIdx()) * dim;
      Emit("\t.zero\t").Emit(static_cast<int64>(size)).Emit("\n");
    }
  }
  Emit("\n");
}

void Emitter::EmitStructConstant(MIRConst &mirConst) {
  StructEmitInfo *sEmitInfo = cg->GetMIRModule()->GetMemPool()->New<StructEmitInfo>();
  CHECK_FATAL(sEmitInfo != nullptr, "create a new struct emit info failed in Emitter::EmitStructConstant");
  MIRType &mirType = mirConst.GetType();
  MIRAggConst &structCt = static_cast<MIRAggConst&>(mirConst);
  MIRStructType &structType = static_cast<MIRStructType&>(mirType);
  /* all elements of struct. */
  uint8 num;
  if (structType.GetKind() == kTypeUnion) {
    num = 1;
  } else {
    num = static_cast<uint8>(structType.GetFieldsSize());
  }
  /* total size of emitted elements size. */
  uint32 size = Globals::GetInstance()->GetBECommon()->GetTypeSize(structType.GetTypeIndex());
  uint32 fieldIdx = 1;
  if (structType.GetKind() == kTypeUnion) {
    fieldIdx = structCt.GetFieldIdItem(0);
  }
  for (uint32 i = 0; i < num; ++i) {
    if (((i + 1) == num) && cg->GetMIRModule()->GetSrcLang() == kSrcLangC) {
      isFlexibleArray = Globals::GetInstance()->GetBECommon()->GetHasFlexibleArray(mirType.GetTypeIndex().GetIdx());
      arraySize = 0;
    }
    MIRConst *elemConst = structCt.GetAggConstElement(fieldIdx);
    MIRType *elemType = structType.GetElemType(i);
    if (structType.GetKind() == kTypeUnion) {
      elemType = &(elemConst->GetType());
    }
    MIRType *nextElemType = nullptr;
    if (i != static_cast<uint32>(num - 1)) {
      nextElemType = structType.GetElemType(i + 1);
    }
    uint32 elemSize = Globals::GetInstance()->GetBECommon()->GetTypeSize(elemType->GetTypeIndex());
    uint8 charBitWidth = GetPrimTypeSize(PTY_i8) * kBitsPerByte;
    if (elemType->GetKind() == kTypeBitField) {
      if (elemConst == nullptr) {
        MIRIntConst *zeroFill = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *elemType);
        elemConst = zeroFill;
      }
      uint64 fieldOffset =
          static_cast<uint64>(static_cast<int64>(
              Globals::GetInstance()->GetBECommon()->GetFieldOffset(structType, fieldIdx).first)) *
          static_cast<uint64>(charBitWidth) +
          static_cast<uint64>(static_cast<int64>(
              Globals::GetInstance()->GetBECommon()->GetFieldOffset(structType, fieldIdx).second));
      EmitBitFieldConstant(*sEmitInfo, *elemConst, nextElemType, fieldOffset);
    } else {
      if (elemConst != nullptr) {
        if (IsPrimitiveScalar(elemType->GetPrimType())) {
          EmitScalarConstant(*elemConst, true, false, true);
        } else if (elemType->GetKind() == kTypeArray) {
          if (elemType->GetSize() != 0) {
            EmitArrayConstant(*elemConst);
          }
        } else if ((elemType->GetKind() == kTypeStruct) || (elemType->GetKind() == kTypeClass) ||
                   (elemType->GetKind() == kTypeUnion)) {
          EmitStructConstant(*elemConst);
        } else {
          ASSERT(false, "should not run here");
        }
      } else {
        EmitNullConstant(elemSize);
      }
      sEmitInfo->IncreaseTotalSize(elemSize);
      sEmitInfo->SetNextFieldOffset(sEmitInfo->GetTotalSize() * charBitWidth);
    }

    if (nextElemType != nullptr && kTypeBitField != nextElemType->GetKind()) {
      ASSERT(i < static_cast<uint32>(num - 1), "NYI");
      uint32 nextAlign = Globals::GetInstance()->GetBECommon()->GetTypeAlign(nextElemType->GetTypeIndex());
      ASSERT(nextAlign != 0, "expect non-zero");
      /* append size, append 0 when align need. */
      uint64 totalSize = sEmitInfo->GetTotalSize();
      uint32 psize = (totalSize % nextAlign == 0) ? 0 : (nextAlign - (totalSize % nextAlign));
      if (psize != 0) {
        EmitNullConstant(psize);
        sEmitInfo->IncreaseTotalSize(psize);
        sEmitInfo->SetNextFieldOffset(sEmitInfo->GetTotalSize() * charBitWidth);
      }
      /* element is uninitialized, emit null constant. */
    }
    fieldIdx++;
  }

  uint32 opSize = size - sEmitInfo->GetTotalSize();
  if (opSize != 0) {
    EmitNullConstant(opSize);
  }
}

/* BlockMarker is for Debugging/Profiling */
void Emitter::EmitBlockMarker(const std::string &markerName, const std::string &sectionName,
                              bool withAddr, const std::string &addrName) {
  /*
   * .type $marker_name$, %object
   * .global $marker_name$
   * .data
   * .align 3
   * $marker_name$:
   * .quad 0xdeadbeefdeadbeef
   * .size $marker_name$, 8
   */
  Emit(asmInfo->GetType());
  Emit(markerName);
  Emit(", %object\n");
  if (CGOptions::IsEmitBlockMarker()) {  /* exposed as global symbol, for profiling */
    Emit(asmInfo->GetGlobal());
  } else {                               /* exposed as local symbol, for release. */
    Emit(asmInfo->GetLocal());
  }
  Emit(markerName);
  Emit("\n");

  if (!sectionName.empty()) {
    Emit("\t.section ." + sectionName);
    if (sectionName.find("ro") == 0) {
      Emit(",\"a\",%progbits\n");
    } else {
      Emit(",\"aw\",%progbits\n");
    }
  } else {
    EmitAsmLabel(kAsmData);
  }
  Emit(asmInfo->GetAlign());
  Emit("  3\n" + markerName + ":\n");
  if (withAddr) {
    Emit("\t.quad " + addrName + "\n");
  } else {
    Emit("\t.quad\t0xdeadbeefdeadbeef\n"); /* hexspeak in aarch64 represents crash or dead lock */
  }
  Emit(asmInfo->GetSize());
  Emit(markerName + ", 8\n");
}

void Emitter::EmitLiteral(const MIRSymbol &literal, const std::map<GStrIdx, MIRType*> &strIdx2Type) {
  /*
   * .type _C_STR_xxxx, %object
   * .local _C_STR_xxxx
   * .data
   * .align 3
   * _C_STR_xxxx:
   * .quad __cinf_Ljava_2Flang_2FString_3B
   * ....
   * .size _C_STR_xxxx, 40
   */
  if (literal.GetStorageClass() == kScUnused) {
    return;
  }
  EmitAsmLabel(literal, kAsmType);
  /* literal should always be fstatic and readonly? */
  EmitAsmLabel(literal, kAsmLocal);  /* alwasy fstatic */
  (void)Emit("\t.section\t." + std::string(kMapleLiteralString) + ",\"aw\", %progbits\n");
  EmitAsmLabel(literal, kAsmAlign);
  EmitAsmLabel(literal, kAsmSyname);
  /* literal is an array */
  MIRConst *mirConst = literal.GetKonst();
  CHECK_FATAL(mirConst != nullptr, "mirConst should not be nullptr in EmitLiteral");
  if (literal.HasAddrOfValues()) {
    EmitConstantTable(literal, *mirConst, strIdx2Type);
  } else {
    EmitArrayConstant(*mirConst);
  }
  EmitAsmLabel(literal, kAsmSize);
}

void Emitter::EmitFuncLayoutInfo(const MIRSymbol &layout) {
  /*
   * .type $marker_name$, %object
   * .global $marker_name$
   * .data
   * .align 3
   * $marker_name$:
   * .quad funcaddr
   * .size $marker_name$, 8
   */
  MIRConst *mirConst = layout.GetKonst();
  MIRAggConst *aggConst = safe_cast<MIRAggConst>(mirConst);
  ASSERT(aggConst != nullptr, "null ptr check");
  if (aggConst->GetConstVec().size() != static_cast<uint32>(LayoutType::kLayoutTypeCount)) {
    maple::LogInfo::MapleLogger(kLlErr) << "something wrong happen in funclayoutsym\t"
         << "constVec size\t" << aggConst->GetConstVec().size() << "\n";
    return;
  }
  for (size_t i = 0; i < static_cast<size_t>(LayoutType::kLayoutTypeCount); ++i) {
    std::string markerName = "__MBlock_" + GetLayoutTypeString(i) + "_func_start";
    CHECK_FATAL(aggConst->GetConstVecItem(i)->GetKind() == kConstAddrofFunc, "expect kConstAddrofFunc type");
    MIRAddroffuncConst *funcAddr = safe_cast<MIRAddroffuncConst>(aggConst->GetConstVecItem(i));
    ASSERT(funcAddr != nullptr, "null ptr check");
    Emit(asmInfo->GetType());
    Emit(markerName + ", %object\n");
    Emit(asmInfo->GetGlobal());
    Emit(markerName + "\n");
    EmitAsmLabel(kAsmData);
    Emit(asmInfo->GetAlign());
    Emit("  3\n" + markerName + ":\n");
#if TARGAARCH64 || TARGRISCV64
    Emit("\t.quad ");
#else
    Emit("\t.word ");
#endif
    Emit(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcAddr->GetValue())->GetName());
    Emit("\n");
    Emit(asmInfo->GetSize());
    Emit(markerName + ", 8\n");
  }
}

void Emitter::EmitStaticFields(const std::vector<MIRSymbol*> &fields) {
  for (auto *itSymbol : fields) {
    EmitAsmLabel(*itSymbol, kAsmType);
    /* literal should always be fstatic and readonly? */
    EmitAsmLabel(*itSymbol, kAsmLocal);  /* alwasy fstatic */
    EmitAsmLabel(kAsmData);
    EmitAsmLabel(*itSymbol, kAsmAlign);
    EmitAsmLabel(*itSymbol, kAsmSyname);
    /* literal is an array */
    MIRConst *mirConst = itSymbol->GetKonst();
    EmitArrayConstant(*mirConst);
  }
}

void Emitter::EmitLiterals(std::vector<std::pair<MIRSymbol*, bool>> &literals,
                           const std::map<GStrIdx, MIRType*> &strIdx2Type) {
  /*
   * load literals profile
   * currently only used here, so declare it as local
   */
  if (!cg->GetMIRModule()->GetProfile().GetLiteralProfileSize()) {
    for (const auto &literalPair : literals) {
      EmitLiteral(*(literalPair.first), strIdx2Type);
    }
    return;
  }
  /* emit hot literal start symbol */
  EmitBlockMarker("__MBlock_literal_hot_begin", "", false);
  /*
   * emit literals into .data section
   * emit literals in the profile first
   */
  for (auto &literalPair : literals) {
    if (cg->GetMIRModule()->GetProfile().CheckLiteralHot(literalPair.first->GetName())) {
      /* it's in the literal profiling data, means it's "hot" */
      EmitLiteral(*(literalPair.first), strIdx2Type);
      literalPair.second = true;
    }
  }
  /* emit hot literal end symbol */
  EmitBlockMarker("__MBlock_literal_hot_end", "", false);

  /* emit cold literal start symbol */
  EmitBlockMarker("__MBlock_literal_cold_begin", "", false);
  /* emit other literals (not in the profile) next. */
  for (const auto &literalPair : literals) {
    if (!literalPair.second) {
      /* not emit yet */
      EmitLiteral(*(literalPair.first), strIdx2Type);
    }
  }
  /* emit cold literal end symbol */
  EmitBlockMarker("__MBlock_literal_cold_end", "", false);
}

void Emitter::GetHotAndColdMetaSymbolInfo(const std::vector<MIRSymbol*> &mirSymbolVec,
                                          std::vector<MIRSymbol*> &hotFieldInfoSymbolVec,
                                          std::vector<MIRSymbol*> &coldFieldInfoSymbolVec, const std::string &prefixStr,
                                          bool forceCold) {
  bool isHot = false;
  for (auto mirSymbol : mirSymbolVec) {
    CHECK_FATAL(prefixStr.length() < mirSymbol->GetName().length(), "string length check");
    std::string name = mirSymbol->GetName().substr(prefixStr.length());
    std::string klassJavaDescriptor;
    namemangler::DecodeMapleNameToJavaDescriptor(name, klassJavaDescriptor);
    if (prefixStr == kFieldsInfoPrefixStr) {
      isHot = cg->GetMIRModule()->GetProfile().CheckFieldHot(klassJavaDescriptor);
    } else if (prefixStr == kMethodsInfoPrefixStr) {
      isHot = cg->GetMIRModule()->GetProfile().CheckMethodHot(klassJavaDescriptor);
    } else {
      isHot = cg->GetMIRModule()->GetProfile().CheckClassHot(klassJavaDescriptor);
    }
    if (isHot && !forceCold) {
      hotFieldInfoSymbolVec.emplace_back(mirSymbol);
    } else {
      coldFieldInfoSymbolVec.emplace_back(mirSymbol);
    }
  }
}

void Emitter::EmitMetaDataSymbolWithMarkFlag(const std::vector<MIRSymbol*> &mirSymbolVec,
                                             const std::map<GStrIdx, MIRType*> &strIdx2Type,
                                             const std::string &prefixStr, const std::string &sectionName,
                                             bool isHotFlag) {
  if (cg->GetMIRModule()->IsCModule()) {
    return;
  }
  if (mirSymbolVec.empty()) {
    return;
  }
  const std::string &markString = "__MBlock" + prefixStr;
  const std::string &hotOrCold = isHotFlag ? "hot" : "cold";
  EmitBlockMarker((markString + hotOrCold + "_begin"), sectionName, false);
  if (prefixStr == kFieldsInfoCompactPrefixStr || prefixStr == kMethodsInfoCompactPrefixStr ||
      prefixStr == kFieldOffsetDataPrefixStr || prefixStr == kMethodAddrDataPrefixStr) {
    for (auto s : mirSymbolVec) {
      EmitMethodFieldSequential(*s, strIdx2Type, sectionName);
    }
  } else {
    for (auto s : mirSymbolVec) {
      EmitClassInfoSequential(*s, strIdx2Type, sectionName);
    }
  }
  EmitBlockMarker((markString + hotOrCold + "_end"), sectionName, false);
}

void Emitter::MarkVtabOrItabEndFlag(const std::vector<MIRSymbol*> &mirSymbolVec) {
  for (auto mirSymbol : mirSymbolVec) {
    auto *aggConst = safe_cast<MIRAggConst>(mirSymbol->GetKonst());
    if ((aggConst == nullptr) || (aggConst->GetConstVec().empty())) {
      continue;
    }
    size_t size = aggConst->GetConstVec().size();
    MIRConst *elemConst = aggConst->GetConstVecItem(size - 1);
    ASSERT(elemConst != nullptr, "null ptr check");
    if (elemConst->GetKind() == kConstAddrofFunc) {
      maple::LogInfo::MapleLogger(kLlErr) << "ERROR: the last vtab/itab content should not be funcAddr\n";
    } else {
      if (elemConst->GetKind() != kConstInt) {
        CHECK_FATAL(elemConst->GetKind() == kConstAddrof, "must be");
        continue;
      }
      MIRIntConst *tabConst = static_cast<MIRIntConst*>(elemConst);
#ifdef USE_32BIT_REF
      /* #define COLD VTAB ITAB END FLAG  0X4000000000000000 */
      tabConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          static_cast<uint32>(tabConst->GetValue()) | 0X40000000, tabConst->GetType());
#else
      /* #define COLD VTAB ITAB END FLAG  0X4000000000000000 */
      tabConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          static_cast<int64>(static_cast<uint64>(tabConst->GetValue()) | 0X4000000000000000), tabConst->GetType());
#endif
      aggConst->SetItem(size - 1, tabConst, aggConst->GetFieldIdItem(size - 1));
    }
  }
}

void Emitter::EmitStringPointers() {
  Emit(asmInfo->GetSection()).Emit(asmInfo->GetData()).Emit("\n");
  for (auto idx: stringPtr) {
    if (idx == 0) {
      continue;
    }
    uint32 strId = idx.GetIdx();
    std::string str = GlobalTables::GetUStrTable().GetStringFromStrIdx(idx);
    Emit("\t.align 3\n");
    Emit(".LSTR__").Emit(strId).Emit(":\n");
    std::string mplstr(str);
    EmitStr(mplstr, false, true);
  }
}

void Emitter::EmitLocalVariable(const CGFunc &cgfunc) {
  /* function local pstatic initialization */
  if (cg->GetMIRModule()->IsCModule()) {
    MIRSymbolTable *lSymTab = cgfunc.GetMirModule().CurFunction()->GetSymTab();
    if (lSymTab != nullptr) {
      /* anything larger than is created by cg */
      size_t lsize = cgfunc.GetLSymSize();
      for (size_t i = 0; i < lsize; i++) {
        MIRSymbol *st = lSymTab->GetSymbolFromStIdx(i);
        if (st != nullptr && st->GetStorageClass() == kScPstatic) {
          /*
           * Local static names can repeat.
           * Append the current program unit index to the name.
           */
          PUIdx pIdx = cgfunc.GetMirModule().CurFunction()->GetPuidx();
          std::string localname = st->GetName() + std::to_string(pIdx);
          static std::vector<std::string> emittedLocalSym;
          bool found = false;
          for (auto name : emittedLocalSym) {
            if (name == localname) {
              found = true;
              break;
            }
          }
          if (found) {
            continue;
          }
          emittedLocalSym.push_back(localname);

          Emit(asmInfo->GetSection());
          Emit(asmInfo->GetData());
          Emit("\n");
          EmitAsmLabel(*st, kAsmAlign);
          MIRType *ty = st->GetType();
          MIRConst *ct = st->GetKonst();
          if (ct == nullptr) {
            EmitAsmLabel(*st, kAsmComm);
          } else if (kTypeStruct == ty->GetKind() || kTypeUnion == ty->GetKind() || kTypeClass == ty->GetKind()) {
            EmitAsmLabel(*st, kAsmSyname);
            EmitStructConstant(*ct);
          } else if (kTypeArray == ty->GetKind()) {
            if (ty->GetSize() != 0) {
              EmitAsmLabel(*st, kAsmSyname);
              EmitArrayConstant(*ct);
            }
          } else {
            EmitAsmLabel(*st, kAsmSyname);
            EmitScalarConstant(*ct, true, false, true /* isIndirect */);
          }
        }
      }
    }
  }
}

void Emitter::EmitGlobalVar(const MIRSymbol &globalVar) {
  EmitAsmLabel(globalVar, kAsmType);
  EmitAsmLabel(globalVar, kAsmLocal);
  EmitAsmLabel(globalVar, kAsmComm);
}

void Emitter::EmitGlobalVars(std::vector<std::pair<MIRSymbol*, bool>> &globalVars) {
  /* load globalVars profile */
  if (globalVars.empty()) {
    return;
  }
  std::unordered_set<std::string> hotVars;
  std::ifstream inFile;
  if (!CGOptions::IsGlobalVarProFileEmpty()) {
    inFile.open(CGOptions::GetGlobalVarProFile());
    if (inFile.fail()) {
      maple::LogInfo::MapleLogger(kLlErr) << "Cannot open globalVar profile file " << CGOptions::GetGlobalVarProFile()
                                         << "\n";
    }
  }
  if (CGOptions::IsGlobalVarProFileEmpty() || inFile.fail()) {
    for (const auto &globalVarPair : globalVars) {
      EmitGlobalVar(*(globalVarPair.first));
    }
    return;
  }
  std::string globalVarName;
  while (inFile >> globalVarName) {
    (void)hotVars.insert(globalVarName);
  }
  inFile.close();
  bool hotBeginSet = false;
  bool coldBeginSet = false;
  for (auto &globalVarPair : globalVars) {
    if (hotVars.find(globalVarPair.first->GetName()) != hotVars.end()) {
      if (!hotBeginSet) {
        /* emit hot globalvar start symbol */
        EmitBlockMarker("__MBlock_globalVars_hot_begin", "", true, globalVarPair.first->GetName());
        hotBeginSet = true;
      }
      EmitGlobalVar(*(globalVarPair.first));
      globalVarPair.second = true;
    }
  }
  for (const auto &globalVarPair : globalVars) {
    if (!globalVarPair.second) {  /* not emit yet */
      if (!coldBeginSet) {
        /* emit hot globalvar start symbol */
        EmitBlockMarker("__MBlock_globalVars_cold_begin", "", true, globalVarPair.first->GetName());
        coldBeginSet = true;
      }
      EmitGlobalVar(*(globalVarPair.first));
    }
  }
  MIRSymbol *endSym = globalVars.back().first;
  MIRType *mirType = endSym->GetType();
  const std::string kStaticVarEndAdd =
      std::to_string(Globals::GetInstance()->GetBECommon()->GetTypeSize(mirType->GetTypeIndex())) + "+" +
                     endSym->GetName();
  EmitBlockMarker("__MBlock_globalVars_cold_end", "", true, kStaticVarEndAdd);
}

void Emitter::EmitGlobalVariable() {
  std::vector<MIRSymbol*> typeStVec;
  std::vector<MIRSymbol*> typeNameStVec;
  std::map<GStrIdx, MIRType*> strIdx2Type;

  /* Create name2type map which will be used by reflection. */
  for (MIRType *type : GlobalTables::GetTypeTable().GetTypeTable()) {
    if (type == nullptr || (type->GetKind() != kTypeClass && type->GetKind() != kTypeInterface)) {
      continue;
    }
    GStrIdx strIdx = type->GetNameStrIdx();
    strIdx2Type[strIdx] = type;
  }

  /* sort symbols; classinfo-->field-->method */
  size_t size = GlobalTables::GetGsymTable().GetSymbolTableSize();
  std::vector<MIRSymbol*> classInfoVec;
  std::vector<MIRSymbol*> vtabVec;
  std::vector<MIRSymbol*> staticFieldsVec;
  std::vector<std::pair<MIRSymbol*, bool>> globalVarVec;
  std::vector<MIRSymbol*> itabVec;
  std::vector<MIRSymbol*> itabConflictVec;
  std::vector<MIRSymbol*> vtabOffsetVec;
  std::vector<MIRSymbol*> fieldOffsetVec;
  std::vector<MIRSymbol*> valueOffsetVec;
  std::vector<MIRSymbol*> localClassInfoVec;
  std::vector<MIRSymbol*> constStrVec;
  std::vector<std::pair<MIRSymbol*, bool>> literalVec;
  std::vector<MIRSymbol*> muidVec = { nullptr };
  std::vector<MIRSymbol*> fieldOffsetDatas;
  std::vector<MIRSymbol*> methodAddrDatas;
  std::vector<MIRSymbol*> methodSignatureDatas;
  std::vector<MIRSymbol*> staticDecoupleKeyVec;
  std::vector<MIRSymbol*> staticDecoupleValueVec;
  std::vector<MIRSymbol*> superClassStVec;
  std::vector<MIRSymbol*> arrayClassCacheVec;
  std::vector<MIRSymbol*> arrayClassCacheNameVec;

  for (size_t i = 0; i < size; ++i) {
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if (mirSymbol == nullptr || mirSymbol->IsDeleted() || mirSymbol->GetStorageClass() == kScUnused) {
      continue;
    }
    if (mirSymbol->GetName().find(VTAB_PREFIX_STR) == 0) {
      vtabVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(ITAB_PREFIX_STR) == 0) {
      itabVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(ITAB_CONFLICT_PREFIX_STR) == 0) {
      itabConflictVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kVtabOffsetTabStr) == 0) {
      vtabOffsetVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kFieldOffsetTabStr) == 0) {
      fieldOffsetVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kOffsetTabStr) == 0) {
      valueOffsetVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsArrayClassCache()) {
      arrayClassCacheVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsArrayClassCacheName()) {
      arrayClassCacheNameVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kLocalClassInfoStr) == 0) {
      localClassInfoVec.emplace_back(mirSymbol);
      continue;
    } else if (StringUtils::StartsWith(mirSymbol->GetName(), namemangler::kDecoupleStaticKeyStr)) {
      staticDecoupleKeyVec.emplace_back(mirSymbol);
      continue;
    } else if (StringUtils::StartsWith(mirSymbol->GetName(), namemangler::kDecoupleStaticValueStr)) {
      staticDecoupleValueVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsLiteral()) {
      literalVec.emplace_back(std::make_pair(mirSymbol, false));
      continue;
    } else if (mirSymbol->IsConstString() || mirSymbol->IsLiteralPtr()) {
      MIRConst *mirConst = mirSymbol->GetKonst();
      if (mirConst != nullptr && mirConst->GetKind() == kConstAddrof) {
        constStrVec.emplace_back(mirSymbol);
        continue;
      }
    } else if (mirSymbol->IsReflectionClassInfoPtr()) {
      /* _PTR__cinf is emitted in dataDefTab and dataUndefTab */
      continue;
    } else if (mirSymbol->IsMuidTab()) {
      if (!GetCG()->GetMIRModule()->IsCModule()) {
        muidVec[0] = mirSymbol;
        EmitMuidTable(muidVec, strIdx2Type, mirSymbol->GetMuidTabName());
      }
      continue;
    } else if (mirSymbol->IsCodeLayoutInfo()) {
      if (!GetCG()->GetMIRModule()->IsCModule()) {
        EmitFuncLayoutInfo(*mirSymbol);
      }
      continue;
    } else if (mirSymbol->GetName().find(kStaticFieldNamePrefixStr) == 0) {
      staticFieldsVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kGcRootList) == 0) {
      EmitGlobalRootList(*mirSymbol);
      continue;
    } else if (mirSymbol->GetName().find(kFunctionProfileTabPrefixStr) == 0) {
      muidVec[0] = mirSymbol;
      EmitMuidTable(muidVec, strIdx2Type, kFunctionProfileTabPrefixStr);
      continue;
    } else if (mirSymbol->IsReflectionFieldOffsetData()) {
      fieldOffsetDatas.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsReflectionMethodAddrData()) {
      methodAddrDatas.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsReflectionSuperclassInfo()) {
      superClassStVec.emplace_back(mirSymbol);
      continue;
    } else if (mirSymbol->IsReflectionMethodSignature()) {
      methodSignatureDatas.push_back(mirSymbol);
      continue;
    }

    if (mirSymbol->IsReflectionInfo()) {
      if (mirSymbol->IsReflectionClassInfo()) {
        classInfoVec.emplace_back(mirSymbol);
      }
      continue;
    }
    /* symbols we do not emit here. */
    if (mirSymbol->GetSKind() == kStFunc || mirSymbol->GetSKind() == kStJavaClass ||
        mirSymbol->GetSKind() == kStJavaInterface) {
      continue;
    }
    if (mirSymbol->GetStorageClass() == kScTypeInfo) {
      typeStVec.emplace_back(mirSymbol);
      continue;
    }
    if (mirSymbol->GetStorageClass() == kScTypeInfoName) {
      typeNameStVec.emplace_back(mirSymbol);
      continue;
    }
    if (mirSymbol->GetStorageClass() == kScTypeCxxAbi) {
      continue;
    }

    MIRType *mirType = mirSymbol->GetType();
    if (mirType == nullptr) {
      continue;
    }

    /*
     * emit uninitialized global/static variables.
     * these variables store in .comm section.
     */
    if ((mirSymbol->GetStorageClass() == kScGlobal || mirSymbol->GetStorageClass() == kScFstatic) &&
        !mirSymbol->IsConst()) {
      if (mirSymbol->IsGctibSym()) {
        /* GCTIB symbols are generated in GenerateObjectMaps */
        continue;
      }
      if (mirSymbol->GetStorageClass() == kScGlobal) {
        EmitAsmLabel(*mirSymbol, kAsmType);
        EmitAsmLabel(*mirSymbol, kAsmComm);
      } else {
        globalVarVec.emplace_back(std::make_pair(mirSymbol, false));
      }
      continue;
    }

    EmitAsmLabel(*mirSymbol, kAsmType);
    /* emit initialized global/static variables. */
    if (mirSymbol->GetStorageClass() == kScGlobal ||
        (mirSymbol->GetStorageClass() == kScFstatic && !mirSymbol->IsReadOnly())) {
      /* Emit section */
      if (mirSymbol->IsReflectionStrTab()) {
        std::string sectionName = ".reflection_strtab";
        if (mirSymbol->GetName().find(kReflectionStartHotStrtabPrefixStr) == 0) {
          sectionName = ".reflection_start_hot_strtab";
        } else if (mirSymbol->GetName().find(kReflectionBothHotStrTabPrefixStr) == 0) {
          sectionName = ".reflection_both_hot_strtab";
        } else if (mirSymbol->GetName().find(kReflectionRunHotStrtabPrefixStr) == 0) {
          sectionName = ".reflection_run_hot_strtab";
        }
        Emit("\t.section\t" + sectionName + ",\"a\",%progbits\n");
      } else if (mirSymbol->GetName().find(kDecoupleOption) == 0) {
        Emit("\t.section\t." + std::string(kDecoupleStr) + ",\"a\",%progbits\n");
      } else if (mirSymbol->IsRegJNITab()) {
        Emit("\t.section\t.reg_jni_tab,\"a\", %progbits\n");
      } else if (mirSymbol->GetName().find(kCompilerVersionNum) == 0) {
        Emit("\t.section\t." + std::string(kCompilerVersionNumStr) + ",\"a\", %progbits\n");
      } else if (mirSymbol->GetName().find(kSourceMuid) == 0) {
        Emit("\t.section\t." + std::string(kSourceMuidSectionStr) + ",\"a\", %progbits\n");
      } else if (mirSymbol->GetName().find(kCompilerMfileStatus) == 0) {
        Emit("\t.section\t." + std::string(kCompilerMfileStatus) + ",\"a\", %progbits\n");
      } else if (mirSymbol->IsRegJNIFuncTab()) {
        Emit("\t.section\t.reg_jni_func_tab,\"aw\", %progbits\n");
      } else if (mirSymbol->IsReflectionPrimitiveClassInfo()) {
        Emit("\t.section\t.primitive_classinfo,\"awG\", %progbits,__primitive_classinfo__,comdat\n");
      } else if (mirSymbol->IsReflectionHashTabBucket()) {
        std::string stName = mirSymbol->GetName();
        const std::string delimiter = "$$";
        if (stName.find(delimiter) == std::string::npos) {
          FATAL(kLncFatal, "Can not find delimiter in target ");
        }
        std::string secName = stName.substr(0, stName.find(delimiter));
        /* remove leading "__" in sec name. */
        secName.erase(0, 2);
        Emit("\t.section\t." + secName + ",\"a\",%progbits\n");
      } else {
        (void)Emit("\t.section\t." + std::string(kMapleGlobalVariable) + ",\"aw\", %progbits\n");
      }
      /* Emit size and align by type */
      if (mirSymbol->GetStorageClass() == kScGlobal) {
        if (mirSymbol->GetAttr(ATTR_weak) || mirSymbol->IsReflectionPrimitiveClassInfo()) {
          EmitAsmLabel(*mirSymbol, kAsmWeak);
        } else {
          EmitAsmLabel(*mirSymbol, kAsmGlbl);
        }
        EmitAsmLabel(*mirSymbol, kAsmHidden);
      } else if (mirSymbol->GetStorageClass() == kScFstatic) {
        EmitAsmLabel(*mirSymbol, kAsmLocal);
      }
      if (mirSymbol->IsReflectionStrTab()) {
        Emit("\t.align 3\n");  /* reflection-string-tab also aligned to 8B boundaries. */
      } else {
        EmitAsmLabel(*mirSymbol, kAsmAlign);
      }
      EmitAsmLabel(*mirSymbol, kAsmSyname);
      MIRConst *mirConst = mirSymbol->GetKonst();
      if (IsPrimitiveScalar(mirType->GetPrimType())) {
        if (IsAddress(mirType->GetPrimType())) {
          uint32 sizeinbits = GetPrimTypeBitSize(mirConst->GetType().GetPrimType());
          CHECK_FATAL(sizeinbits == k64BitSize, "EmitGlobalVariable: pointer must be of size 8");
        }
        if (cg->GetMIRModule()->IsCModule()) {
          EmitScalarConstant(*mirConst, true, false, true);
        } else {
          EmitScalarConstant(*mirConst);
        }
      } else if (mirType->GetKind() == kTypeArray) {
        if (mirSymbol->HasAddrOfValues()) {
          EmitConstantTable(*mirSymbol, *mirConst, strIdx2Type);
        } else {
          EmitArrayConstant(*mirConst);
        }
      } else if (mirType->GetKind() == kTypeStruct || mirType->GetKind() == kTypeClass ||
                 mirType->GetKind() == kTypeUnion) {
        if (mirSymbol->HasAddrOfValues()) {
          EmitConstantTable(*mirSymbol, *mirConst, strIdx2Type);
        } else {
          EmitStructConstant(*mirConst);
        }
      } else {
        ASSERT(false, "NYI");
      }
      EmitAsmLabel(*mirSymbol, kAsmSize);
      /* emit constant float/double */
    } else if (mirSymbol->IsReadOnly()) {
      /* emit .section */
      Emit(asmInfo->GetSection());
      Emit(asmInfo->GetRodata());
      Emit("\n");
      EmitAsmLabel(*mirSymbol, kAsmAlign);
      EmitAsmLabel(*mirSymbol, kAsmSyname);
      MIRConst *mirConst = mirSymbol->GetKonst();
      EmitScalarConstant(*mirConst);
    } else if (mirSymbol->GetStorageClass() == kScPstatic) {
      Emit(asmInfo->GetSection());
      Emit(asmInfo->GetData());
      Emit("\n");
      EmitAsmLabel(*mirSymbol, kAsmAlign);
      MIRConst *ct = mirSymbol->GetKonst();
      if (ct == nullptr) {
        EmitAsmLabel(*mirSymbol, kAsmComm);
      } else if (IsPrimitiveScalar(mirType->GetPrimType())) {
        EmitAsmLabel(*mirSymbol, kAsmSyname);
        EmitScalarConstant(*ct, true, false, true);
      } else if (kTypeArray == mirType->GetKind()) {
        EmitAsmLabel(*mirSymbol, kAsmSyname);
        EmitArrayConstant(*ct);
      } else if (kTypeStruct == mirType->GetKind() || kTypeClass == mirType->GetKind() ||
                 kTypeUnion == mirType->GetKind()) {
        EmitAsmLabel(*mirSymbol, kAsmSyname);
        EmitStructConstant(*ct);
      } else {
        CHECK_FATAL(0, "Unknown type in Global pstatic");
      }
    }
  } /* end proccess all mirSymbols. */
  EmitStringPointers();
  /* emit global var */
  EmitGlobalVars(globalVarVec);
  /* emit literal std::strings */
  EmitLiterals(literalVec, strIdx2Type);
  /* emit static field std::strings */
  EmitStaticFields(staticFieldsVec);

  if (GetCG()->GetMIRModule()->IsCModule()) {
    return;
  }

  EmitMuidTable(constStrVec, strIdx2Type, kMuidConststrPrefixStr);

  /* emit classinfo, field, method */
  std::vector<MIRSymbol*> fieldInfoStVec;
  std::vector<MIRSymbol*> fieldInfoStCompactVec;
  std::vector<MIRSymbol*> methodInfoStVec;
  std::vector<MIRSymbol*> methodInfoStCompactVec;

  std::string sectionName = kMuidClassMetadataPrefixStr;
  Emit("\t.section ." + sectionName + ",\"aw\",%progbits\n");
  Emit(sectionName + "_begin:\n");

  for (size_t i = 0; i < classInfoVec.size(); ++i) {
    MIRSymbol *mirSymbol = classInfoVec[i];
    if (mirSymbol != nullptr && mirSymbol->GetKonst() != nullptr && mirSymbol->IsReflectionClassInfo()) {
      /* Emit classinfo */
      EmitClassInfoSequential(*mirSymbol, strIdx2Type, sectionName);
      std::string stName = mirSymbol->GetName();
      std::string className = stName.substr(strlen(CLASSINFO_PREFIX_STR));
      /* Get classinfo ro symbol */
      MIRSymbol *classInfoROSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(CLASSINFO_RO_PREFIX_STR + className));
      EmitClassInfoSequential(*classInfoROSt, strIdx2Type, sectionName);
      /* Get fields */
      MIRSymbol *fieldSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kFieldsInfoPrefixStr + className));
      MIRSymbol *fieldStCompact = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kFieldsInfoCompactPrefixStr + className));
      /* Get methods */
      MIRSymbol *methodSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kMethodsInfoPrefixStr + className));
      MIRSymbol *methodStCompact = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(kMethodsInfoCompactPrefixStr + className));

      if (fieldSt != nullptr) {
        fieldInfoStVec.emplace_back(fieldSt);
      }
      if (fieldStCompact != nullptr) {
        fieldInfoStCompactVec.emplace_back(fieldStCompact);
      }
      if (methodSt != nullptr) {
        methodInfoStVec.emplace_back(methodSt);
      }
      if (methodStCompact != nullptr) {
        methodInfoStCompactVec.emplace_back(methodStCompact);
      }
    }
  }
  Emit(sectionName + "_end:\n");

  std::vector<MIRSymbol*> hotVtabStVec;
  std::vector<MIRSymbol*> coldVtabStVec;
  std::vector<MIRSymbol*> hotItabStVec;
  std::vector<MIRSymbol*> coldItabStVec;
  std::vector<MIRSymbol*> hotItabCStVec;
  std::vector<MIRSymbol*> coldItabCStVec;
  std::vector<MIRSymbol*> hotMethodsInfoCStVec;
  std::vector<MIRSymbol*> coldMethodsInfoCStVec;
  std::vector<MIRSymbol*> hotFieldsInfoCStVec;
  std::vector<MIRSymbol*> coldFieldsInfoCStVec;
  GetHotAndColdMetaSymbolInfo(vtabVec, hotVtabStVec, coldVtabStVec, VTAB_PREFIX_STR,
                              ((CGOptions::IsLazyBinding() || CGOptions::IsHotFix()) && !cg->IsLibcore()));
  GetHotAndColdMetaSymbolInfo(itabVec, hotItabStVec, coldItabStVec, ITAB_PREFIX_STR,
                              ((CGOptions::IsLazyBinding() || CGOptions::IsHotFix()) && !cg->IsLibcore()));
  GetHotAndColdMetaSymbolInfo(itabConflictVec, hotItabCStVec, coldItabCStVec, ITAB_CONFLICT_PREFIX_STR,
                              ((CGOptions::IsLazyBinding() || CGOptions::IsHotFix()) && !cg->IsLibcore()));
  GetHotAndColdMetaSymbolInfo(fieldInfoStVec, hotFieldsInfoCStVec, coldFieldsInfoCStVec, kFieldsInfoPrefixStr);
  GetHotAndColdMetaSymbolInfo(methodInfoStVec, hotMethodsInfoCStVec, coldMethodsInfoCStVec, kMethodsInfoPrefixStr);

  std::string sectionNameIsEmpty;
  std::string fieldSectionName("rometadata.field");
  std::string methodSectionName("rometadata.method");

  /* fieldinfo */
  EmitMetaDataSymbolWithMarkFlag(hotFieldsInfoCStVec, strIdx2Type, kFieldsInfoPrefixStr, fieldSectionName, true);
  EmitMetaDataSymbolWithMarkFlag(coldFieldsInfoCStVec, strIdx2Type, kFieldsInfoPrefixStr, fieldSectionName, false);
  EmitMetaDataSymbolWithMarkFlag(fieldInfoStCompactVec, strIdx2Type, kFieldsInfoCompactPrefixStr, fieldSectionName,
                                 false);
  /* methodinfo */
  EmitMetaDataSymbolWithMarkFlag(hotMethodsInfoCStVec, strIdx2Type, kMethodsInfoPrefixStr, methodSectionName, true);
  EmitMetaDataSymbolWithMarkFlag(coldMethodsInfoCStVec, strIdx2Type, kMethodsInfoPrefixStr, methodSectionName, false);
  EmitMetaDataSymbolWithMarkFlag(methodInfoStCompactVec, strIdx2Type, kMethodsInfoCompactPrefixStr, methodSectionName,
                                 false);

  /* itabConflict */
  MarkVtabOrItabEndFlag(coldItabCStVec);
  EmitMuidTable(hotItabCStVec, strIdx2Type, kMuidItabConflictPrefixStr);
  EmitMetaDataSymbolWithMarkFlag(coldItabCStVec, strIdx2Type, ITAB_CONFLICT_PREFIX_STR, kMuidColdItabConflictPrefixStr,
                                 false);

  /*
   * vtab
   * And itab to vtab section
   */
  for (auto sym : hotItabStVec) {
    hotVtabStVec.emplace_back(sym);
  }
  for (auto sym : coldItabStVec) {
    coldVtabStVec.emplace_back(sym);
  }
  MarkVtabOrItabEndFlag(coldVtabStVec);
  EmitMuidTable(hotVtabStVec, strIdx2Type, kMuidVtabAndItabPrefixStr);
  EmitMetaDataSymbolWithMarkFlag(coldVtabStVec, strIdx2Type, VTAB_AND_ITAB_PREFIX_STR, kMuidColdVtabAndItabPrefixStr,
                                 false);

  /* vtab_offset */
  EmitMuidTable(vtabOffsetVec, strIdx2Type, kMuidVtabOffsetPrefixStr);
  /* field_offset */
  EmitMuidTable(fieldOffsetVec, strIdx2Type, kMuidFieldOffsetPrefixStr);
  /* value_offset */
  EmitMuidTable(valueOffsetVec, strIdx2Type, kMuidValueOffsetPrefixStr);
  /* local clasinfo */
  EmitMuidTable(localClassInfoVec, strIdx2Type, kMuidLocalClassInfoStr);
  /* Emit decouple static */
  EmitMuidTable(staticDecoupleKeyVec, strIdx2Type, kDecoupleStaticKeyStr);
  EmitMuidTable(staticDecoupleValueVec, strIdx2Type, kDecoupleStaticValueStr);

  /* super class */
  EmitMuidTable(superClassStVec, strIdx2Type, kMuidSuperclassPrefixStr);

  /* field offset rw */
  EmitMetaDataSymbolWithMarkFlag(fieldOffsetDatas, strIdx2Type, kFieldOffsetDataPrefixStr, sectionNameIsEmpty, false);
  /* method address rw */
  EmitMetaDataSymbolWithMarkFlag(methodAddrDatas, strIdx2Type, kMethodAddrDataPrefixStr, sectionNameIsEmpty, false);
  /* method address ro */
  std::string methodSignatureSectionName("romethodsignature");
  EmitMetaDataSymbolWithMarkFlag(methodSignatureDatas, strIdx2Type, kMethodSignaturePrefixStr,
                                 methodSignatureSectionName, false);

  /* array class cache table */
  EmitMuidTable(arrayClassCacheVec, strIdx2Type, kArrayClassCacheTable);
  /* array class cache name table */
  EmitMuidTable(arrayClassCacheNameVec, strIdx2Type, kArrayClassCacheNameTable);

#if !defined(TARGARM32)
  /* finally emit __gxx_personality_v0 DW.ref */
  if (!cg->GetMIRModule()->IsCModule())  {
    EmitDWRef("__mpl_personality_v0");
  }
#endif
}
void Emitter::EmitAddressString(const std::string &address) {
#if TARGAARCH64 || TARGRISCV64
  Emit("\t.quad\t" + address);
#else
  Emit("\t.word\t" + address);
#endif
}
void Emitter::EmitGlobalRootList(const MIRSymbol &gcrootsSt) {
  Emit("\t.section .maple.gcrootsmap").Emit(",\"aw\",%progbits\n");
  std::vector<std::string> nameVec;
  std::string name = gcrootsSt.GetName();
  nameVec.emplace_back(name);
  nameVec.emplace_back(name + "Size");
  bool gcrootsFlag = true;
  uint64 vecSize = 0;
  for (const auto &gcrootsName : nameVec) {
#if TARGAARCH64 || TARGRISCV64
    Emit("\t.type\t" + gcrootsName + ", @object\n" + "\t.p2align 3\n");
#else
    Emit("\t.type\t" + gcrootsName + ", %object\n" + "\t.p2align 3\n");
#endif
    Emit("\t.global\t" + gcrootsName + "\n");
    if (gcrootsFlag) {
      Emit(kMuidGlobalRootlistPrefixStr).Emit("_begin:\n");
    }
    Emit(gcrootsName + ":\n");
    if (gcrootsFlag) {
      MIRAggConst *aggConst = safe_cast<MIRAggConst>(gcrootsSt.GetKonst());
      if (aggConst == nullptr) {
        continue;
      }
      size_t i = 0;
      while (i < aggConst->GetConstVec().size()) {
        MIRConst *elemConst = aggConst->GetConstVecItem(i);
        if (elemConst->GetKind() == kConstAddrof) {
          MIRAddrofConst *symAddr = safe_cast<MIRAddrofConst>(elemConst);
          CHECK_FATAL(symAddr != nullptr, "nullptr of symAddr");
          MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(symAddr->GetSymbolIndex().Idx());
          const std::string &symAddrName = symAddrSym->GetName();
          EmitAddressString(symAddrName + "\n");
        } else {
          EmitScalarConstant(*elemConst);
        }
        i++;
      }
      vecSize = i;
    } else {
      EmitAddressString(std::to_string(vecSize) + "\n");
    }
    Emit("\t.size\t" + gcrootsName + ",.-").Emit(gcrootsName + "\n");
    if (gcrootsFlag) {
      Emit(kMuidGlobalRootlistPrefixStr).Emit("_end:\n");
    }
    gcrootsFlag = false;
  }
}

void Emitter::EmitMuidTable(const std::vector<MIRSymbol*> &vec, const std::map<GStrIdx, MIRType*> &strIdx2Type,
                            const std::string &sectionName) {
  MIRSymbol *st = nullptr;
  if (!vec.empty()) {
    st = vec[0];
  }
  if (st != nullptr && st->IsMuidRoTab()) {
    Emit("\t.section  ." + sectionName + ",\"a\",%progbits\n");
  } else {
    Emit("\t.section  ." + sectionName + ",\"aw\",%progbits\n");
  }
  Emit(sectionName + "_begin:\n");
  bool isConstString = sectionName == kMuidConststrPrefixStr;
  for (size_t i = 0; i < vec.size(); i++) {
    MIRSymbol *st1 = vec[i];
    ASSERT(st1 != nullptr, "null ptr check");
    if (st1->GetStorageClass() == kScUnused) {
      continue;
    }
    EmitAsmLabel(*st1, kAsmType);
    if (st1->GetStorageClass() == kScFstatic) {
      EmitAsmLabel(*st1, kAsmLocal);
    } else {
      EmitAsmLabel(*st1, kAsmGlbl);
      EmitAsmLabel(*st1, kAsmHidden);
    }
    EmitAsmLabel(*st1, kAsmAlign);
    EmitAsmLabel(*st1, kAsmSyname);
    MIRConst *mirConst = st1->GetKonst();
    CHECK_FATAL(mirConst != nullptr, "mirConst should not be nullptr in EmitMuidTable");
    if (mirConst->GetKind() == kConstAddrof) {
      MIRAddrofConst *symAddr = safe_cast<MIRAddrofConst>(mirConst);
      CHECK_FATAL(symAddr != nullptr, "call static_cast failed in EmitMuidTable");
      MIRSymbol *symAddrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(symAddr->GetSymbolIndex().Idx());
      if (isConstString) {
        EmitAddressString(symAddrSym->GetName() + " - . + ");
        Emit(kDataRefIsOffset);
        Emit("\n");
      } else {
        EmitAddressString(symAddrSym->GetName() + "\n");
      }
    } else if (mirConst->GetKind() == kConstInt) {
      EmitScalarConstant(*mirConst, true);
    } else {
      EmitConstantTable(*st1, *mirConst, strIdx2Type);
    }
    EmitAsmLabel(*st1, kAsmSize);
  }
  Emit(sectionName + "_end:\n");
}

void Emitter::EmitClassInfoSequential(const MIRSymbol &mirSymbol, const std::map<GStrIdx, MIRType*> &strIdx2Type,
                                      const std::string &sectionName) {
  EmitAsmLabel(mirSymbol, kAsmType);
  if (!sectionName.empty()) {
    Emit("\t.section ." + sectionName);
    if (StringUtils::StartsWith(sectionName, "ro")) {
      Emit(",\"a\",%progbits\n");
    } else {
      Emit(",\"aw\",%progbits\n");
    }
  } else {
    EmitAsmLabel(kAsmData);
  }
  EmitAsmLabel(mirSymbol, kAsmGlbl);
  EmitAsmLabel(mirSymbol, kAsmHidden);
  EmitAsmLabel(mirSymbol, kAsmAlign);
  EmitAsmLabel(mirSymbol, kAsmSyname);
  MIRConst *mirConst = mirSymbol.GetKonst();
  CHECK_FATAL(mirConst != nullptr, "mirConst should not be nullptr in EmitClassInfoSequential");
  EmitConstantTable(mirSymbol, *mirConst, strIdx2Type);
  EmitAsmLabel(mirSymbol, kAsmSize);
}

void Emitter::EmitMethodDeclaringClass(const MIRSymbol &mirSymbol, const std::string &sectionName) {
  std::string symName = mirSymbol.GetName();
  std::string emitSyName = symName + "_DeclaringClass";
  std::string declaringClassName = symName.substr(strlen(kFieldsInfoCompactPrefixStr) + 1);
  Emit(asmInfo->GetType());
  Emit(emitSyName + ", %object\n");
  if (!sectionName.empty()) {
    Emit("\t.section  ." + sectionName + "\n");
  } else {
    EmitAsmLabel(kAsmData);
  }
  Emit(asmInfo->GetLocal());
  Emit(emitSyName + "\n");
  Emit(asmInfo->GetAlign());
  Emit("  2\n" + emitSyName + ":\n");
  Emit("\t.long\t");
  Emit(CLASSINFO_PREFIX_STR + declaringClassName + " - .\n");
  Emit(asmInfo->GetSize());
  Emit(emitSyName + ", 4\n");
}

void Emitter::EmitMethodFieldSequential(const MIRSymbol &mirSymbol,
                                        const std::map<GStrIdx, MIRType*> &strIdx2Type,
                                        const std::string &sectionName) {
  std::string symName = mirSymbol.GetName();
  if (symName.find(kMethodsInfoCompactPrefixStr) != std::string::npos) {
    EmitMethodDeclaringClass(mirSymbol, sectionName);
  }
  EmitAsmLabel(mirSymbol, kAsmType);
  if (!sectionName.empty()) {
    Emit("\t.section  ." + sectionName + "\n");
  } else {
    EmitAsmLabel(kAsmData);
  }
  EmitAsmLabel(mirSymbol, kAsmLocal);

  /* Emit(2) is 4 bit align */
  Emit(asmInfo->GetAlign()).Emit(2).Emit("\n");
  EmitAsmLabel(mirSymbol, kAsmSyname);
  MIRConst *ct = mirSymbol.GetKonst();
  EmitConstantTable(mirSymbol, *ct, strIdx2Type);
  std::string symbolName = mirSymbol.GetName();
  Emit("\t.size\t" + symbolName + ", .-");
  Emit(symbolName + "\n");
}

void Emitter::EmitDWRef(const std::string &name) {
  /*
   *   .hidden DW.ref._ZTI3xxx
   *   .weak DW.ref._ZTI3xxx
   *   .section  .data.DW.ref._ZTI3xxx,"awG",@progbits,DW.ref._ZTI3xxx,comdat
   *   .align  3
   *   .type DW.ref._ZTI3xxx, %object
   *   .size DW.ref._ZTI3xxx, 8
   * DW.ref._ZTI3xxx:
   *   .xword  _ZTI3xxx
   */
  Emit("\t.hidden DW.ref." + name + "\n");
  Emit("\t.weak DW.ref." + name + "\n");
  Emit("\t.section .data.DW.ref." + name + ",\"awG\",%progbits,DW.ref.");
  Emit(name + ",comdat\n");
  Emit("\t.align 3\n");
  Emit("\t.type DW.ref." + name + ", \%object\n");
  Emit("\t.size DW.ref." + name + ",8\n");
  Emit("DW.ref." + name + ":\n");
#if TARGAARCH64 || TARGRISCV64
  Emit("\t.xword " + name + "\n");
#else
  Emit("\t.word " + name + "\n");
#endif
}

void Emitter::EmitDecSigned(int64 num) {
  std::ios::fmtflags flag(outStream.flags());
  outStream << std::dec << num;
  outStream.flags(flag);
}

void Emitter::EmitDecUnsigned(uint64 num) {
  std::ios::fmtflags flag(outStream.flags());
  outStream << std::dec << num;
  outStream.flags(flag);
}

void Emitter::EmitHexUnsigned(uint64 num) {
  std::ios::fmtflags flag(outStream.flags());
  outStream << "0x" << std::hex << num;
  outStream.flags(flag);
}

#define XSTR(s) str(s)
#define str(s) #s

void Emitter::EmitDIHeader() {
  if (cg->GetMIRModule()->GetSrcLang() == kSrcLangC) {
    (void)Emit("\t.section ." + std::string("c_text") + ",\"ax\"\n");
  } else {
    (void)Emit("\t.section ." + std::string(namemangler::kMuidJavatextPrefixStr) + ",\"ax\"\n");
  }
  Emit(".L" XSTR(TEXT_BEGIN) ":\n");
}

void Emitter::EmitDIFooter() {
  (void)Emit("\t.section ." + std::string(namemangler::kMuidJavatextPrefixStr) + ",\"ax\"\n");
  Emit(".L" XSTR(TEXT_END) ":\n");
}

void Emitter::EmitDIHeaderFileInfo() {
  Emit("// dummy header file 1\n");
  Emit("// dummy header file 2\n");
  Emit("// dummy header file 3\n");
}

void Emitter::AddLabelDieToLabelIdxMapping(DBGDie *lbldie, LabelIdx lblidx) {
  InsertLabdie2labidxTable(lbldie, lblidx);
}

LabelIdx Emitter::GetLabelIdxForLabelDie(DBGDie *lbldie) {
  auto it = labdie2labidxTable.find(lbldie);
  CHECK_FATAL(it != labdie2labidxTable.end(), "");
  return it->second;
}


void Emitter::ApplyInPrefixOrder(DBGDie *die, const std::function<void(DBGDie*)> &func) {
  func(die);
  ASSERT(die, "");
  if (die->GetSubDieVec().size() > 0) {
    for (auto c : die->GetSubDieVec()) {
      ApplyInPrefixOrder(c, func);
    }
    /* mark the end of the sibling list */
    func(nullptr);
  }
}

void Emitter::EmitDIFormSpecification(unsigned int dwform) {
  switch (dwform) {
    case DW_FORM_string:
      Emit(".string  ");
      break;
    case DW_FORM_strp:
    case DW_FORM_data4:
    case DW_FORM_ref4:
      Emit(".4byte   ");
      break;
    case DW_FORM_data1:
      Emit(".byte    ");
      break;
    case DW_FORM_data2:
      Emit(".2byte   ");
      break;
    case DW_FORM_data8:
      Emit(".8byte   ");
      break;
    case DW_FORM_sec_offset:
      /* if DWARF64, should be .8byte? */
      Emit(".4byte   ");
      break;
    case DW_FORM_addr: /* Should we use DWARF64? for now, we generate .8byte as gcc does for DW_FORM_addr */
      Emit(".8byte   ");
      break;
    case DW_FORM_exprloc:
      Emit(".uleb128 ");
      break;
    default:
      CHECK_FATAL(maple::GetDwFormName(dwform) != nullptr,
                  "GetDwFormName() return null in Emitter::EmitDIFormSpecification");
      LogInfo::MapleLogger() << "unhandled : " << maple::GetDwFormName(dwform) << std::endl;
      ASSERT(0, "NYI");
  }
}

void Emitter::EmitDIAttrValue(DBGDie *die, DBGDieAttr *attr, DwAt attrName, DwTag tagName, DebugInfo *di) {
  MapleVector<DBGDieAttr*> &attrvec = die->GetAttrVec();

  switch (attr->GetDwForm()) {
    case DW_FORM_string: {
      const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(attr->GetId());
      Emit("\"").Emit(name).Emit("\"");
      Emit(CMNT "len = ");
      EmitDecUnsigned(name.length() + 1);
    } break;
    case DW_FORM_strp:
      Emit(".L" XSTR(DEBUG_STR_LABEL));
      outStream << attr->GetId();
      break;
    case DW_FORM_data1:
#if DEBUG
      if (attr->GetI() == kDbgDefaultVal) {
        EmitHexUnsigned(attr->GetI());
      } else
#endif
      EmitHexUnsigned(uint8_t(attr->GetI()));
      break;
    case DW_FORM_data2:
#if DEBUG
      if (attr->GetI() == kDbgDefaultVal) {
        EmitHexUnsigned(attr->GetI());
      } else
#endif
      EmitHexUnsigned(uint16_t(attr->GetI()));
      break;
    case DW_FORM_data4:
#if DEBUG
      if (attr->GetI() == kDbgDefaultVal) {
        EmitHexUnsigned(attr->GetI());
      } else
#endif
      EmitHexUnsigned(uint32_t(attr->GetI()));
      break;
    case DW_FORM_data8:
      if (attrName == DW_AT_high_pc) {
        if (tagName == DW_TAG_compile_unit) {
          Emit(".L" XSTR(TEXT_END) "-.L" XSTR(TEXT_BEGIN));
        } else if (tagName == DW_TAG_subprogram) {
          DBGDieAttr *name = LFindAttribute(attrvec, DW_AT_name);
          if (name == nullptr) {
            DBGDieAttr *spec = LFindAttribute(attrvec, DW_AT_specification);
            CHECK_FATAL(spec != nullptr, "spec is null in Emitter::EmitDIAttrValue");
            DBGDie *decl = di->GetDie(spec->GetId());
            name = LFindAttribute(decl->GetAttrVec(), DW_AT_name);
          }
          CHECK_FATAL(name != nullptr, "name is null in Emitter::EmitDIAttrValue");
          const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(name->GetId());

          MIRBuilder *mirbuilder = GetCG()->GetMIRModule()->GetMIRBuilder();
          MIRFunction *mfunc = mirbuilder->GetFunctionFromName(str);
          MapleMap<MIRFunction*, std::pair<LabelIdx, LabelIdx> >::iterator it =
              CG::GetFuncWrapLabels().find(mfunc);
          if (it != CG::GetFuncWrapLabels().end()) {
            EmitLabelForFunc(mfunc, (*it).second.second);  /* end label */
          } else {
            EmitLabelRef(attr->GetId());      /* maybe deadbeef */
          }
          Emit("-");
          if (it != CG::GetFuncWrapLabels().end()) {
            EmitLabelForFunc(mfunc, (*it).second.first);  /* start label */
          } else {
            DBGDieAttr *lowpc = LFindAttribute(attrvec, DW_AT_low_pc);
            CHECK_FATAL(lowpc != nullptr, "lowpc is null in Emitter::EmitDIAttrValue");
            EmitLabelRef(lowpc->GetId());    /* maybe deadbeef */
          }
        }
      } else {
        EmitHexUnsigned(attr->GetI());
      }
      break;
    case DW_FORM_sec_offset:
      if (attrName == DW_AT_stmt_list) {
        Emit(".L");
        Emit(XSTR(DEBUG_LINE_0));
      }
      break;
    case DW_FORM_addr:
      if (attrName == DW_AT_low_pc) {
        if (tagName == DW_TAG_compile_unit) {
          Emit(".L" XSTR(TEXT_BEGIN));
        } else if (tagName == DW_TAG_subprogram) {
          /* if decl, name should be found; if def, we try DW_AT_specification */
          DBGDieAttr *name = LFindAttribute(attrvec, DW_AT_name);
          if (name == nullptr) {
            DBGDieAttr *spec = LFindAttribute(attrvec, DW_AT_specification);
            CHECK_FATAL(spec != nullptr, "spec is null in Emitter::EmitDIAttrValue");
            DBGDie *decl = di->GetDie(spec->GetId());
            name = LFindAttribute(decl->GetAttrVec(), DW_AT_name);
          }
          CHECK_FATAL(name != nullptr, "name is null in Emitter::EmitDIAttrValue");
          const std::string &str = GlobalTables::GetStrTable().GetStringFromStrIdx(name->GetId());
          MIRBuilder *mirbuilder = GetCG()->GetMIRModule()->GetMIRBuilder();
          MIRFunction *mfunc = mirbuilder->GetFunctionFromName(str);
          MapleMap<MIRFunction*, std::pair<LabelIdx, LabelIdx> >::iterator
              it = CG::GetFuncWrapLabels().find(mfunc);
          if (it != CG::GetFuncWrapLabels().end()) {
            EmitLabelForFunc(mfunc, (*it).second.first);  /* it is a <pair> */
          } else {
            EmitLabelRef(attr->GetId());     /* maybe deadbeef */
          }
        } else if (tagName == DW_TAG_label) {
          LabelIdx labelIdx = GetLabelIdxForLabelDie(die);
          DBGDie *subpgm = die->GetParent();
          ASSERT(subpgm->GetTag() == DW_TAG_subprogram, "Label DIE should be a child of a Subprogram DIE");
          DBGDieAttr *fnameAttr = LFindAttribute(subpgm->GetAttrVec(), DW_AT_name);
          if (!fnameAttr) {
            DBGDieAttr *specAttr = LFindAttribute(subpgm->GetAttrVec(), DW_AT_specification);
            CHECK_FATAL(specAttr, "pointer is null");
            DBGDie *twin = di->GetDie(specAttr->GetU());
            fnameAttr = LFindAttribute(twin->GetAttrVec(), DW_AT_name);
          }
          CHECK_FATAL(fnameAttr, "");
          const std::string &fnameStr = GlobalTables::GetStrTable().GetStringFromStrIdx(fnameAttr->GetId());
          LabelOperand *res = memPool->New<LabelOperand>(fnameStr.c_str(), labelIdx);
          res->Emit(*this, nullptr);
        }
      } else if (attrName == DW_AT_high_pc) {
        if (tagName == DW_TAG_compile_unit) {
          Emit(".L" XSTR(TEXT_END) "-.L" XSTR(TEXT_BEGIN));
        }
      } else {
        Emit("XXX--ADDR--XXX");
      }
      break;
    case DW_FORM_ref4:
      if (attrName == DW_AT_type) {
        DBGDie *die0 = di->GetDie(attr->GetU());
        if (die0->GetOffset()) {
          EmitHexUnsigned(die0->GetOffset());
        } else {
          /* unknown type, missing mplt */
          EmitHexUnsigned(di->GetDummyTypeDie()->GetOffset());
          Emit(CMNT "Warning: dummy type used");
        }
      } else if (attrName == DW_AT_specification || attrName == DW_AT_sibling) {
        DBGDie *die0 = di->GetDie(attr->GetU());
        ASSERT(die0->GetOffset(), "");
        EmitHexUnsigned(die0->GetOffset());
      } else if (attrName == DW_AT_object_pointer) {
        GStrIdx thisIdx = GlobalTables::GetStrTable().GetStrIdxFromName(kDebugMapleThis);
        DBGDie *that = LFindChildDieWithName(die, DW_TAG_formal_parameter, thisIdx);
        /* need to find the this or self based on the source language
           what is the name for 'this' used in mapleir?
           this has to be with respect to a function */
        if (that) {
          EmitHexUnsigned(that->GetOffset());
        } else {
          EmitHexUnsigned(attr->GetU());
        }
      } else {
        Emit(" OFFSET ");
        EmitHexUnsigned(attr->GetU());
      }
      break;
    case DW_FORM_exprloc: {
      DBGExprLoc *elp = attr->GetPtr();
      switch (elp->GetOp()) {
        case DW_OP_call_frame_cfa:
          EmitHexUnsigned(1);
          Emit("\n\t.byte    ");
          EmitHexUnsigned(elp->GetOp());
          break;
        case DW_OP_addr:
          EmitHexUnsigned(k9ByteSize);
          Emit("\n\t.byte    ");
          EmitHexUnsigned(elp->GetOp());
          Emit("\n\t.8byte   ");
          Emit(GlobalTables::GetStrTable().GetStringFromStrIdx(elp->GetGvarStridx()).c_str());
          break;
        case DW_OP_fbreg:
          EmitHexUnsigned(1 + namemangler::GetSleb128Size(elp->GetFboffset()));
          Emit("\n\t.byte    ");
          EmitHexUnsigned(elp->GetOp());
          Emit("\n\t.sleb128 ");
          EmitDecSigned(elp->GetFboffset());
          break;
        default:
          EmitHexUnsigned(uintptr_t(elp));
          break;
      }
    } break;
    default:
      CHECK_FATAL(maple::GetDwFormName(attr->GetDwForm()) != nullptr,
                  "GetDwFormName return null in Emitter::EmitDIAttrValue");
      LogInfo::MapleLogger() << "unhandled : " << maple::GetDwFormName(attr->GetDwForm()) << std::endl;
      ASSERT(0, "NYI");
  }
}

void Emitter::EmitDIDebugInfoSection(DebugInfo *mirdi) {
  /* From DWARF Standard Specification V4. 7.5.1
     collect section size */
  Emit("\t.section\t.debug_info,\"\",@progbits\n");
  /* label to mark start of the .debug_info section */
  Emit(".L" XSTR(DEBUG_INFO_0) ":\n");
  /* $ 7.5.1.1 */
  Emit("\t.4byte\t");
  EmitHexUnsigned(mirdi->GetDebugInfoLength());
  Emit(CMNT "section length\n");
  /* DWARF version. uhalf. */
  Emit("\t.2byte\t0x" XSTR(kDwarfVersion) "\n");  /* 4 for version 4. */
  /* debug_abbrev_offset. 4byte for 32-bit, 8byte for 64-bit */
  Emit("\t.4byte\t.L" XSTR(DEBUG_ABBREV_0) "\n");
  /* address size. ubyte */
  Emit("\t.byte\t0x" XSTR(kSizeOfPTR) "\n");
  /*
   * 7.5.1.2 type unit header
   * currently empty...
   *
   * 7.5.2 Debugging Information Entry (DIE)
   */
  Emitter *emitter = this;
  MapleVector<DBGAbbrevEntry*> &abbrevVec = mirdi->GetAbbrevVec();
  ApplyInPrefixOrder(mirdi->GetCompUnit(), [&abbrevVec, &emitter, &mirdi](DBGDie *die) {
    if (!die) {
      /* emit the null entry and return */
      emitter->Emit("\t.byte    0x0\n");
      return;
    }
    bool verbose = emitter->GetCG()->GenerateVerboseAsm();
    if (verbose) {
      emitter->Emit("\n");
    }
    emitter->Emit("\t.uleb128 ");
    emitter->EmitHexUnsigned(die->GetAbbrevId());
    if (verbose) {
      emitter->Emit(CMNT);
      CHECK_FATAL(maple::GetDwTagName(die->GetTag()) != nullptr,
                  "GetDwTagName(die->GetTag()) return null in Emitter::EmitDIDebugInfoSection");
      emitter->Emit(maple::GetDwTagName(die->GetTag()));
      emitter->Emit(" Offset= ");
      emitter->EmitHexUnsigned(die->GetOffset());
      emitter->Emit(" (");
      emitter->EmitDecUnsigned(die->GetOffset());
      emitter->Emit(" ),  Size= ");
      emitter->EmitHexUnsigned(die->GetSize());
      emitter->Emit(" (");
      emitter->EmitDecUnsigned(die->GetSize());
      emitter->Emit(" )\n");
    } else {
      emitter->Emit("\n");
    }
    DBGAbbrevEntry *diae = LFindAbbrevEntry(abbrevVec, die->GetAbbrevId());
    CHECK_FATAL(diae != nullptr, "diae is null in Emitter::EmitDIDebugInfoSection");
    MapleVector<uint32> &apl = diae->GetAttrPairs();  /* attribute pair list */

    std::string sfile, spath;
    if (diae->GetTag() == DW_TAG_compile_unit && sfile.empty()) {
      /* get full source path from fileMap[2] */
      if (emitter->GetFileMap().size() > k2ByteSize) {  /* have src file map */
        std::string srcPath = emitter->GetFileMap()[k2ByteSize];
        size_t t = srcPath.rfind("/");
        ASSERT(t != std::string::npos, "");
        sfile = srcPath.substr(t + 1);
        spath = srcPath.substr(0, t);
      }
    }

    for (int i = 0; i < diae->GetAttrPairs().size(); i += k2ByteSize) {
      DBGDieAttr *attr = LFindAttribute(die->GetAttrVec(), DwAt(apl[i]));
      if (!LShouldEmit(unsigned(apl[i + 1]))) {
        continue;
      }
      /* update DW_AT_name and DW_AT_comp_dir attrs under DW_TAG_compile_unit
         to be C/C++ */
      if (!sfile.empty()) {
        if (attr->GetDwAt() == DW_AT_name) {
          attr->SetId(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(sfile).GetIdx());
          emitter->GetCG()->GetMIRModule()->GetDbgInfo()->AddStrps(attr->GetId());
        } else if (attr->GetDwAt() == DW_AT_comp_dir) {
          attr->SetId(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(spath).GetIdx());
          emitter->GetCG()->GetMIRModule()->GetDbgInfo()->AddStrps(attr->GetId());
        }
      }
      emitter->Emit("\t");
      emitter->EmitDIFormSpecification(unsigned(apl[i + 1]));
      emitter->EmitDIAttrValue(die, attr, unsigned(apl[i]), diae->GetTag(), mirdi);
      if (verbose) {
        emitter->Emit(CMNT);
        emitter->Emit(maple::GetDwAtName(unsigned(apl[i])));
        emitter->Emit(" : ");
        emitter->Emit(maple::GetDwFormName(unsigned(apl[i + 1])));
        if (apl[i + 1] == DW_FORM_strp || apl[i + 1] == DW_FORM_string) {
          emitter->Emit(" : ");
          emitter->Emit(GlobalTables::GetStrTable().GetStringFromStrIdx(attr->GetId()).c_str());
        } else if (apl[i] == DW_AT_data_member_location) {
          emitter->Emit(" : ");
          emitter->Emit(apl[i + 1]).Emit("  attr= ");
          emitter->EmitHexUnsigned(uintptr_t(attr));
        }
      }
      emitter->Emit("\n");
    }
  });
}

void Emitter::EmitDIDebugAbbrevSection(DebugInfo *mirdi) {
  Emit("\t.section\t.debug_abbrev,\"\",@progbits\n");
  Emit(".L" XSTR(DEBUG_ABBREV_0) ":\n");

  /* construct a list of DI abbrev entries
     1. DW_TAG_compile_unit 0x11
     2. DW_TAG_subprogram   0x2e */
  bool verbose = GetCG()->GenerateVerboseAsm();
  for (DBGAbbrevEntry *diae : mirdi->GetAbbrevVec()) {
    if (!diae) {
      continue;
    }
    /* ID */
    if (verbose) {
      Emit("\n");
    }
    Emit("\t.uleb128 ");
    EmitHexUnsigned(diae->GetAbbrevId());
    if (verbose) {
      Emit(CMNT "Abbrev Entry ID");
    }
    Emit("\n");
    /* TAG */
    Emit("\t.uleb128 ");
    EmitHexUnsigned(diae->GetTag());
    CHECK_FATAL(maple::GetDwTagName(diae->GetTag()) != nullptr,
                "GetDwTagName return null in Emitter::EmitDIDebugAbbrevSection");
    if (verbose) {
      Emit(CMNT);
      Emit(maple::GetDwTagName(diae->GetTag()));
    }
    Emit("\n");

    MapleVector<uint32> &apl = diae->GetAttrPairs();  /* attribute pair list */
    /* children? */
    Emit("\t.byte    ");
    EmitHexUnsigned(diae->GetWithChildren());
    if (verbose) {
      Emit(diae->GetWithChildren() ? CMNT "DW_CHILDREN_yes" : CMNT "DW_CHILDREN_no");
    }
    Emit("\n");

    for (int i = 0; i < diae->GetAttrPairs().size(); i += k2ByteSize) {
      /* odd entry -- DW_AT_*, even entry -- DW_FORM_* */
      Emit("\t.uleb128 ");
      EmitHexUnsigned(apl[i]);
      CHECK_FATAL(maple::GetDwAtName(unsigned(apl[i])) != nullptr,
                  "GetDwAtName return null in Emitter::EmitDIDebugAbbrevSection");
      if (verbose) {
        Emit(CMNT);
        Emit(maple::GetDwAtName(unsigned(apl[i])));
      }
      Emit("\n");
      Emit("\t.uleb128 ");
      EmitHexUnsigned(apl[i + 1]);
      CHECK_FATAL(maple::GetDwFormName(unsigned(apl[i + 1])) != nullptr,
                  "GetDwFormName return null in Emitter::EmitDIDebugAbbrevSection");
      if (verbose) {
        Emit(CMNT);
        Emit(maple::GetDwFormName(unsigned(apl[i + 1])));
      }
      Emit("\n");
    }
    /* end of an abbreviation record */
    Emit("\t.byte    0x0\n");
    Emit("\t.byte    0x0\n");
  }
  Emit("\t.byte    0x0\n");
}

void Emitter::EmitDIDebugARangesSection() {
  Emit("\t.section\t.debug_aranges,\"\",@progbits\n");
}

void Emitter::EmitDIDebugRangesSection() {
  Emit("\t.section\t.debug_ranges,\"\",@progbits\n");
}

void Emitter::EmitDIDebugLineSection() {
  Emit("\t.section\t.debug_line,\"\",@progbits\n");
  Emit(".L" XSTR(DEBUG_LINE_0) ":\n");
}

void Emitter::EmitDIDebugStrSection() {
  Emit("\t.section\t.debug_str,\"MS\",@progbits,1\n");
  for (auto it : GetCG()->GetMIRModule()->GetDbgInfo()->GetStrps()) {
    Emit(".L" XSTR(DEBUG_STR_LABEL));
    outStream << it;
    Emit(":\n");
    const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it);
    Emit("\t.string \"").Emit(name).Emit("\"\n");
  }
}

void Emitter::FillInClassByteSize(DBGDie *die, DBGDieAttr *byteSizeAttr) {
  ASSERT(byteSizeAttr->GetDwForm() == DW_FORM_data1 || byteSizeAttr->GetDwForm() == DW_FORM_data2 ||
         byteSizeAttr->GetDwForm() == DW_FORM_data4 || byteSizeAttr->GetDwForm() == DW_FORM_data8,
         "Unknown FORM value for DW_AT_byte_size");
  if (byteSizeAttr->GetI() == static_cast<uint32>(kDbgDefaultVal)) {
    /* get class size */
    DBGDieAttr *nameAttr = LFindDieAttr(die, DW_AT_name);
    CHECK_FATAL(nameAttr != nullptr, "name_attr is nullptr in Emitter::FillInClassByteSize");
    /* hope this is a global string index as it is a type name */
    TyIdx tyIdx =
        GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(GStrIdx(nameAttr->GetId()));
    CHECK_FATAL(tyIdx.GetIdx() < Globals::GetInstance()->GetBECommon()->GetSizeOfTypeSizeTable(),
                "index out of range in Emitter::FillInClassByteSize");
    int64_t byteSize = Globals::GetInstance()->GetBECommon()->GetTypeSize(tyIdx.GetIdx());
    LUpdateAttrValue(byteSizeAttr, byteSize);
  }
}

void Emitter::SetupDBGInfo(DebugInfo *mirdi) {
  Emitter *emitter = this;
  MapleVector<DBGAbbrevEntry*> &abbrevVec = mirdi->GetAbbrevVec();
  ApplyInPrefixOrder(mirdi->GetCompUnit(), [&abbrevVec, &emitter](DBGDie *die) {
    if (!die) {
      return;
    }

    CHECK_FATAL(maple::GetDwTagName(die->GetTag()) != nullptr,
                "maple::GetDwTagName(die->GetTag()) is nullptr in Emitter::SetupDBGInfo");
    if (die->GetAbbrevId() == 0) {
      LogInfo::MapleLogger() << maple::GetDwTagName(die->GetTag()) << std::endl;
    }
    CHECK_FATAL(die->GetAbbrevId() < abbrevVec.size(), "index out of range in Emitter::SetupDBGInfo");
    ASSERT(abbrevVec[die->GetAbbrevId()]->GetAbbrevId() == die->GetAbbrevId(), "");
    DBGAbbrevEntry *diae = abbrevVec[die->GetAbbrevId()];
    switch (diae->GetTag()) {
      case DW_TAG_subprogram: {
        DBGExprLoc *exprloc = emitter->memPool->New<DBGExprLoc>(emitter->GetCG()->GetMIRModule());
        exprloc->GetSimpLoc()->SetDwOp(DW_OP_call_frame_cfa);
        die->SetAttr(DW_AT_frame_base, exprloc);
      } break;
      case DW_TAG_structure_type:
      case DW_TAG_class_type:
      case DW_TAG_interface_type: {
        DBGDieAttr *byteSizeAttr = LFindDieAttr(die, DW_AT_byte_size);
        if (byteSizeAttr) {
          emitter->FillInClassByteSize(die, byteSizeAttr);
        }
        /* get the name */
        DBGDieAttr *atName = LFindDieAttr(die, DW_AT_name);
        CHECK_FATAL(atName != nullptr, "at_name is null in Emitter::SetupDBGInfo");
        /* get the type from string name */
        TyIdx ctyIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(GStrIdx(atName->GetId()));
        MIRType *mty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ctyIdx);
        MIRStructType *sty = static_cast<MIRStructType *>(mty);
        CHECK_FATAL(sty != nullptr, "pointer cast failed");
        CHECK_FATAL(sty->GetTypeIndex().GetIdx() <
                    Globals::GetInstance()->GetBECommon()->GetSizeOfStructFieldCountTable(), "");
        int embeddedIDs = 0;
        MIRStructType *prevSubstruct = nullptr;
        for (int i = 0; i < sty->GetFields().size(); i++) {
          TyIdx fieldtyidx = sty->GetFieldsElemt(i).second.first;
          MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldtyidx);
          if (prevSubstruct) {
            embeddedIDs +=
                Globals::GetInstance()->GetBECommon()->GetStructFieldCount(prevSubstruct->GetTypeIndex().GetIdx());
          }
          prevSubstruct = fieldty->EmbeddedStructType();
          FieldID fieldID = i + embeddedIDs + 1;
          int offset = Globals::GetInstance()->GetBECommon()->GetFieldOffset(*sty, fieldID).first;
          GStrIdx fldName = sty->GetFieldsElemt(i).first;
          DBGDie *cdie = LFindChildDieWithName(die, DW_TAG_member, fldName);
          CHECK_FATAL(cdie != nullptr, "cdie is null in Emitter::SetupDBGInfo");
          DBGDieAttr *mloc = LFindDieAttr(cdie, DW_AT_data_member_location);
          CHECK_FATAL(mloc != nullptr, "mloc is null in Emitter::SetupDBGInfo");
          DBGAbbrevEntry *childDiae = abbrevVec[cdie->GetAbbrevId()];
          CHECK_FATAL(childDiae != nullptr, "child_diae is null in Emitter::SetupDBGInfo");
          LUpdateAttrValue(mloc, offset);
        }
      } break;
      default:
        break;
    }
  });

  /* compute DIE sizes and offsets */
  mirdi->ComputeSizeAndOffsets();
}

void Emitter::EmitHugeSoRoutines(bool lastRoutine) {
  if (!lastRoutine && (javaInsnCount < (static_cast<uint64>(hugeSoSeqence) *
                                        static_cast<uint64>(kHugeSoInsnCountThreshold)))) {
    return;
  }
  for (auto &target : hugeSoTargets) {
    (void)Emit("\t.section ." + std::string(namemangler::kMuidJavatextPrefixStr) + ",\"ax\"\n");
    Emit("\t.align 3\n");
    std::string routineName = target + HugeSoPostFix();
    Emit("\t.type\t" + routineName + ", %function\n");
    Emit(routineName + ":\n");
    Emit("\tadrp\tx17, :got:" + target + "\n");
    Emit("\tldr\tx17, [x17, :got_lo12:" + target + "]\n");
    Emit("\tbr\tx17\n");
    javaInsnCount += kSizeOfHugesoRoutine;
  }
  hugeSoTargets.clear();
  ++hugeSoSeqence;
}

void ImmOperand::Dump() const {
  LogInfo::MapleLogger() << "imm:" << value;
}

void LabelOperand::Emit(Emitter &emitter, const OpndProp *opndProp) const {
  (void)opndProp;
  emitter.EmitLabelRef(labelIndex);
}

void LabelOperand::Dump() const {
  LogInfo::MapleLogger() << "label:" << labelIndex;
}
}  /* namespace maplebe */
