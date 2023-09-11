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
 * @file: FaultDomain.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * @brief FaultDomain is a domain that share the same fault history
 */

#ifndef __FAULT_DOMAIN_HH__
#define __FAULT_DOMAIN_HH__

#include <list>
#include <vector>
#include "FaultRateInfo.hh"
#include "util.hh"
#include "message.hh"

class ECC;
class Fault;


/**@addtogroup Fault_Management
 * \brief Classes related to faults
 * \details Class Fault defines different fault types. Class FaultDomain defines
 * memory domain where faults are injected. Class FaultRateInfo defines the
 * fault injection rates (rate information is separately managed).
 * \details
 * @{
 *
 * @class FaultDomain
 * \brief FaultDomain is a memory domain consisting of multiple units (e.g.,
 * physical ranks).
 * @details This class defines how to invoke fault injection, random error
 * generation, and ECC decode within memory domains.
 * FaultDomain size (e.g., how many ranks and chips) can be chosen by users and
 * DomainGroup is comprised of multiple fault domains (please see main.cc and
 * DomainGroup.cc).
 *
 */
class FaultDomain {
 public:
  //! Constructor
  /*\param _rankPerDomain Number of ranks in each fault domain
          \param _devicesPerRank Number of chips in a rank
          \param _pinsPerDevices Number of data I/O pins (channel width)
          \param _blkHeight Burst length
          \param _faultRateInfo FaultRateInfo pointer
           */
  FaultDomain(int _ranksPerDomain, int _devicesPerRank, int _pinsPerDevice,
              int _blkHeight, FaultRateInfo *_faultRateInfo)
      : ranksPerDomain(_ranksPerDomain),
        devicesPerRank(_devicesPerRank),
        pinsPerDevice(_pinsPerDevice),
        blkHeight(_blkHeight),
        faultRateInfo(_faultRateInfo),
        message_config(0,0,0,0,0,pinsPerDevice,devicesPerRank),
        inherentFault(NULL) {
    clear();
    HBM_setup=false;
    
  }

  FaultDomain(int _ranksPerDomain, int _devicesPerRank, int _pinsPerDevice,
              int _blkHeight, FaultRateInfo *_faultRateInfo, int _msg_height_base,
              int _DRAM_BaseBL, int _DRAM_extraBeat, int _msg_extraheight, int _DRAM_overfetch_mult)
      : ranksPerDomain(_ranksPerDomain),
        devicesPerRank(_devicesPerRank),
        pinsPerDevice(_pinsPerDevice),
        blkHeight(_blkHeight),
        faultRateInfo(_faultRateInfo),
        message_config(_msg_height_base,_DRAM_BaseBL,_DRAM_extraBeat,_msg_extraheight,_DRAM_overfetch_mult,pinsPerDevice,devicesPerRank),
        inherentFault(NULL) {
    clear();
    HBM_setup=false;
    assert(blkHeight != _DRAM_BaseBL * _DRAM_overfetch_mult + _DRAM_extraBeat);
  }


  FaultDomain(int _ranksPerDomain, int _devicesPerRank, int _pinsPerDevice,
              int _blkHeight, FaultRateInfo *_faultRateInfo, MSGConfig _message_config)
      : ranksPerDomain(_ranksPerDomain),
        devicesPerRank(_devicesPerRank),
        pinsPerDevice(_pinsPerDevice),
        blkHeight(_blkHeight),
        faultRateInfo(_faultRateInfo),
        message_config(_message_config),
        inherentFault(NULL) {
    clear();
    HBM_setup=false;
    
  }

  //! Destructor
  ~FaultDomain() {
    if (inherentFault != NULL) {
      delete inherentFault;
    }
    delete faultRateInfo;
  }
  
  void setHBM(bool _HBM_setup){
    HBM_setup=_HBM_setup;
  }

 public:
  int getChannelWidth() { return devicesPerRank * pinsPerDevice; }
  int getChipWidth() { return pinsPerDevice; }
  int getChipCount() { return devicesPerRank; }
  int getBeatHeight() { return blkHeight; }
  double getFaultRate() {
    return ranksPerDomain * devicesPerRank * faultRateInfo->getTotalRate();
  }

  void resetInherentFault(Fault *fault, ECC *ecc);  //!< reset inherent fault
  //! rate (at the beginning of
  //! each iteration)
  void setInherentFault(Fault *fault, ECC *ecc,
                        bool first);  //!< set inherent fault rate
  void setInitialRetiredBlkCount(ECC *ecc, double rate);

  unsigned long long getRetiredBlkCount() { return retiredBlkCount; }
  size_t getRetiredChipCount() { return retiredChipIDList.size(); }
  size_t getRetiredPinCount() { return retiredPinIDList.size(); }
  //! Fault generation and Test based on scenario
  /*! \param ecc ECC pointer
           \param faultCount Number of faults
           \param faults Pointer to string array where fault types are stored
           \param chipOverlapCheck Overlap check parameter
   */
  ErrorType genScenarioRandomFaultAndTest(
      ECC *ecc, int faultCount, std::string *faults,
      bool chipOverlapCheck = true);  // For fault generation based on scenario
  //! Fault generation and Test for a system
  /*! \param ecc ECC pointer */
  ErrorType genSystemRandomFaultAndTest(
      ECC *ecc);  // For fault generation based on fault rates

  void retirePin(int pinID);
  void retireChip(int chipID);

  void scrub();

  void clear();
  void print(FILE *fd = stdout) const;

  // GONG
  bool permFaults();          //!< a function checking if permanent faults exist
  void setSingleChipFault();  //!< set single chip fault (in advance)
  void InitInherentFaultRate() { faultRateInfo->iRate->setIFRate(); };
  bool overlapTest();  //!< Checking if overlap exists
  bool getBadCount(ECC *ecc);

 public:
  int ranksPerDomain;  //!< number of ranks involved in this fault domain
  int devicesPerRank;  //!< number of chips in each rank
  int pinsPerDevice;   //!< number of pins (bit-width of each chip
  int blkHeight;       //!< Entire blk height 
  
  MSGConfig message_config;

  FaultRateInfo *faultRateInfo;  //!< fault rate informaation

  Fault *inherentFault;
  std::list<Fault *>
      operationalFaultList;  //!< List of (injected) operational faults
  
  std::vector<Fault *>
      currentPossibleFaultList;  //!< List of (injected) operational faults
  std::list<Fault *>    
      activeFaultList;
  unsigned long long retiredBlkCount;
  std::list<int> retiredPinIDList;   //!< List of retired pins
  std::list<int> retiredChipIDList;  //!< List of retired chips

  // GONG
  void FaultyChipDetect(std::list<int> *chip_list);  //!< find faulty chip (for
                                                     //! a faster simulation)
  void updateInherentFault(ECC *ecc);
  void printFaultRate() { printf("getFaultRate(): %.10e\n", getFaultRate()); };
  void setTester(class TesterSystem *tester, class ECC *ecc);
  void printOperationalFaults();
  void printTransientFaults();
  void printVisualFaults();
  void printActiveFaults();
  void setFaultStats(ErrorType type,int year);
  ADDR OverlappedAddr(void);
  float* getFaultStats(ErrorType type,int year);
  float** getFaultStatsALL(ErrorType type);
  bool HBM_setup;

 protected:
  class TesterSystem *_tester;
  float **SDCstats;
  float **DUEstats;
};

/* @} */

/**
 * \brief DDR FaultDomain class
 */
class FaultDomainDDR : public FaultDomain {
 public:
  FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight);
  FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config);
  FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config,bool HBM);
  FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config,bool HBM,bool detailed);
  ~FaultDomainDDR();
};

unsigned long long within_row_addr();
unsigned long long within_bank_addr();

//------------------------------------------------------------------------------
#endif /* __FAULT_DOMAIN_HH__ */
