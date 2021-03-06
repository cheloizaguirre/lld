//===--------- lib/ReaderWriter/ELF/ARM/ARMTargetHandler.h ----------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_READER_WRITER_ELF_ARM_ARM_TARGET_HANDLER_H
#define LLD_READER_WRITER_ELF_ARM_ARM_TARGET_HANDLER_H

#include "ARMELFFile.h"
#include "ARMRelocationHandler.h"
#include "ELFReader.h"
#include "TargetLayout.h"
#include "llvm/ADT/Optional.h"

namespace lld {
namespace elf {
class ARMLinkingContext;

template <class ELFT> class ARMTargetLayout : public TargetLayout<ELFT> {
public:
  ARMTargetLayout(ARMLinkingContext &ctx) : TargetLayout<ELFT>(ctx) {}

  uint64_t getGOTSymAddr() {
    if (!_gotSymAddr.hasValue()) {
      AtomLayout *gotAtom = this->findAbsoluteAtom("_GLOBAL_OFFSET_TABLE_");
      _gotSymAddr = gotAtom ? gotAtom->_virtualAddr : 0;
    }
    return *_gotSymAddr;
  }

  uint64_t getTPOffset() {
    if (_tpOff.hasValue())
      return *_tpOff;

    for (const auto &phdr : *this->_programHeader) {
      if (phdr->p_type == llvm::ELF::PT_TLS) {
        _tpOff = llvm::RoundUpToAlignment(TCB_SIZE, phdr->p_align);
        return *_tpOff;
      }
    }
    llvm_unreachable("TLS segment not found");
  }

  bool target1Rel() const { return this->_ctx.armTarget1Rel(); }

private:
  // TCB block size of the TLS.
  enum { TCB_SIZE = 0x8 };

private:
  // Cached value of the GOT origin symbol address.
  llvm::Optional<uint64_t> _gotSymAddr;

  // Cached value of the TLS offset from the $tp pointer.
  llvm::Optional<uint64_t> _tpOff;
};

class ARMTargetHandler final : public TargetHandler {
  typedef llvm::object::ELFType<llvm::support::little, 2, false> ELFTy;
  typedef ELFReader<ELFTy, ARMLinkingContext, ARMELFFile> ObjReader;
  typedef ELFReader<ELFTy, ARMLinkingContext, DynamicFile> DSOReader;

public:
  ARMTargetHandler(ARMLinkingContext &ctx);

  const TargetRelocationHandler &getRelocationHandler() const override {
    return *_relocationHandler;
  }

  std::unique_ptr<Reader> getObjReader() override {
    return llvm::make_unique<ObjReader>(_ctx);
  }

  std::unique_ptr<Reader> getDSOReader() override {
    return llvm::make_unique<DSOReader>(_ctx);
  }

  std::unique_ptr<Writer> getWriter() override;

private:
  ARMLinkingContext &_ctx;
  std::unique_ptr<ARMTargetLayout<ELFTy>> _targetLayout;
  std::unique_ptr<ARMTargetRelocationHandler> _relocationHandler;
};

} // end namespace elf
} // end namespace lld

#endif // LLD_READER_WRITER_ELF_ARM_ARM_TARGET_HANDLER_H
