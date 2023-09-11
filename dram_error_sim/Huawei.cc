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

#include "Huawei.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"

//------------------------------------------------------------------------------
Bamboo72b::Bamboo72b() : ECC(PIN) {
  codec = new RS<2, 8>("S8SC w/ H (AMD)\t", 72, 8, 4);
}

ErrorType Bamboo72b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  msg.extract(&errorBlk, PIN, 0, errorBlk.getChannelWidth());
  ErrorType result = codec->decode(&msg, &decoded, &correctedPosSet);

  if (correctedPosSet.size() > 2) {  // maxPins==2
    int chipPos = -1;
    for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend();
         it++) {
      int newChipPos = (*it) / 4;
      if (chipPos == -1) {  // first chip location
        chipPos = newChipPos;
      } else {
        if (chipPos != newChipPos) {
          correctedPosSet.clear();
          return DUE;
        }
      }
    }
  }

  return result;
}

//------------------------------------------------------------------------------
OnChip72bBamboo::OnChip72bBamboo() : Bamboo72b() {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  this->setInDRAM(DoubleDouble18);
  this->setInDRAMDown(Double);

  chipRand = false;
  setBitN(136);
}

ErrorType OnChip72bBamboo::decodeInternal(FaultDomain *fd,
                                          CacheLine &errorBlk) {
  // on-chip ECC
  // ECCWord msg = {136, 128};
  // ECCWord decoded = {136, 128};
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};

  if (errorBlk.isZero()) {
    return NE;
  }
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

    if (!msg.isZero()) {
      onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (correctedPosSet.size() != 0) {
        auto it = correctedPosSet.begin();
        if ((*it) < 72)
          errorBlk.invBit(i * 4 + (*it) % 4 +
                          ((*it) / 4) * errorBlk.getChannelWidth());
        correctedPosSet.clear();
      }
    }
  }

  if (errorBlk.isZero()) {
    return CE;
  } else {
    correctedPosSet.clear();
    ErrorType result = Bamboo72b::decodeInternal(fd, errorBlk);
    return result;
  }
}

//------------------------------------------------------------------------------
AMDChipkill40b::AMDChipkill40b(bool _doPostprocess) : ECC(AMD, _doPostprocess) {
  configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 10, 2, 1)});
}

ErrorType AMDChipkill40b::postprocess(FaultDomain *fd, ErrorType preResult) {
  if (correctedPosSet.size() > 1) {
    correctedPosSet.clear();
    return DUE;
  } else {
    return preResult;
  }
}

//------------------------------------------------------------------------------
OnChip40bSDDC::OnChip40bSDDC(bool _doPostprocess)
    : AMDChipkill40b(_doPostprocess) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  this->setInDRAM(DoubleDouble10);
  this->setInDRAMDown(Double);
  setBitN(136);
}

ErrorType OnChip40bSDDC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  // on-chip ECC
  // ECCWord msg = {136, 128};
  // ECCWord decoded = {136, 128};
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};

  if (errorBlk.isZero()) {
    return NE;
  }
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

    if (!msg.isZero()) {
      onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (correctedPosSet.size() == 1) {
        auto it = correctedPosSet.begin();
        if ((*it) < 72)
          errorBlk.invBit(i * 4 + (*it) % 4 +
                          ((*it) / 4) * errorBlk.getChannelWidth());
        correctedPosSet.clear();
      }
    }
  }

  if (errorBlk.isZero()) {
    return CE;
  } else {
    correctedPosSet.clear();
    ErrorType result = AMDChipkill40b::decodeInternal(fd, errorBlk);
    result = AMDChipkill40b::postprocess(fd, result);
    return result;
  }
}

unsigned long long OnChip40bSDDC::getInitialRetiredBlkCount(
    FaultDomain *fd, double rate) {  // Fault *fault) {
  double cellFaultRate = rate;       // fault->getCellFaultRate();
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
    assert(eccBlkSize == 72);
    double goodECCBlkProb =
        pow(1 - cellFaultRate, eccBlkSize) +
        eccBlkSize * pow(1 - cellFaultRate, eccBlkSize - 1) * cellFaultRate;
    double goodBlkProb =
        pow(goodECCBlkProb, fd->getChannelWidth() / fd->getChipWidth());
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 128;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

//------------------------------------------------------------------------------
void OnChip40bXEDx8::detectInDRAM(CacheLine &errorBlk,
                                  std::list<int> &chipLocations) {
  // ECCWord msg = {72, 64};
  // ECCWord decoded = {72, 64};
  ECCWord msg = {136, 128};
  ECCWord decoded = {136, 128};
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x8, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if ((result == CE) || (result == DUE)) {
        // catch-word collision
        unsigned long long draw =
            ((unsigned long long)rand() * ((unsigned long long)RAND_MAX + 1)) |
            rand();
        if (draw % 0x100000000ull != 0) {  // 2^-32
          // no collision
          chipLocations.push_back(i);
        }
      }
    }
  }
}

void OnChip40bXEDx8::correctInDRAM(CacheLine &errorBlk) {
  // ECCWord msg = {72, 64};
  // ECCWord decoded = {72, 64};
  ECCWord msg = {136, 128};
  ECCWord decoded = {136, 128};
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x8, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (result == CE) {
        for (int j = 0; j < 128; j++) {
          if (msg.getBit(j) != decoded.getBit(j)) {
            errorBlk.invBit(i * 8 + j % 8 +
                            (j / 8) * errorBlk.getChannelWidth());
          }
        }
        // ignore in-DRAM redundant bits
        for (int j = 128; j < 136; j++) {
          errorBlk.setBit(i * 8 + j % 8 + (j / 8) * errorBlk.getChannelWidth(),
                          0);
        }
      }
    }
  }
}

ErrorType OnChip40bXEDx8::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  // on-chip ECC
  if (errorBlk.isZero()) {
    return NE;
  }

  // in-DRAM ECC detection
  std::list<int> chipErrorDetectionLocations;
  detectInDRAM(errorBlk, chipErrorDetectionLocations);

  // 1. no detection
  if (chipErrorDetectionLocations.size() == 0) {
    // Paper Section VI
    // 1.1. check parity
    bool match = checkParity(errorBlk);

    if (match) {
      return SDC;
    } else {
      // detected by parity
      if (doFaultDiagnosis) {
        // fault diagnosis + erasure decoding
        ErrorType result = diagnoseFault(fd, errorBlk, 1);
        return result;
      } else {
        return DUE;
      }
    }
  } else if (chipErrorDetectionLocations.size() == 1 && metadataByte == 0) {
    // Paper Section V.C.2
    // 1.1. erasure decoding
    for (auto it = chipErrorDetectionLocations.cbegin();
         it != chipErrorDetectionLocations.cend(); it++) {
      int faultyChipLocation = (*it);
      // correct using erasure code
      for (int j = 0; j < 136; j++) {
        errorBlk.setBit(faultyChipLocation * 8 + j % 8 +
                            (j / 8) * errorBlk.getChannelWidth(),
                        0);
      }
    }
    if (errorBlk.isZero()) {
      return CE;
    } else {
      return SDC;
    }
  } else {
    // Paper Section VII
    // serial mode (SEC correction of on-die ECC)
    correctInDRAM(errorBlk);
    if (errorBlk.isZero()) {
      return CE;
    }

    // 1.2. check parity
    bool match = checkParity(errorBlk);

    if (match) {
      // (possibly) 1-bit error is corrected by SEC
      return SDC;
    } else {
      // detected by parity
      if (doFaultDiagnosis) {
        ErrorType result = diagnoseFault(fd, errorBlk, 1);
        return result;
      } else {
        return DUE;
      }
    }
  }
}

unsigned long long OnChip40bXEDx8::getInitialRetiredBlkCount(FaultDomain *fd,
                                                             double rate) {
  double cellFaultRate = rate;  // fault->getCellFaultRate();
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
    double goodECCBlkProb =
        pow(1 - cellFaultRate, eccBlkSize) +
        eccBlkSize * pow(1 - cellFaultRate, eccBlkSize - 1) * cellFaultRate;
    double goodBlkProb =
        pow(goodECCBlkProb, fd->getChannelWidth() / fd->getChipWidth());
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 128;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

bool OnChip40bXEDx8::checkParity(CacheLine &errorBlk) {
  ECCWord parity = {72, 64};
  ECCWord msg = {72, 64};
  // ECCWord parity = {136, 128};
  // ECCWord msg = {136, 128};

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

    if (!msg.isZero()) {
      parity ^= msg;
    }
  }
  return parity.isZero();
}

//------------------------------------------------------------------------------
Bamboo40b::Bamboo40b(int _metadataByte, bool _By8) : ECC(PIN16) {
  codec = new RS<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte,
                       (16 - _metadataByte) / 2);
  rs_dual_8 = new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte, 8);
  rs_dual_16 =
      new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte, 16);
  ErasureLocation = new std::list<int>;
  By8 = _By8;
  if (By8 == 0)
    ChipkillAvailable = (16 - _metadataByte >= 8) ? true : false;
  else
    ChipkillAvailable = (16 - _metadataByte >= 16) ? true : false;
}

ErrorType Bamboo40b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

  result = codec->decode(&msg, &decoded, &correctedPosSet);
  if ((result == CE) || (result == SDC)) {
    if (result == SDC) {
      // case that error exists in parity
      if (decoded.isZero())
        result = CE;
      else {
        printf("SDC from RS correction\n");
        msg.print();
        decoded.print();
      }
    }
  } else if (result == DUE) {
    if (ChipkillAvailable) {
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};

      ErrorType tmp_result = result;
      std::list<int> chip_list;
      int faultyChip;
      fd->FaultyChipDetect(&chip_list);
      int size = chip_list.size();
      if (size == 1 || size == 2) {  // faulty chip found
        tmp_msg.clone(&msg);
        ErasureLocation->clear();
        for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
          faultyChip = (*it);
          int perChipByte = (By8) ? 16 : 8;
          int startPos = faultyChip * perChipByte;
          for (int j = startPos; j < startPos + perChipByte; j++) {
            ErasureLocation->push_back(j);
          }
        }
        std::set<int> tmp_correctedPos;
        if (!By8 && size == 1)
          tmp_result = rs_dual_8->decode(&tmp_msg, &tmp_decoded,
                                         &tmp_correctedPos, ErasureLocation);
        else
          tmp_result = rs_dual_16->decode(&tmp_msg, &tmp_decoded,
                                          &tmp_correctedPos, ErasureLocation);

        if (tmp_result == CE) {
          this->correctedPosSet.clear();
          for (std::set<int>::iterator it = tmp_correctedPos.begin();
               it != tmp_correctedPos.end(); ++it) {
            this->correctedPosSet.insert(*it);
          }
          return CE;
        } else if (tmp_result == SDC) {
          return SDC;
        } else if (tmp_result == DUE) {
          return DUE;
        }
      } else {  // else for if(size==1)
        return DUE;
      }

    }  // ChipkillAvailable

  } else {
    assert(0);  // NE
  }

  return result;
}

//------------------------------------------------------------------------------
SDDCBamboo36b::SDDCBamboo36b() : ECC(PIN16) {
  codec = new RS<2, 8>("S8SC w/ H (AMD)\t", 72, 8, 4);
  rs_dual_8 = new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 72, 8,
                                8);  //(16-_metadataByte)/2);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Penta);
  this->setInDRAMDown(Double);
  setBitN(544);
}

ErrorType SDDCBamboo36b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

  result = codec->decode(&msg, &decoded, &correctedPosSet);
  if ((result == CE) || (result == SDC)) {
    if (result == SDC) {
      // case that error exists in parity
      if (decoded.isZero())
        result = CE;
      else {
        // printf("SDC from RS correction\n");
        // msg.print();
        // decoded.print();
      }
    }
  } else if (result == DUE) {
    ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
    ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    fd->FaultyChipDetect(&chip_list);
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int perChipByte = 8;
        int startPos = faultyChip * perChipByte;
        for (int j = startPos; j < startPos + perChipByte; j++) {
          ErasureLocation->push_back(j);
        }
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_8->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                     ErasureLocation);

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        return SDC;
      } else if (tmp_result == DUE) {
      }
    } else {  // else for if(size==1)
    }

  } else {
    assert(0);  // NE
  }

  return result;
}

//------------------------------------------------------------------------------
OnChip40bBamboo::OnChip40bBamboo(int _metadataByte, bool _By8, bool onChipECC)
    : Bamboo40b(_metadataByte, _By8) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  if (_metadataByte == 0) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(DoubleDouble10);
  } else if (_metadataByte == 2) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(DoubleDouble10);
  } else if (_metadataByte == 3) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Triple);
  } else if (_metadataByte == 4) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Triple);
  } else {
    exit(0);
  }
  onchipecc = onChipECC;
  setBitN(72);
}

ErrorType OnChip40bBamboo::decodeInternal(FaultDomain *fd,
                                          CacheLine &errorBlk) {
  // on-chip ECC
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};
  if (errorBlk.isZero()) {
    return NE;
  }
  if (onchipecc){
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

      if (!msg.isZero()) {
        ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
        if (correctedPosSet.size() == 1) {
          auto it = correctedPosSet.begin();
          errorBlk.invBit(i * 4 + (*it) % 4 +
                          ((*it) / 4) * errorBlk.getChannelWidth());
          correctedPosSet.clear();
        }
      }
    }
  }

  if (errorBlk.isZero()) {
    return CE;
  } else {
    ErrorType result = Bamboo40b::decodeInternal(fd, errorBlk);
    return result;
  }
}

unsigned long long OnChip40bBamboo::getInitialRetiredBlkCount(
    FaultDomain *fd, double rate) {  // Fault *fault) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
    assert(eccBlkSize == 72);
    double goodECCBlkProb =
        pow(1 - cellFaultRate, eccBlkSize) +
        eccBlkSize * pow(1 - cellFaultRate, eccBlkSize - 1) * cellFaultRate;
    double goodBlkProb = pow(goodECCBlkProb, fd->getChipCount());
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 128;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

//------------------------------------------------------------------------------
OnChip40bBamboox8::OnChip40bBamboox8(int _metadataByte, bool _By8)
    : Bamboo40b(_metadataByte, _By8) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  if (_metadataByte == 0) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(DoubleDouble10);
  } else {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Double);
  }
  setBitN(136);
}

ErrorType OnChip40bBamboox8::decodeInternal(FaultDomain *fd,
                                            CacheLine &errorBlk) {
  // on-chip ECC
  ECCWord msg = {136, 128};
  ECCWord decoded = {136, 128};

  if (errorBlk.isZero()) {
    return NE;
  }
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x8, i, errorBlk.getChannelWidth());

    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (correctedPosSet.size() == 1) {
        auto it = correctedPosSet.begin();
        errorBlk.invBit(i * 8 + (*it) % 8 +
                        ((*it) / 8) * errorBlk.getChannelWidth());
        correctedPosSet.clear();
      }
    }
  }

  if (errorBlk.isZero()) {
    return CE;
  } else {
    ErrorType result = Bamboo40b::decodeInternal(fd, errorBlk);
    return result;
  }
}

unsigned long long OnChip40bBamboox8::getInitialRetiredBlkCount(
    FaultDomain *fd, double rate) {  // Fault *fault) {
  double cellFaultRate = rate;       // fault->getCellFaultRate();
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
    double goodECCBlkProb =
        pow(1 - cellFaultRate, eccBlkSize) +
        eccBlkSize * pow(1 - cellFaultRate, eccBlkSize - 1) * cellFaultRate;
    double goodBlkProb = pow(goodECCBlkProb, fd->getChipCount());
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 128;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

//------------------------------------------------------------------------------
OnChip40bDUO::OnChip40bDUO(int _metadataByte) : ECC(duoBL17) {
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 84, 20, 10, 9);
  rs_dual_8 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                20 - _metadataByte, 8);
  rs_dual_16 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                 20 - _metadataByte, 16);
  rs_dual_19 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                 20 - _metadataByte, 19);
  ErasureLocation = new std::list<int>;
  if (_metadataByte == 0) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Hexa);
  } else if (_metadataByte == 2) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Penta);
  } else if (_metadataByte == 3) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Quad);
  } else if (_metadataByte == 4) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Triple);
  } else {
    printf("metadata\n");
    exit(0);
  }
  setBitN(612);
}

ErrorType OnChip40bDUO::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  // Up to 6-sym corrections
  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());
  result = codec->decode(&msg, &decoded, &correctedPosSet);

  if ((result == CE) || (result == SDC)) {
    if (result == SDC) {
      // case that error exists in parity
      if (decoded.isZero())
        result = CE;
      else {
        // printf("SDC from RS correction\n");
        // msg.print();
        // decoded.print();
      }
    }
  } else if (result == DUE) {
    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    fd->FaultyChipDetect(&chip_list);
    int size = chip_list.size();
    if (size == 1 || size == 2) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 8;
        for (int j = startPos; j < startPos + 8; j++) {
          ErasureLocation->push_back(j);
        }
      }

      CorrectByParity(&tmp_msg, &errorBlk, &chip_list);
      std::set<int> tmp_correctedPos;
      if (size == 1) {
        tmp_result = rs_dual_8->decode(&tmp_msg, &tmp_decoded,
                                       &tmp_correctedPos, ErasureLocation);
      } else if (size == 2) {
        if (rs_dual_16->symR >= rs_dual_16->symB)
          tmp_result = rs_dual_16->decode(&tmp_msg, &tmp_decoded,
                                          &tmp_correctedPos, ErasureLocation);
      } else {
        exit(0);
      }

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        return SDC;
      } else {
        if (rs_dual_19->symR >= rs_dual_19->symB) {  // when no metadata needed

          tmp_msg.clone(&msg);
          std::set<int> erasure_set;
          erasure_set.insert(80);
          erasure_set.insert(81);
          erasure_set.insert(82);
          erasure_set.insert(83);

          for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
            faultyChip = (*it);
            if (faultyChip == 0 || faultyChip == 1) {
              ErasureLocation->push_back(80);
              erasure_set.erase(80);
            }
            if (faultyChip == 2 || faultyChip == 3) {
              ErasureLocation->push_back(81);
              erasure_set.erase(81);
            }
            if (faultyChip == 4 || faultyChip == 5) {
              ErasureLocation->push_back(82);
              erasure_set.erase(82);
            }
            if (faultyChip == 6 || faultyChip == 7) {
              ErasureLocation->push_back(83);
              erasure_set.erase(83);
            }
          }
          // try all three combinations
          for (std::set<int>::iterator it = erasure_set.begin();
               it != erasure_set.end(); it++) {
            tmp_correctedPos.clear();
            ErasureLocation->push_back(*it);
            tmp_result = rs_dual_19->decode(&tmp_msg, &tmp_decoded,
                                            &tmp_correctedPos, ErasureLocation);
            if (tmp_result == CE || tmp_result == SDC)
              break;
            else {
              ErasureLocation->pop_back();
            }
          }

          if (tmp_result == CE || tmp_result == SDC)
            return tmp_result;
          else {
            return DUE;
          }
        }
      }
    } else {  // DUE?
    }
  } else {
    assert(0);  // NE
  }
  return result;
}

unsigned long long OnChip40bDUO::getInitialRetiredBlkCount(FaultDomain *fd,
                                                           double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    double goodSymProb = pow(1 - cellFaultRate, fd->getBeatHeight());
    double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) +
                         fd->getChannelWidth() *
                             pow(goodSymProb, fd->getChannelWidth() - 1) *
                             (1 - goodSymProb);

    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 64;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

void OnChip40bDUO::CorrectByParity(ECCWord *msg, Block *errorBlk,
                                   std::list<int> *chip_list) {  // int chipID){
  bool cor0, cor1, cor2, cor3;
  cor0 = cor1 = cor2 = cor3 = false;
  int offset = 10 * 16 * 4;
  int chipID[2];
  int cnt = 0;
  for (std::list<int>::iterator it = chip_list->begin(); it != chip_list->end();
       it++) {
    chipID[cnt] = (*it);
    if (*(it) == 8) {
      cor0 ^= errorBlk->getBit(offset + 8 * 4 + 0);
      cor1 ^= errorBlk->getBit(offset + 8 * 4 + 1);
      cor2 ^= errorBlk->getBit(offset + 8 * 4 + 2);
      cor3 ^= errorBlk->getBit(offset + 8 * 4 + 3);
    } else if ((*it) == 9) {
      cor0 ^= errorBlk->getBit(offset + 9 * 4 + 0);
      cor1 ^= errorBlk->getBit(offset + 9 * 4 + 1);
      cor2 ^= errorBlk->getBit(offset + 9 * 4 + 2);
      cor3 ^= errorBlk->getBit(offset + 9 * 4 + 3);
    }
    cnt++;
  }

  for (int chip = 0; chip < 8; chip++) {
    if (chip != chipID[0] && chip != chipID[1]) {
      cor0 ^= msg->getBit(offset + chip * 4 + 0);
      cor1 ^= msg->getBit(offset + chip * 4 + 1);
      cor2 ^= msg->getBit(offset + chip * 4 + 2);
      cor3 ^= msg->getBit(offset + chip * 4 + 3);
    }
  }
  cor0 ^= errorBlk->getBit(offset + 8 * 4 + 0);
  cor1 ^= errorBlk->getBit(offset + 8 * 4 + 1);
  cor2 ^= errorBlk->getBit(offset + 8 * 4 + 2);
  cor3 ^= errorBlk->getBit(offset + 8 * 4 + 3);
  cor0 ^= errorBlk->getBit(offset + 9 * 4 + 0);
  cor1 ^= errorBlk->getBit(offset + 9 * 4 + 1);
  cor2 ^= errorBlk->getBit(offset + 9 * 4 + 2);
  cor3 ^= errorBlk->getBit(offset + 9 * 4 + 3);

  msg->setBit(offset + chipID[0] * 4 + 0, cor0);
  msg->setBit(offset + chipID[0] * 4 + 1, cor1);
  msg->setBit(offset + chipID[0] * 4 + 2, cor2);
  msg->setBit(offset + chipID[0] * 4 + 3, cor3);

  if (cnt == 2) {
    msg->setBit(offset + chipID[1] * 4 + 0, cor0);
    msg->setBit(offset + chipID[1] * 4 + 1, cor1);
    msg->setBit(offset + chipID[1] * 4 + 2, cor2);
    msg->setBit(offset + chipID[1] * 4 + 3, cor3);
  }
}

//------------------------------------------------------------------------------
OnChip40bDUOx8::OnChip40bDUOx8(int _metadataByte) : ECC(duoBL17x8) {
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 84, 20, 10, 9);
  rs_dual_8 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                20 - _metadataByte, 8);
  rs_dual_16 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                 20 - _metadataByte, 16);
  rs_dual_19 = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 84,
                                 20 - _metadataByte, 19);
  ErasureLocation = new std::list<int>;
  if (_metadataByte == 0) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Triple);
  } else if (_metadataByte == 2) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Double);
  } else if (_metadataByte == 3) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(Double);
  } else {
    exit(0);
  }
  setBitN(680);
}

ErrorType OnChip40bDUOx8::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }
  // Up to 6-sym corrections
  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());
  result = codec->decode(&msg, &decoded, &correctedPosSet);
  if ((result == CE) || (result == SDC)) {
    if (result == SDC) {
      // case that error exists in parity
      if (decoded.isZero())
        result = CE;
      else {
        printf("SDC from RS correction\n");
        msg.print();
        decoded.print();
      }
    }
  } else if (result == DUE) {
    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    fd->FaultyChipDetect(&chip_list);
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 16;
        for (int j = startPos; j < startPos + 16; j++) {
          ErasureLocation->push_back(j);
        }
      }

      CorrectByParity(&tmp_msg, &errorBlk, faultyChip);  //;&chip_list);
      std::set<int> tmp_correctedPos;
      result = rs_dual_16->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                  ErasureLocation);

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        return SDC;
      }
    } else {  // DUE?
    }
  } else {
    assert(0);  // NE
  }
  return result;
}

unsigned long long OnChip40bDUOx8::getInitialRetiredBlkCount(FaultDomain *fd,
                                                             double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    double goodSymProb = pow(1 - cellFaultRate, fd->getBeatHeight());
    double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) +
                         fd->getChannelWidth() *
                             pow(goodSymProb, fd->getChannelWidth() - 1) *
                             (1 - goodSymProb);

    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 64;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

void OnChip40bDUOx8::CorrectByParity(ECCWord *msg, Block *errorBlk,
                                     int chipID) {
  bool cor0, cor1, cor2, cor3, cor4, cor5, cor6, cor7;
  cor0 = cor1 = cor2 = cor3 = cor4 = cor5 = cor6 = cor7 = false;
  int offset = 16 * 5 * 8;
  int cnt = 0;

  for (int chip = 0; chip < 4; chip++) {
    if (chip != chipID) {
      cor0 ^= msg->getBit(offset + chip * 8 + 0);
      cor1 ^= msg->getBit(offset + chip * 8 + 1);
      cor2 ^= msg->getBit(offset + chip * 8 + 2);
      cor3 ^= msg->getBit(offset + chip * 8 + 3);
      cor4 ^= msg->getBit(offset + chip * 8 + 4);
      cor5 ^= msg->getBit(offset + chip * 8 + 5);
      cor6 ^= msg->getBit(offset + chip * 8 + 6);
      cor7 ^= msg->getBit(offset + chip * 8 + 7);
      cnt++;
    }
  }
  if (cnt < 4) {
    cor0 ^= errorBlk->getBit(offset + 4 * 8 + 0);
    cor1 ^= errorBlk->getBit(offset + 4 * 8 + 1);
    cor2 ^= errorBlk->getBit(offset + 4 * 8 + 2);
    cor3 ^= errorBlk->getBit(offset + 4 * 8 + 3);
    cor4 ^= errorBlk->getBit(offset + 4 * 8 + 4);
    cor5 ^= errorBlk->getBit(offset + 4 * 8 + 5);
    cor6 ^= errorBlk->getBit(offset + 4 * 8 + 6);
    cor7 ^= errorBlk->getBit(offset + 4 * 8 + 7);
  }

  msg->setBit(offset + chipID * 8 + 0, cor0);
  msg->setBit(offset + chipID * 8 + 1, cor1);
  msg->setBit(offset + chipID * 8 + 2, cor2);
  msg->setBit(offset + chipID * 8 + 3, cor3);
  msg->setBit(offset + chipID * 8 + 4, cor4);
  msg->setBit(offset + chipID * 8 + 5, cor5);
  msg->setBit(offset + chipID * 8 + 6, cor6);
  msg->setBit(offset + chipID * 8 + 7, cor7);
}

//------------------------------------------------------------------------------
OnChip4xSEC::OnChip4xSEC(int size) : ECC(LINEAR) {
  onchip_codec = new SEC("SEC (Hsiao)\t18\t4\t", 136, 8);
  this->setInDRAM(Double);
  this->setInDRAMDown(Single);
  setBitN(136);
}

ErrorType OnChip4xSEC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {136, 128};
  ECCWord decoded = {136, 128};
  ErrorType worst = NE;
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x8, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      worst = worseErrorType(worst, result);
      if (result == DUE) return DUE;
    }
  }
  if (worst == NE) return CE;
  return worst;
}

//------------------------------------------------------------------------------
OnChip4xSEC_FLEX::OnChip4xSEC_FLEX(int size) : ECC(USEMESGCONFIG) {
  // shared pointer version
  // Use http://robotics.ucsd.edu/rr_chap13.pdf to set redundancy.
  codec_size = size;
  if (size==0){
    onchip_codec = std::make_shared<SEC>("SEC (Hsiao)\t18\t4\t", 136, 8);
    setBitN(136);
    this->setInDRAM(Double);
    this->setInDRAMDown(Single);

  } else if(size == 2){
    onchip_codec = std::make_shared<BCH<8> >("SEC (Hsiao)\t18\t4\t", 156, 28,2);
    setBitN(156);
    this->setInDRAM(Triple);
    this->setInDRAMDown(Double);

  }else if(size==3){
    onchip_codec = std::make_shared<BCH<8> >("SEC (Hsiao)\t18\t4\t", 168, 40,3);
    setBitN(168);
    this->setInDRAM(Quad);
    this->setInDRAMDown(Triple);

  }else if(size==4){
    onchip_codec = std::make_shared<BCH<8> >("SEC (Hsiao)\t18\t4\t", 184, 56,4);
    setBitN(184);
    this->setInDRAM(Penta);
    this->setInDRAMDown(Quad);
  }
    
}



ErrorType OnChip4xSEC_FLEX::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord *msg;
  ECCWord *decoded;
  if(codec_size == 0){
    msg = new ECCWord(136, 128);
    decoded = new ECCWord(136, 128);
  } else if(codec_size == 2){
    msg = new ECCWord(156, 128);
    decoded = new ECCWord(156, 128);
  } else if(codec_size == 3){
    msg = new ECCWord(168, 128);
    decoded = new ECCWord(168, 128);
  } else if(codec_size == 4){
    msg = new ECCWord(184, 128);
    decoded = new ECCWord(184, 128);
  } else{
    assert(0);
  }
  
  ErrorType worst = NE;
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg->reset();
    msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
    if (!msg->isZero()) {
      ErrorType result = onchip_codec->decode(msg, decoded, &correctedPosSet);
      worst = worseErrorType(worst, result);
    }
    // If it is all DQ errors, then it is SDC
    if (true){
      if(errorBlk.errorDQ == fd->getChipWidth())
        return SDC;
    }
  }
  delete msg;
  delete decoded;
  return worst;
}

OnChip4xSEC_FLEX::~OnChip4xSEC_FLEX() {
  // shared pointer version
  onchip_codec.reset();
}

//------------------------------------------------------------------------------
OnChip8xSECDED::OnChip8xSECDED(int size) : ECC(LINEAR) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  this->setInDRAM(Double);
  this->setInDRAMDown(Single);
  setBitN(72);
}

ErrorType OnChip8xSECDED::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIPx8, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (result == CE) {
        for (int j = 0; j < 72; j++) {
          if (msg.getBit(j) != decoded.getBit(j)) {
            errorBlk.invBit(i * 8 + j % 8 +
                            (j / 8) * errorBlk.getChannelWidth());
          }
        }
      }
      if (result == DUE) return DUE;
    }
  }
  if (!errorBlk.isZero()) return SDC;

  return CE;
}

//------------------------------------------------------------------------------
OnChip2xSEC16b::OnChip2xSEC16b(int size) : ECC(LINEAR) {
  onchip_codec = new SEC("SEC-DED (Hsiao)\t18\t4\t", 272, 9);
  this->setInDRAM(Double);
  this->setInDRAMDown(Double);
  setBitN(272);
}

ErrorType OnChip2xSEC16b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {272, 263};
  ECCWord decoded = {272, 263};
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x16, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      if (result == CE) {
        for (int j = 0; j < 256; j++) {
          if (msg.getBit(j) != decoded.getBit(j)) {
            errorBlk.invBit(i * 16 + j % 16 +
                            (j / 16) * errorBlk.getChannelWidth());
          }
        }
        // ignore in-DRAM redundant bits
        for (int j = 256; j < 272; j++) {
          errorBlk.setBit(
              i * 16 + j % 16 + (j / 16) * errorBlk.getChannelWidth(), 0);
        }
      }
      if (result == DUE) return DUE;
    }
  }
  if (!errorBlk.isZero()) return SDC;

  return CE;
}

//------------------------------------------------------------------------------
OnChip1xSEC16b::OnChip1xSEC16b(int size) : ECC(LINEAR) {
  onchip_codec = new SEC("SEC-DED (Hsiao)\t18\t4\t", 544, 10);
  this->setInDRAM(Double);
  this->setInDRAMDown(Double);
  setBitN(544);
}

ErrorType OnChip1xSEC16b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {544, 534};
  ECCWord decoded = {544, 534};
  for (int i = errorBlk.getChipCount() - 2; i >= 0; i--) {
    msg.extract(&errorBlk, ONCHIP17x32, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      for (int j = 0; j < 512; j++) {
        if (msg.getBit(j) != decoded.getBit(j)) {
          errorBlk.invBit(i * 32 + j % 32 +
                          (j / 32) * errorBlk.getChannelWidth());
        }
      }
      // ignore in-DRAM redundant bits
      for (int j = 512; j < 544; j++) {
        errorBlk.setBit(i * 32 + j % 32 + (j / 32) * errorBlk.getChannelWidth(),
                        0);
      }
      if (result == DUE) return DUE;
    }
  }
  if (!errorBlk.isZero()) return SDC;

  return CE;
}

//------------------------------------------------------------------------------
OnChipBCHTriple::OnChipBCHTriple(bool _parityOn) : ECC(LINEAR) {
  codec = new BCH<10>("TEC BCH", 544, 30, 3);
  this->setInDRAM(Quad);
  this->setInDRAMDown(Triple);
  setBitN(544);
  parityOn = _parityOn;
}

ErrorType OnChipBCHTriple::decodeInternal(FaultDomain *fd,
                                          CacheLine &errorBlk) {
  ECCWord msg = {544, 514};
  ECCWord decoded = {544, 514};

  msg.extract(&errorBlk, ONCHIP17x32, 0, errorBlk.getChannelWidth());

  ErrorType result = codec->decode(&msg, &decoded);

  if (parityOn) {
    if (result != DUE) {  // parity check
      bool parity0, parity1;
      parity0 = parity1 = false;
      for (int i = 0; i < 544; i++) {
        if (i % 2 == 0)
          parity0 ^= decoded.getBit(i);
        else
          parity1 ^= decoded.getBit(i);
      }
      if (parity0 || parity1) return DUE;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
OnChipBCHHexa::OnChipBCHHexa(bool _parityOn) : ECC(LINEAR) {
  codec = new BCH<10>("HEC BCH", 576, 60, 6);
  this->setInDRAM(Septa);
  this->setInDRAMDown(Hexa);
  setBitN(576);
  parityOn = _parityOn;
}

ErrorType OnChipBCHHexa::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {576, 516};
  ECCWord decoded = {576, 516};

  msg.extract(&errorBlk, ONCHIP18x32, 0, errorBlk.getChannelWidth());

  ErrorType result = codec->decode(&msg, &decoded);

  if (parityOn) {
    if (result != DUE) {  // naive parity check
      bool parity0, parity1, parity2, parity3;
      parity0 = parity1 = parity2 = parity3 = false;
      for (int i = 0; i < 576; i++) {
        switch (i % 4) {
          case 0:
            parity0 ^= decoded.getBit(i);
            break;
          case 1:
            parity1 ^= decoded.getBit(i);
            break;
          case 2:
            parity2 ^= decoded.getBit(i);
            break;
          case 3:
            parity3 ^= decoded.getBit(i);
            break;
          default:
            assert(0);
        }
      }
      if (parity0 || parity1) return DUE;
    }
  }
  return result;
}

//------------------------------------------------------------------------------
OnChipNone::OnChipNone() : ECC(LINEAR) {
  onchip_codec = new SEC("SEC-DED (Hsiao)\t18\t4\t", 136, 8);
  this->setInDRAM(Septa);
  this->setInDRAMDown(Septa);
  setBitN(544);
}

ErrorType OnChipNone::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {544, 534};
  ECCWord decoded = {544, 534};

  msg.extract(&errorBlk, ONCHIP17x32, 0, errorBlk.getChannelWidth());
  if (!msg.isZero()) {
    // ErrorType result = onchip_codec->decode(&msg, &decoded,
    // &correctedPosSet);
    int error_bit_count = 0;
    for (int i = 0; i < 544; i++) {
      if (msg.getBit(i)) error_bit_count++;
    }
    if (error_bit_count == 0)
      return CE;
    else
      return DUE;
  } else
    return CE;
}

//------------------------------------------------------------------------------
Sym16bBB72b::Sym16bBB72b() : ECC(PIN) {
  codec = new RS<2, 16>("S8SC w/ H (AMD)\t", 36, 3, 1);
  rs_dual = new RS_DUAL<2, 16>("S8SC w/ H (AMD)\t", 36, 3, 1);
  ErasureLocation = new std::list<int>;
}

ErrorType Sym16bBB72b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  msg.extract(&errorBlk, PIN, 0, errorBlk.getChannelWidth());
  ErrorType result = codec->decode(&msg, &decoded, &correctedPosSet);

  if (result == DUE) {
    ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
    ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    fd->FaultyChipDetect(&chip_list);
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int j = 2 * faultyChip;
        ErasureLocation->push_back(j);
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                   ErasureLocation);

      result = tmp_result;

      /*
      if(tmp_result==CE){
              return CE;
      }else if(tmp_result==SDC){

              return SDC;
      }else if(tmp_result==DUE){
//					printf("j: %i\ttmp_result: %i\n",
2*faultyChip, tmp_result);
//					printf("msg:\n");tmp_msg.print();
// printf("decoded:\n");tmp_decoded.print();
      }
*/
    } else {  // else for if(size==1)
    }
  }

  // no post-processing at this moment
  //    if (correctedPosSet.size() > 2) {//maxPins==2
  //        int chipPos = -1;
  //        for (auto it = correctedPosSet.cbegin(); it !=
  //        correctedPosSet.cend(); it++) {
  //            int newChipPos = (*it)/4;
  //            if (chipPos == -1) {    // first chip location
  //                chipPos = newChipPos;
  //            } else {
  //                if (chipPos != newChipPos) {
  //                    correctedPosSet.clear();
  //                    return DUE;
  //                }
  //            }
  //        }
  //    }

  return result;
}

OnChipSym16bBB72b::OnChipSym16bBB72b(bool onchip) : Sym16bBB72b() {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  this->setInDRAM(DoubleDouble18);
  this->setInDRAMDown(Double);

  chipRand = false;
  setBitN(136);
  _onchip = onchip;
}

ErrorType OnChipSym16bBB72b::decodeInternal(FaultDomain *fd,
                                            CacheLine &errorBlk) {
  // on-chip ECC
  // ECCWord msg = {136, 128};
  // ECCWord decoded = {136, 128};
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};

  if (errorBlk.isZero()) {
    return NE;
  }
  if (_onchip) {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

      if (!msg.isZero()) {
        onchip_codec->decode(&msg, &decoded, &correctedPosSet);
        if (correctedPosSet.size() != 0) {
          auto it = correctedPosSet.begin();
          if ((*it) < 32)
            errorBlk.invBit(i * 4 + (*it) % 4 +
                            ((*it) / 4) * errorBlk.getChannelWidth());
          correctedPosSet.clear();
        }
      }
    }
  }
  if (errorBlk.isZero()) {
    return CE;
  } else {
    correctedPosSet.clear();
    ErrorType result = Sym16bBB72b::decodeInternal(fd, errorBlk);
    return result;
  }
}

//------------------------------------------------------------------------------
Sym8bBB72b::Sym8bBB72b(int _metadataByte) : ECC(PIN) {
  codec = new RS<2, 8>("S8SC w/ H (AMD)\t", 72, 8, 4);
  rs_dual = new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 72, 8 - _metadataByte, 4);
  ErasureLocation = new std::list<int>;
  // this->setInDRAM(Penta);
  // this->setInDRAMDown(Double);
  setBitN(576);
}

ErrorType Sym8bBB72b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

  result = codec->decode(&msg, &decoded, &correctedPosSet);

  if (result == DUE) {
    ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
    ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    fd->FaultyChipDetect(&chip_list);
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int perChipByte = 4;
        int startPos = faultyChip * perChipByte;
        for (int j = startPos; j < startPos + perChipByte; j++) {
          ErasureLocation->push_back(j);
        }
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                   ErasureLocation);

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        return SDC;
      } else if (tmp_result == DUE) {
      }
    } else {  // else for if(size==1)
    }
  }

  return result;
}

OnChip8bBB72b::OnChip8bBB72b(bool onchip, int metadataByte)
    : Sym8bBB72b(metadataByte) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  this->setInDRAM(DoubleDouble18);
  this->setInDRAMDown(Double);

  chipRand = false;
  setBitN(136);
  _onchip = onchip;
}

ErrorType OnChip8bBB72b::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  // on-chip ECC
  // ECCWord msg = {136, 128};
  // ECCWord decoded = {136, 128};
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};

  if (errorBlk.isZero()) {
    return NE;
  }
  if (_onchip) {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());

      if (!msg.isZero()) {
        onchip_codec->decode(&msg, &decoded, &correctedPosSet);
        if (correctedPosSet.size() != 0) {
          auto it = correctedPosSet.begin();
          if ((*it) < 32)
            errorBlk.invBit(i * 4 + (*it) % 4 +
                            ((*it) / 4) * errorBlk.getChannelWidth());
          correctedPosSet.clear();
        }
      }
    }
  }
  if (errorBlk.isZero()) {
    return CE;
  } else {
    correctedPosSet.clear();
    ErrorType result = Sym8bBB72b::decodeInternal(fd, errorBlk);
    return result;
  }
}


//------------------------------------------------------------------------------
Bamboo40b_FLEX::Bamboo40b_FLEX(int _metadataByte, bool _By8) : ECC(PIN16) {
  codec = new RS<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte,
                       (16 - _metadataByte) / 2);
  rs_dual_8 = new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte, 8);
  rs_dual_16 =
      new RS_DUAL<2, 8>("S8SC w/ H (AMD)\t", 80, 16 - _metadataByte, 16);
  ErasureLocation = new std::list<int>;
  By8 = _By8;
  if (By8 == 0)
    ChipkillAvailable = (16 - _metadataByte >= 8) ? true : false;
  else
    ChipkillAvailable = (16 - _metadataByte >= 16) ? true : false;
}

Bamboo40b_FLEX::~Bamboo40b_FLEX(){
  delete codec;
  delete rs_dual_8;
  delete rs_dual_16;
  delete ErasureLocation;
}

ErrorType Bamboo40b_FLEX::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  msg.extract(&errorBlk, USEMESGCONFIG, -1, errorBlk.getChannelWidth(),errorBlk.get_messageConfig());

  result = codec->decode(&msg, &decoded, &correctedPosSet);
  if ((result == CE) || (result == SDC)) {
    if (result == SDC) {
      // case that error exists in parity
      if (decoded.isZero())
        result = CE;
      else {
        printf("SDC from RS correction\n");
        msg.print();
        decoded.print();
      }
    }
  } else if (result == DUE) {
    if (ChipkillAvailable) {
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};

      ErrorType tmp_result = result;
      std::list<int> chip_list;
      int faultyChip;
      fd->FaultyChipDetect(&chip_list);
      int size = chip_list.size();
      if (size == 1 || size == 2) {  // faulty chip found
        tmp_msg.clone(&msg);
        ErasureLocation->clear();
        for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
          faultyChip = (*it);
          int perChipByte = (By8) ? 16 : 8;
          int startPos = faultyChip * perChipByte;
          for (int j = startPos; j < startPos + perChipByte; j++) {
            ErasureLocation->push_back(j);
          }
        }
        std::set<int> tmp_correctedPos;
        if (!By8 && size == 1)
          tmp_result = rs_dual_8->decode(&tmp_msg, &tmp_decoded,
                                         &tmp_correctedPos, ErasureLocation);
        else
          tmp_result = rs_dual_16->decode(&tmp_msg, &tmp_decoded,
                                          &tmp_correctedPos, ErasureLocation);

        if (tmp_result == CE) {
          this->correctedPosSet.clear();
          for (std::set<int>::iterator it = tmp_correctedPos.begin();
               it != tmp_correctedPos.end(); ++it) {
            this->correctedPosSet.insert(*it);
          }
          return CE;
        } else if (tmp_result == SDC) {
          return SDC;
        } else if (tmp_result == DUE) {
          return DUE;
        }
      } else {  // else for if(size==1)
        return DUE;
      }

    }  // ChipkillAvailable

  } else {
    assert(0);  // NE
  }

  return result;
}



//------------------------------------------------------------------------------
OnChip40bBamboo_FLEX::OnChip40bBamboo_FLEX(int _metadataByte, bool _By8, bool onChipECC)
    : Bamboo40b_FLEX(_metadataByte, _By8) {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  if (_metadataByte == 0) {
    this->setInDRAM(DoubleDouble9);
    this->setInDRAMDown(Double);
  } else {
    exit(0);
  }
  onchipecc = onChipECC;
  setBitN(72);
}

OnChip40bBamboo_FLEX::~OnChip40bBamboo_FLEX(){
  delete onchip_codec;
}

ErrorType OnChip40bBamboo_FLEX::decodeInternal(FaultDomain *fd,
                                          CacheLine &errorBlk) {
  // on-chip ECC
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};
  MSGConfig IECC_message = MSGConfig(16,16,0,8,1,errorBlk.getChipWidth()-1,errorBlk.getChipCount(),1,EXTRAPIN);

  if (errorBlk.isZero()) {
    return NE;
  }
  if (onchipecc){
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg.extract(&errorBlk, USEMESGCONFIG, i, errorBlk.getChannelWidth(),IECC_message);

      if (!msg.isZero()) {
        ErrorType result = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
        if (correctedPosSet.size() == 1) {
          auto it = correctedPosSet.begin();
          int width = IECC_message.get_extrapin();
          int height = IECC_message.get_height_base();
          errorBlk.invBit(i * width + ((*it) % height)*errorBlk.getChannelWidth() + ((*it)/height));
          correctedPosSet.clear();
        }
      }
    }
  }

  if (errorBlk.isZero()) {
    return CE;
  } else {
    ErrorType result = Bamboo40b_FLEX::decodeInternal(fd, errorBlk);
    return result;
  }
}

unsigned long long OnChip40bBamboo_FLEX::getInitialRetiredBlkCount(
    FaultDomain *fd, double rate) {  // Fault *fault) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int eccBlkSize = fd->getChipWidth() * fd->getBeatHeight();
    double goodECCBlkProb =
        pow(1 - cellFaultRate, eccBlkSize) +
        eccBlkSize * pow(1 - cellFaultRate, eccBlkSize - 1) * cellFaultRate;
    double goodBlkProb = pow(goodECCBlkProb, fd->getChipCount());
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 128;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

LPDDR5_ONCHIP::~LPDDR5_ONCHIP(){
  onchip_codec.reset();
}

//------------------------------------------------------------------------------
LPDDR5_ONCHIP::LPDDR5_ONCHIP(int size) : ECC(USEMESGCONFIG) {
  // shared pointer version
  codec_size = size;
  if (size==0){
    onchip_codec = std::make_shared<SEC>("SEC (Hsiao)\t18\t4\t", 136, 8);
    setBitN(136);
    this->setInDRAM(Double);
    this->setInDRAMDown(Single);
  } else if(size == 1){
    onchip_codec = std::make_shared< RS<2,8> >("SEC (Hsiao)\t18\t4\t", 34, 2, 1);
    setBitN(272); // BCH(274,256) is minimum DEC
    this->setInDRAM(Double);
    this->setInDRAMDown(Single);
  } else if(size == 2){
    onchip_codec = std::make_shared<RS<2,8> >("RS\t2\t8\t", 36, 4,2);
    setBitN(288);
    this->setInDRAM(Triple);
    this->setInDRAMDown(Double);

  }
    
}



ErrorType LPDDR5_ONCHIP::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord *msg;
  ECCWord *decoded;
  if(codec_size == 0){
    // Two SEC on chip ECC
    msg = new ECCWord(272, 256);
    decoded = new ECCWord(272, 256);
  } else if(codec_size == 1){
    // RS8 on chip ECC
    msg = new ECCWord(272, 256);
    decoded = new ECCWord(272, 256);
  } else if(codec_size == 2){
    // RS8(36,32)
    msg = new ECCWord(288, 256);
    decoded = new ECCWord(288, 256);
  } else if(codec_size == 3){
    // 2 RS8(18,16)
    msg = new ECCWord(288, 256);
    decoded = new ECCWord(288, 256);
  } else if(codec_size == 4){
    // RS16(18,16)
    msg = new ECCWord(288, 256);
    decoded = new ECCWord(288, 256);
  } else{
    assert(0);
  }
  
  ErrorType worst = NE;
  if(codec_size == 0){
    // Two SEC on chip ECC
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ECCWord msg1 = {136, 128};
        ECCWord msg2 = {136, 128};
        ECCWord decoded1 = {136, 128};
        ECCWord decoded2 = {136, 128};
        msg1.copyfromN(msg,0);
        msg2.copyfromN(msg,136);
        ErrorType result = onchip_codec->decode(&msg1, &decoded1, &correctedPosSet);
        worst = worseErrorType(worst, result);
        result = onchip_codec->decode(&msg2, &decoded2, &correctedPosSet);
        worst = worseErrorType(worst, result);
      }
      // If it is all DQ errors, then it is SDC
      if (true){
        if(errorBlk.errorDQ >= fd->getChipWidth()-2)
          worst = SDC;
      }
    }
  } else {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      decoded->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ErrorType result = onchip_codec->decode(msg, decoded, &correctedPosSet);
        worst = worseErrorType(worst, result);
      }

      // If it is all DQ errors, then it is SDC
      if (true){
        if (codec_size == 2){
          if (!decoded->isZero(0,16)){
            worst = DUE;
          }
        } else if (codec_size == 1){
          if (!decoded->isZero(0,8)){
            worst = DUE;
          }
        }      
        else if(errorBlk.errorDQ >= fd->getChipWidth()-2)
          worst = SDC;
      }
    }
  }
  delete msg;
  delete decoded;
  return worst;
}


LPDDR5_LINK_ONCHIP::~LPDDR5_LINK_ONCHIP(){
  codec.reset();
}

//------------------------------------------------------------------------------
LPDDR5_LINK_ONCHIP::LPDDR5_LINK_ONCHIP(int size) : LPDDR5_ONCHIP(size) {
  // shared pointer version
  codec = std::make_shared<SEC>("SEC (Hsiao)\t18\t4\t", 136, 8); 
    
}



ErrorType LPDDR5_LINK_ONCHIP::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord *msg;
  ECCWord *decoded;
  if(codec_size == 0){
    msg = new ECCWord(272, 256);
    decoded = new ECCWord(272, 256);
  } else if(codec_size == 1){
    msg = new ECCWord(288, 256);
    decoded = new ECCWord(288, 256);
  } else if(codec_size == 2){
    msg = new ECCWord(288, 256);
    decoded = new ECCWord(288, 256);
  } else{
    assert(0);
  }
  
  ErrorType worst = NE;
  if(codec_size == 0){
    // Two SEC on chip ECC
    auto msgconfig = errorBlk.get_messageConfig();
    msgconfig.set_msg_height_base(8);
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ECCWord msg1 = {136, 128};
        ECCWord msg2 = {136, 128};
        ECCWord decoded1 = {136, 128};
        ECCWord decoded2 = {136, 128};
        msg1.copyfromN(msg,0);
        msg2.copyfromN(msg,136);
        ErrorType result = onchip_codec->decode(&msg1, &decoded1, &correctedPosSet);
        worst = worseErrorType(worst, result);
        result = onchip_codec->decode(&msg2, &decoded2, &correctedPosSet);
        worst = worseErrorType(worst, result);
      }
      // If it is all DQ errors, then it is SDC
      if (true){
        if(errorBlk.errorDQ == fd->getChipWidth())
          return SDC;
      }
    }
  } else {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ErrorType result = onchip_codec->decode(msg, decoded, &correctedPosSet);
        worst = worseErrorType(worst, result);
      }
      // If it is all DQ errors, then it is SDC
      if (true){
        if(errorBlk.errorDQ == fd->getChipWidth())
          return SDC;
      }
    }
  }
  delete msg;
  delete decoded;
  return worst;
}
