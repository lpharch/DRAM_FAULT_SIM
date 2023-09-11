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
 * @file: DUO.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * DUO declaration. Each class represent one configuration to be tested.
 */

#ifndef __DUO_HH__
#define __DUO_HH__

#include "Bamboo.hh"
#include "ECC.hh"
#include "prior.hh"
#include "rs.hh"

class DUO :public ECC{
 public:
  DUO(int _maxPin, bool _doPostprocess, bool _doRetire,
           int _maxRetiredBlkCount,ECCLayout layout);
  DUO(int _maxPin);
  int bruteForceSearch(FaultDomain *fd, list<int> *chip_list,
                CacheLine* blk,int num_chips);
  void getErasureLocation(std::list<int>* ErasureLocation,int faultychip);
  virtual void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip)=0;
  
  void CorrectByParity(ECCWord *msg, Block *errorBlk, int chipID);
  virtual void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID)=0;
 protected:
  RS<2, 8> *codec;
  RS<2, 8> *decoder;
  RS_DUAL<2, 8> *rs_dual_first;
  RS_DUAL<2, 8> *rs_dual_second;
  std::list<int> *ErasureLocation;
  int maxPin;
};

/**
 *  @brief old version codes for DUO for non-ECC DIMMs.
 */
class DUO64bx4 : public DUO {
 public:
  DUO64bx4(int _maxPin);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID){};

};

/**
 * @brief DUO for 72bit-width channel X4 ECC DIMMs.
 */
class DUO72bx4 : public DUO {
 public:
  // DUO72bx4(int _maxPin);
  DUO72bx4(int _maxPin, bool _doPostprocess, bool _doRetire,
           int _maxRetiredBlkCount);
  //! decodeInternal of DUO
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  bool ParityCheck(ECCWord *decoded, Block *errorBlk);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, std::list<int>* chip_list);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID){};
  void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);

};

/**
 * @brief DUO for 72bit-width channel X8 ECC DIMMs.
 */
class DUO72bx8 : public DUO {
 public:
  // DUO72bx8(int _maxPin);
  DUO72bx8(int _maxPin, bool _doPostprocess, bool _doRetire,
           int _maxRetiredBlkCount);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  bool ParityCheck(ECCWord *decoded, Block *errorBlk);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID);
  void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);

};

class DUO20bx4_34 : public DUO {
  public:
    //    DUO36bx4(int _maxPin);
    DUO20bx4_34(int _maxPin, bool _doPostprocess, bool _doRetire, int _maxRetiredBlkCount);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
    bool needRetire(FaultDomain *fd, Fault *fault) { 
      return !fault->getIsTransient() && (correctedPosSet.size()>1)||
      (fault->getNumDQ()>1);
    }
    void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);
    void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID){};
};

class DUO20bx4_33 : public DUO {
  public:
    //    DUO36bx4(int _maxPin);
    DUO20bx4_33(int _maxPin, bool _doPostprocess, bool _doRetire, int _maxRetiredBlkCount);
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
    unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
    bool needRetire(FaultDomain *fd, Fault *fault) { 
      return !fault->getIsTransient() && (correctedPosSet.size()>1)||
      (fault->getNumDQ()>1);
    }
    void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);
    void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID);
};

//CHANGE 33<->34
class DUO20bx4: public DUO20bx4_33 {
  public:
    DUO20bx4(int _maxPin, bool _doPostprocess, bool _doRetire, int _maxRetiredBlkCount);
};

/**
 * @brief DUO for 36bit-width channel X4 ECC DIMMs.
 */
class DUO36bx4 : public DUO {
 public:
  //    DUO36bx4(int _maxPin);
  DUO36bx4(int _maxPin, bool _doPostprocess, bool _doRetire,
           int _maxRetiredBlkCount);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  bool ParityCheck(ECCWord *decoded, Block *errorBlk);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID);
  void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);

};

/**
 * @brief DUO for 64bit-width channel X4 non-ECC DIMMs.
 */
class DUO64bx4_ : public DUO {
 public:
  DUO64bx4_(int _maxPin, bool _doPostprocess, bool _doRetire,
            int _maxRetiredBlkCount);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  // unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, Fault
  // *fault);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double date);
  bool needRetire(FaultDomain *fd, Fault *fault) {
    return !fault->getIsTransient() && (correctedPosSet.size() > 1) ||
           (fault->getNumDQ() > 1);
  }
  void getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip);
  void CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID){};

};

class DUO20bx4_34_meta : public DUO20bx4_34 {
 public :
  DUO20bx4_34_meta(int _maxPin, bool _doPostprocess, bool _doRetire,
    int _maxRetiredBlkCount, int meta_bytes);
};

class DUO36bx4_meta : public DUO36bx4 {
 public :
  DUO36bx4_meta(int _maxPin, bool _doPostprocess, bool _doRetire,
    int _maxRetiredBlkCount, int meta_bytes);
};



#endif /* __DUO_HH__ */
