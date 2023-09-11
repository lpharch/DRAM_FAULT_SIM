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
 * @file: ECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * @brief General ECC class
 */

#ifndef __ECC_HH__
#define __ECC_HH__

#include <list>
#include <set>
#include "Fault.hh"
#include "message.hh"
#include "util.hh"

class FaultDomain;
class DomainGroup;
class Codec;

extern ErrorType worse2ErrorType(ErrorType a, ErrorType b);

/*! \addtogroup Error_Correction
 * @brief Classes related to error correction
 * @details Codec defines specific error correcting codes (e.g., RS, BCH, etc.)
 * and their behaviors such as syndrome generation and decode.
 * @details ECC defines how to internally utilize Codec to correct errors for
 * each memory transfer block (cacheline) and specific behaviors such as
 * post-processing defined as an ECC module.
 *
 * @{
 *
 * @class ECC
 * @brief General ECC class
 * @details This class defines a general use of error correcting code at memory
 * controller.
 * Specific ECC schemes at different environment (e.g., access granularity) can
 * be defined by inheriting this class.
 * BambooECC (QPC72b) and AMDChipkill72b are the examples of specific ECC
 * classes.
 * Each ECC class can define their own error correcting scheme using different
 * codec and layout, etc.
 * Different codecs requires different ECCWord (codeword) size, e.g., 72B for
 * RS(72,64) of QPC72b and 18B for RS(18,16) of AMDChipkill.
 * Different layout defines how data bits from CacheLine can be rearranged into
 * each ECCWord.
 *
 */
class ECC {
  /*! @} */
 public:
  ECC() : ECC(LINEAR, false, false, 0) {
    // GONG: just for running with almost zero impact
    this->setInDRAM(Septa);
    this->setInDRAMDown(Septa);
    setBitN(512);
  }
  ECC(ECCLayout _layout) : ECC(_layout, false, false, 0) {}
  ECC(ECCLayout _layout, bool _doPostprocess)
      : ECC(_layout, _doPostprocess, false, 0) {}
  ECC(ECCLayout _layout, bool _doPostprocess, bool _doRetire,
      int _maxRetiredBlkCount)
      : layout(_layout),
        doPostprocess(_doPostprocess),
        doRetire(_doRetire),
        maxRetiredBlkCount(_maxRetiredBlkCount) {}
  virtual ~ECC() {}

  //! The decoding function defined in a general way.
  /*!
          \param fd fault domain pointer
          \param blk cacheline block
  */
  ErrorType decode(FaultDomain *fd, CacheLine &blk);
  //! How to decode internally can be defined separately in each ECC scheme.
  /*!
          \param fd fault domain pointer
          \param blk cacheline block
  */
  virtual ErrorType decodeInternal(FaultDomain *fd, CacheLine &blk);
  //! How to postprocess internally for better reliability can be defined
  //! separately in each ECC scheme.
  /*!
          \param fd fault domain pointer
          \param preResult previous decoding result
  */
  virtual ErrorType postprocess(FaultDomain *fd, ErrorType preResult) {
    // no post-processing
    return preResult;
  }

  //! Check whether retirement is set
  bool getDoRetire() { return doRetire; }
  //! Set the retirement feature
  void setDoRetire(bool b) { doRetire = b; }
  //! Get the maximum number of blocks that can be retired
  unsigned long long getMaxRetiredBlkCount() { return maxRetiredBlkCount; }
  //! Set the maximum number of blocks that can be retired
  void setMaxRetiredBlkCount(unsigned long long size) {
    doRetire = true;
    maxRetiredBlkCount = size;
  }
  //! Check a fault needs retirement (i.e., whether it is permanent)
  virtual bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient();
  }
  //! Get the number of blocks that should be retired at the beginning
  virtual unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                                       Fault *fault);
  //! Get the number of blocks that should be retired at the beginning
  virtual unsigned long long getInitialRetiredBlkCount(FaultDomain *fd,
                                                       double rate);

  //! Reset (remove all elements of) the corrected position set
  void clear() { correctedPosSet.clear(); }

  //! ECC configuration
  /*! \param maxDeviceRetirement Retirement capacity in terms of number of
     devices
          \param maxPinRetirement Retirement capacity in terms of number of pins
          \param codec
  */
  struct config {
    int maxDeviceRetirement;
    int maxPinRetirement;
    Codec *codec;
  };
  // GONG
  bool chipRand = true;
  //! Error pattern that indram ECC cannot tolerate
  int indram;
  int getInDRAM() { return indram; }

  // setInDRAM : To compare SDC rate, we could use the number 
  // that it always cause the SDC.
  
  void setInDRAM(int _indram) { indram = _indram; };
  //! Error pattern that indram ECC cannot tolerate, after ECC is downgraded due
  //! to existence of permanent operational faults.
  int indram_down;
  int getInDRAMDown() { return indram_down; }
  void setInDRAMDown(int _indram) { indram_down = _indram; };
  //! Codeword size of indram ECC
  int bitN;
  int getBitN() { return bitN; };
  void setBitN(int _bitN) { bitN = _bitN; };
  //! Correction mode that can be used in multi-tiered ECC (e.g., XED)
  int correct_mode;
  void printHistogram(){
    printf("BruteForce count\n");
    for(int i=0;i<sizeof(BF_Stat)/sizeof(int);i++){
      printf("%d ",BF_Stat[i]);
    }
    printf("\n");
  };

 protected:
  //! List of configuration that will be required for graceful downgrade
  std::list<struct config> configList;  // for graceful downgrade
                                        //! ECC codeword layout
  ECCLayout layout;

  bool doPostprocess;
  bool doRetire;
  unsigned long long maxRetiredBlkCount;
  int BF_Stat[300]={0,};
  //! Corrected (pin or symbol) position set
  std::set<int> correctedPosSet;
};

#endif /* __ECC_HH__ */
