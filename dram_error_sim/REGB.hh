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
 * @file: REGB.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * REGB declaration
 */

#ifndef __REGB_HH__
#define __REGB_HH__

#include "Bamboo.hh"
#include "ECC.hh"
#include "prior.hh"
#include "rs.hh"

/**
 * @brief Onchip (in-dram) ECC only for 64bit-width channel
 */
class OnChip64b : public ECC {
 public:
  OnChip64b();

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
 protected:
  Codec *onchip_codec;
};

/**
 * @brief Onchip (in-dram) ECC + SECDED at the rank-level for 72bit-width
 * channel
 */
class OnChip72bSECDED : public SECDED72b {
 public:
  OnChip72bSECDED();

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
 protected:
  Codec *onchip_codec;
};

/**
 * @brief Onchip (in-dram) ECC + Chipkill at the rank-level for 72bit-width
 * channel
 */
class OnChip72bAMD : public AMDChipkill72b {
 public:
  OnChip72bAMD(bool _doPostprocess = true);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
};

/**
 * @brief Onchip (in-dram) ECC + Chipkill at the rank-level for 36bit-width
 * channel
 */
class OnChip36bSDDC : public AMDChipkill72b {
 public:
  OnChip36bSDDC(bool _doPostprocess = true);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() &&
           (!fault->getIsSingleDQ() || !fault->getIsSingleBeat());
  }

 protected:
  Codec *onchip_codec;
};

/**
 * @brief Onchip (in-dram) ECC + Bamboo QPC at the rank-level for 72bit-width
 * channel
 */
class OnChip72bQPC72b : public QPC72b {
 public:
  OnChip72bQPC72b(int correction, int maxChips);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
 protected:
  Codec *onchip_codec;
};

/**
 * @brief Onchip (in-dram) ECC + REGB QPC at the rank-level for 72bit-width
 * channel
 */
class QPC72bREGB : public QPC72b {
 public:
  QPC72bREGB(bool _doFaultDiagnosis, bool _doRetire);

  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 2) ||
           (fault->getNumDQ() > 2);
  }

 protected:
  bool doFaultDiagnosis;
  Codec *secondCodec;
  Codec *thirdCodec;
  Codec *fourthCodec;
};

#endif /* __REGB_HH__ */
