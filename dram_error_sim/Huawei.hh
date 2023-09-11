/*
Copyright 2023, The University of Texas at Austin
All rights reserved.

THIS FILE IS PART OF THE DRAM FAULT ERROR SIMULATION FRAMEWORK

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:


1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.


2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.


3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file: Huawei.hh
 * @author: Seong-Lyong Gong
 * Declaration of DUO variations (requeted by Huawei). Each class represent one
 * configuration to be tested.
 */
#ifndef __HUAWEI_HH__
#define __HUAWEI_HH__

#include "Bamboo.hh"
#include "ECC.hh"
#include "XED.hh"
#include "bch.hh"
#include "prior.hh"
#include "rs.hh"
#include "sec.hh"

/**
 * @brief Bamboo for 72bit-width channel X4 ECC DIMMs.
 */
class Bamboo72b : public ECC {
 public:
  Bamboo72b();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  RS<2, 8> *codec;
};

/**
 * @brief Bamboo for 72bit-width channel with on-chip (in-dram) ECC.
 */
class OnChip72bBamboo : public Bamboo72b {
 public:
  OnChip72bBamboo();

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
};

/**
 * @brief AMD Chipkill for 40bit-width channel
 */

class AMDChipkill40b : public ECC {
 public:
  AMDChipkill40b(bool _doPostprocess = true);
  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief AMD Chipkill for 40bit-width channel with on-chip (in-dram) ECC.
 */
class OnChip40bSDDC : public AMDChipkill40b {
 public:
  OnChip40bSDDC(bool _doPostprocess = true);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
};

/**
 * @brief XED for 40bit-width channel with on-chip (in-dram) ECC.
 */
class OnChip40bXEDx8 : public XED {
 public:
  OnChip40bXEDx8(bool _doFaultDiagnosis = true, int _metadataByte = 3)
      : XED(_doFaultDiagnosis) {
    metadataByte = _metadataByte;
    if (metadataByte == 0) {
      setInDRAM(DoubleDouble10);
      setInDRAMDown(Double);
    } else if (metadataByte == 3) {
      setInDRAM(Double);
      setInDRAMDown(Double);
    } else {
      exit(0);
    }
    onchip_codec = new CRC8_ATM("CRC8-ATM\t18\t4\t", 136, 8);
  }
  int metadataByte;
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  void detectInDRAM(CacheLine &errorBlk, std::list<int> &chipLocations);
  void correctInDRAM(CacheLine &errorBlk);
  bool checkParity(CacheLine &errorBlk);
};

/**
 * @brief Bamboo for 40bit-width channel X4 ECC DIMMs.
 */
class Bamboo40b : public ECC {
 public:
  Bamboo40b(int metadataByte, bool By8);
  // ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual_8;
  RS_DUAL<2, 8> *rs_dual_16;
  std::list<int> *ErasureLocation;
  bool ChipkillAvailable;
  bool By8;  //
};

/**
 * @brief Bamboo for 40bit-width channel X4 ECC DIMMs.
 */
class Bamboo40b_FLEX : public ECC {
 public:
  Bamboo40b_FLEX(int metadataByte, bool By8);
  // ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  ~Bamboo40b_FLEX();
 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual_8;
  RS_DUAL<2, 8> *rs_dual_16;
  std::list<int> *ErasureLocation;
  bool ChipkillAvailable;
  bool By8;  //
};

/**
 * @brief Bamboo for 40bit-width channel with on-chip (in-dram) ECC.
 */
class OnChip40bBamboo : public Bamboo40b {
 public:
  OnChip40bBamboo(int metadataByte = 3, bool By8 = false, bool onChipECC=true);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
  bool onchipecc;
};

/**
 * @brief Bamboo for 40bit-width channel with on-chip (in-dram) ECC.
 * with flexable message type
 * Input message should be rank level type
 */
class OnChip40bBamboo_FLEX : public Bamboo40b_FLEX {
 public:
  OnChip40bBamboo_FLEX(int metadataByte = 3, bool By8 = false, bool onChipECC=true);
  ~OnChip40bBamboo_FLEX();

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
  bool onchipecc;
};

/**
 * @brief Bamboo for 40bit-width channel (x8 device) with on-chip (in-dram) ECC.
 */
class OnChip40bBamboox8 : public Bamboo40b {
 public:
  OnChip40bBamboox8(int metadataByte = 3, bool By8 = true);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
};

/**
 * @brief Bamboo for 36bit-width channel - SDDC is doable
 */
class SDDCBamboo36b : public ECC {
 public:
  SDDCBamboo36b();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual_8;
  // RS_DUAL<2,8> *rs_dual_16;
  std::list<int> *ErasureLocation;
};

/**
 * @brief DUO for 40bit-width channel with on-chip (in-dram) ECC.
 */
class OnChip40bDUO : public ECC {
 public:
  OnChip40bDUO(int metadataByte = 3);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  void CorrectByParity(ECCWord *msg, Block *errorBlk,
                       std::list<int> *chip_list);

 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual_8;
  RS_DUAL<2, 8> *rs_dual_16;
  RS_DUAL<2, 8> *rs_dual_19;
  std::list<int> *ErasureLocation;
};

/**
 * @brief DUO for 40bit-width channel (x8 device) with on-chip (in-dram) ECC.
 */
class OnChip40bDUOx8 : public ECC {
 public:
  OnChip40bDUOx8(int metadataByte = 3);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  void CorrectByParity(ECCWord *msg, Block *errorBlk, int chipID);

 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual_8;
  RS_DUAL<2, 8> *rs_dual_16;
  RS_DUAL<2, 8> *rs_dual_19;
  std::list<int> *ErasureLocation;
};

/**
 * @brief 4x SEC with on-chip redundnacy (in-dram ECC option without rank-level
 * ECC).
 */
class OnChip4xSEC : public ECC {
 public:
  OnChip4xSEC(int size);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *onchip_codec;
};

/**
 * @brief 4x SEC with on-chip redundnacy (in-dram ECC option without rank-level
 * ECC).
 */
class OnChip4xSEC_FLEX : public ECC {
 public:
  OnChip4xSEC_FLEX(int size);
  ~OnChip4xSEC_FLEX();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  // make onchip codec with shared pointer
  int codec_size;
  std::shared_ptr<Codec> onchip_codec;
};

/**
 * @brief 8x SECDED with on-chip redundnacy.
 */
class OnChip8xSECDED : public ECC {
 public:
  OnChip8xSECDED(int size);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *onchip_codec;
};

/**
 * @brief 2x SEC with on-chip redundnacy.
 */
class OnChip2xSEC16b : public ECC {
 public:
  OnChip2xSEC16b(int size);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *onchip_codec;
};

/**
 * @brief 1x SEC with on-chip redundnacy.
 */
class OnChip1xSEC16b : public ECC {
 public:
  OnChip1xSEC16b(int size);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *onchip_codec;
};

/**
 * @brief DUO VRT (BCH 3-correctable) with on-chip redundnacy.
 */
class OnChipBCHTriple : public ECC {
 public:
  OnChipBCHTriple(bool _parityOn);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *codec;
  bool parityOn;
};

/**
 * @brief DUO VRT (BCH 6-correctable) with on-chip redundnacy of 12.5\%.
 */
class OnChipBCHHexa : public ECC {
 public:
  OnChipBCHHexa(bool _parityOn);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *codec;
  bool parityOn;
};

/**
 * @brief No ECC for comparison
 */
class OnChipNone : public ECC {
 public:
  OnChipNone();

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  Codec *onchip_codec;
};

class Sym16bBB72b : public ECC {
 public:
  Sym16bBB72b();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  std::list<int> *ErasureLocation;

 protected:
  RS<2, 16> *codec;
  RS_DUAL<2, 16> *rs_dual;
};

class OnChipSym16bBB72b : public Sym16bBB72b {
 public:
  OnChipSym16bBB72b(bool onchip);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
  bool _onchip;
};

class Sym8bBB72b : public ECC {
 public:
  Sym8bBB72b(int metadataByte);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  RS<2, 8> *codec;
  RS_DUAL<2, 8> *rs_dual;
  std::list<int> *ErasureLocation;
};

class OnChip8bBB72b : public Sym8bBB72b {
 public:
  OnChip8bBB72b(bool onchip, int metadataByte);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
  bool _onchip;
};

/**
 * @brief LPDDR5 onchip ECC
 */
class LPDDR5_ONCHIP : public ECC {
 public:
  LPDDR5_ONCHIP(int size);
  ~LPDDR5_ONCHIP();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  // make onchip codec with shared pointer
  int codec_size;
  std::shared_ptr<Codec> onchip_codec;
};

/**
 * @brief LPDDR5 Link + onchip ECC
 */
class LPDDR5_LINK_ONCHIP : public LPDDR5_ONCHIP {
 public:
  LPDDR5_LINK_ONCHIP(int size);
  ~LPDDR5_LINK_ONCHIP();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

 protected:
  // make onchip codec with shared pointer
  std::shared_ptr<Codec> codec;
};

#endif /* __HUAWEI_HH__ */
