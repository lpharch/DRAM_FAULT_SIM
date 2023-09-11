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
 * @file: XED.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * XED declaration
 * Prashant J. Nair, Vilas Sridharan, and Moinuddin K. Qureshi, "XED: Exposing
 * On-Die Error Detection Information for Strong Memory Reliability," ISCA 2016.
 */

#ifndef __XED_HH__
#define __XED_HH__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <random>

#include "Bamboo.hh"
#include "Config.hh"
#include "DomainGroup.hh"
#include "Tester.hh"
#include "prior.hh"

/**
 * @brief CRC8_ATM (8-bit CRC) delaration
 */
class CRC8_ATM : public Codec {
 public:
  CRC8_ATM(const char *name, int _bitN, int _bitR);
  void encode(Block *data, ECCWord *encoded);
  ErrorType decode(ECCWord *msg, ECCWord *decoded,
                   std::set<int> *correctedPos = NULL);

 protected:
  unsigned char correctionTable[256];
};

/**@class XED
 * \brief Parent XED class
 */
class XED : public ECC {
 public:
  XED(bool _doFaultDiagnosis);
  //! Fault diagnosis
  /*! \param fd FaultDoamin pointer
                  \param errorBlk CacheLine reference
                  \parm erasures number of available erasure corrections
          */
  ErrorType diagnoseFault(FaultDomain *fd, CacheLine &errorBlk, int erasures);

 protected:
  virtual void detectInDRAM(
      CacheLine &errorBlk,
      std::list<int> &chipLocations) = 0;  //!< detect by in-dram ECC
  virtual void correctInDRAM(
      CacheLine &errorBlk) = 0;  //!< correct by in-dram ECC
  virtual bool checkParity(
      CacheLine &errorBlk) = 0;  //!< parity check for avoiding miscorrection
  Codec *onchip_codec;           //!< on-chip ECC pointer
  bool doFaultDiagnosis;         //!< doing fault diagnosis or not
};

/**
 * XED SDDC
 * @brief XED configuration that SDDC is doable
 */
class XED_SDDC : public XED {
 public:
  XED_SDDC(bool _doFaultDiagnosis = true) : XED(_doFaultDiagnosis) {
    setInDRAM(DoubleSingle9);
    setInDRAMDown(Double);
  }
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
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
 * XED DDDC (Double Device Data Correctable)
 * @brief XED configuration that DDDC is doable
 */
class XED_DDDC : public XED {
 public:
  XED_DDDC(bool _doFaultDiagnosis = true) : XED(_doFaultDiagnosis) {
    setInDRAM(DoubleSingleSingle18);
    setInDRAMDown(DoubleDouble18);
  }
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                               double rate);  // Fault *fault);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    if (!fault->getIsTransient()) {
      int chipCnt = 0;
      for (auto it = fd->operationalFaultList.cbegin();
           it != fd->operationalFaultList.cend(); it++) {
        if ((*it)->overlap(fault)) {
          if (!fault->getIsSingleDQ() || !fault->getIsSingleBeat()) {
            chipCnt++;
          }
        }
      }
      return (chipCnt > 1);
    }
    return false;
  }
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  void detectInDRAM(CacheLine &errorBlk, std::list<int> &chipLocations);
  void correctInDRAM(CacheLine &errorBlk);
  bool checkParity(CacheLine &errorBlk);
};

/**
 * XED SDDC Narrow channel
 * @brief XED configuration that SDDC is doable, with narrow channel
 */
class XED_SDDC_NC : public XED {
 public:
  XED_SDDC_NC(bool _doFaultDiagnosis = true) : XED(_doFaultDiagnosis) {
    setInDRAM(DoubleSingle9);
    setInDRAMDown(Double);
  }
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
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
//------------------------------------------------------------------------------
#endif /* __XED_HH__ */
