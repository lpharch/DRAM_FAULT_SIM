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
 * @file: DomainGroup.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * @brief DomainGroup is a group of fault domains.
 */

#ifndef __DOMAIN_GROUP_HH__
#define __DOMAIN_GROUP_HH__

#include <list>

#include "FaultDomain.hh"
#include "common.hh"


/**
 * @brief DomainGroup class
 * @details This class defines the target DRAM configuration using parameters
 * such as number of pins (chip-width), number of chips per rank, number of
 * ranks per FaultDomain, number of Faultdomains.
 * Detailed behaviors such as fault injection are performed by FaultDomain
 * class.
 */
class DomainGroup {  // Corresponds to a cluster
 public:
  DomainGroup() {}
  ~DomainGroup() {
    for (auto& domain : FDList) {
      delete domain;
    }
  }

  virtual void setHBM(bool isHBM) {
    for (auto& domain : FDList) {
      domain->setHBM(isHBM);
    }
  }

  double getFaultRate();  //!< overall fault rate of domain group
  void setSingleChipFault() {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->setSingleChipFault();
    }
  }  //!< set single chip fault in advance
  void resetInherentFault(Fault *fault, ECC *ecc) {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->resetInherentFault(fault, ecc);
    }
  }  //!< reset of inherent fault rate (at the beginning of iterations)
  void setInherentFault(Fault *fault, ECC *ecc, bool first) {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->setInherentFault(fault, ecc, first);
    }
  }  //!< set inherent fault rate
  void updateInherentFault(ECC *ecc) {
    for (auto it = FDList.begin(); it != FDList.end(); it++) {
      (*it)->updateInherentFault(ecc);
    }
  }  //!< update inherent fault rate due to operational fault overlap
  bool getBadCount(ECC *ecc) {
    for (auto it = FDList.begin(); it != FDList.end(); it++) {
      if ((*it)->getBadCount(ecc)) return true;
    }
    return false;
  }  //!< old functionality

  void setInitialRetiredBlkCount(ECC *ecc, double rate) {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->setInitialRetiredBlkCount(ecc, rate);
    }
  }  //!< count number of blocks that needs to be retired even at the initial
     //! state

  FaultDomain *pickRandomFD();  //!< pick randomly one of FaultDomains

  void scrub() {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->scrub();
    }
  }  //!< scrubbing wrapper
  void clear() {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->clear();
    }
  }  //!< clearing wrapper
  FaultDomain *getFD() { return FDList.front(); }

  void printFaultRate() {
    for (auto it = FDList.begin(); it != FDList.end(); it++) {
      (*it)->printFaultRate();
    }
  }

  void setTester(class TesterSystem *tester, ECC *ecc) {
    _tester = tester;
    for (auto it = FDList.begin(); it != FDList.end(); it++) {
      (*it)->setTester(tester, ecc);
    }
  }

  void printFaultStats(FILE *fd, long DUECntYear, long SDCCntYear, int year);
  void printFaultStatsAll(FILE *fd, long* DUECntYears, long* SDCCntYears, int MAXYEAR);

 protected:
  class TesterSystem *_tester;  //<! tester system class

 protected:
  std::list<FaultDomain *>
      FDList;  //<! list of fault domains included in this domain group
};

/**
 * @brief DDR Domain Group class
 */
class DomainGroupDDR : public DomainGroup {
 public:
  DomainGroupDDR(int domainsPerGroup, int ranksPerDomain, int devicesPerRank,
                 int pinsPerDevice, int blkHeight) {
    for (int i = 0; i < domainsPerGroup; i++) {
      FDList.push_back(new FaultDomainDDR(ranksPerDomain, devicesPerRank,
                                          pinsPerDevice, blkHeight));
    }
  }

  
  DomainGroupDDR(int domainsPerGroup, int ranksPerDomain, int devicesPerRank,
                 int pinsPerDevice, int blkHeight, MSGConfig message_config) {
    for (int i = 0; i < domainsPerGroup; i++) {
      FDList.push_back(new FaultDomainDDR(ranksPerDomain, devicesPerRank,
                                          pinsPerDevice, blkHeight, message_config));
    }
  }

  DomainGroupDDR(int domainsPerGroup, int ranksPerDomain, int devicesPerRank,
                 int pinsPerDevice, int blkHeight, MSGConfig message_config, bool HBM) {
    for (int i = 0; i < domainsPerGroup; i++) {
      FDList.push_back(new FaultDomainDDR(ranksPerDomain, devicesPerRank,
                                          pinsPerDevice, blkHeight, message_config, HBM));
    }
  }

  DomainGroupDDR(int domainsPerGroup, int ranksPerDomain, int devicesPerRank,
                 int pinsPerDevice, int blkHeight, MSGConfig message_config, bool HBM, bool detailed) {
    for (int i = 0; i < domainsPerGroup; i++) {
      FDList.push_back(new FaultDomainDDR(ranksPerDomain, devicesPerRank,
                                          pinsPerDevice, blkHeight, message_config, HBM, detailed));
    }
  }

  void setHBM(bool HBM) {
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      (*it)->setHBM(HBM);
    }
  }
};

#endif /* __DOMAIN_GROUP_HH__ */
