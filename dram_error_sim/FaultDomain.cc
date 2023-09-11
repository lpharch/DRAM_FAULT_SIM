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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "Config.hh"
#include "ECC.hh"
#include "FaultDomain.hh"
#include "Tester.hh"

//------------------------------------------------------------------------------
ErrorType worseErrorType(ErrorType a, ErrorType b) { return (a > b) ? a : b; }

//------------------------------------------------------------------------------
ErrorType FaultDomain::genScenarioRandomFaultAndTest(ECC *ecc, int faultCount,
                                                     std::string *faults,
                                                     bool chipOverlapCheck) {
  CacheLine blk = {
      pinsPerDevice,
      (devicesPerRank - (int)retiredChipIDList.size()) * pinsPerDevice -
          (int)retiredPinIDList.size(),
      blkHeight, message_config};

  clear();

  

  int fault1ChipID = -1;
  int fault2ChipID = -1;
  int fault3ChipID = -1;

  // generate fault1
  if (faultCount > 0) {
    Fault *fault1 = Fault::genRandomFault(faults[0], this);
    // GONG
    operationalFaultList.push_back(fault1);
    currentPossibleFaultList.push_back(fault1);
    activeFaultList.push_back(fault1);

    fault1->genRandomError(&blk);
    fault1ChipID = fault1->getChipID();
    //        delete fault1;
  }
  // generate fault2
  if (faultCount > 1) {
    Fault *fault2 = NULL;
    do {
      if (fault2 != NULL) delete fault2;  // delete previous one
      fault2 = Fault::genRandomFault(faults[1], this);
    } while (fault2->getChipID() == fault1ChipID && chipOverlapCheck);
    // GONG
    operationalFaultList.push_back(fault2);
    currentPossibleFaultList.push_back(fault2);
    activeFaultList.push_back(fault2);

    fault2->genRandomError(&blk);
    fault2ChipID = fault2->getChipID();
    //        delete fault2;
  }

  // generate fault3
  if (faultCount > 2) {
    Fault *fault3 = NULL;
    do {
      if (fault3 != NULL) delete fault3;  // delete previous one
      fault3 = Fault::genRandomFault(faults[2], this);
    } while (((fault3->getChipID() == fault1ChipID) ||
              (fault3->getChipID() == fault2ChipID)) &&
             chipOverlapCheck);
    // GONG
    operationalFaultList.push_back(fault3);
    currentPossibleFaultList.push_back(fault3);
    activeFaultList.push_back(fault3);

    fault3->genRandomError(&blk);
    fault3ChipID = fault3->getChipID();
    //        delete fault3;
  }

  // generate fault4
  if (faultCount > 3) {
    Fault *fault4 = NULL;
    do {
      if (fault4 != NULL) delete fault4;  // delete previous one
      fault4 = Fault::genRandomFault(faults[3], this);
    } while (((fault4->getChipID() == fault1ChipID) ||
              (fault4->getChipID() == fault2ChipID) ||
              (fault4->getChipID() == fault3ChipID)) &&
             chipOverlapCheck);
    // GONG
    operationalFaultList.push_back(fault4);
    currentPossibleFaultList.push_back(fault4);
    activeFaultList.push_back(fault4);

    fault4->genRandomError(&blk);
    //        delete fault4;
  }

  // generate fault5
  if (faultCount > 4) {
    Fault *fault5 = NULL;
    do {
      if (fault5 != NULL) delete fault5;  // delete previous one
      fault5 = Fault::genRandomFault(faults[4], this);
    } while (((fault5->getChipID() == fault1ChipID) ||
              (fault5->getChipID() == fault2ChipID) ||
              (fault5->getChipID() == fault3ChipID)) &&
             chipOverlapCheck);
    // GONG
    operationalFaultList.push_back(fault5);
    currentPossibleFaultList.push_back(fault5);
    activeFaultList.push_back(fault5);

    fault5->genRandomError(&blk);
    //        delete fault5;
  }

  // generate fault6
  if (faultCount > 5) {
    Fault *fault6 = NULL;
    do {
      if (fault6 != NULL) delete fault6;  // delete previous one
      fault6 = Fault::genRandomFault(faults[5], this);
    } while (((fault6->getChipID() == fault1ChipID) ||
              (fault6->getChipID() == fault2ChipID) ||
              (fault6->getChipID() == fault3ChipID)) &&
             chipOverlapCheck);
    // GONG
    operationalFaultList.push_back(fault6);
    currentPossibleFaultList.push_back(fault6);
    activeFaultList.push_back(fault6);

    fault6->genRandomError(&blk);
    //        delete fault6;
  }
  assert(faultCount <= 6);

  // decode and report the result
  ErrorType result = ecc->decode(this, blk);
  //clear();
  return result;
}

void generateCombinations(const std::vector<Fault*>& faults,
                          std::vector<std::vector<Fault*>>& combinations,
                          std::vector<Fault*> current = {}, size_t start = 0) {
  if (current.size() > 1) {
    combinations.push_back(current);
  }
  for (size_t i = start; i < faults.size(); ++i) {
    current.push_back(faults[i]);
    generateCombinations(faults, combinations, current, i + 1);
    current.pop_back();
  }
}

ErrorType FaultDomain::genSystemRandomFaultAndTest(ECC *ecc) {
  // CacheLine blkOrg = {pinsPerDevice, (devicesPerRank -(int)
  // retiredChipIDList.size()) * pinsPerDevice - (int) retiredPinIDList.size(),
  // blkHeight};
  CacheLine blk = {
      pinsPerDevice,
      (devicesPerRank - (int)retiredChipIDList.size()) * pinsPerDevice -
          (int)retiredPinIDList.size(),
      blkHeight, message_config};
  Fault *newFault;
  ErrorType result = NE;
  // whether we try decode errors here by inherent fault or the other error
  // sources

  //----------------------------------------------------------
  // 1. generate a new fault
  //----------------------------------------------------------
  // std::string newFaultType = faultRateInfo->pickRandomType();
  const std::pair<std::string, double> *newFaultType =
      faultRateInfo->pickRandomType();

  // whether this test caused by (intermittent) inherent faults
  bool ByInherentFault = (newFaultType->first == "inherent") ? true : false;

  if (!ByInherentFault) {
    newFault = Fault::genRandomFault(newFaultType->first, this);

    /*	//GONG: retirement is currently not considered with inherent faults
        //----------------------------------------------------------
        // check whether the fault is on a retired chip or pin
        //----------------------------------------------------------
        // a new fault on already retired chip -> skip
        for (auto it = retiredChipIDList.cbegin(); it !=
       retiredChipIDList.cend(); ++it) {
            if (newFault->getChipID()==(*it)) {
                delete newFault;
                return NE;
            }
        }
        // a new pin fault on already retired pin -> skip
        if (newFault->getIsSingleDQ()) {
            // retired pin
            for (auto it = retiredPinIDList.cbegin(); it !=
       retiredPinIDList.cend(); ++it) {
                if (newFault->getPinID()==(*it)) {
                    delete newFault;
                    return NE;
                }
            }
        }
    */
    // overlap_test
    int indram = ecc->getInDRAM();
    if ((double)rand() / RAND_MAX <=
        faultRateInfo->overlap_prob(newFault->getName())) {
      newFault->overlapped = true;
    } else {
      newFault->overlapped = false;
    }

#if 1
    operationalFaultList.push_back(newFault);

  } else {
    // GONG: we do not generate a new fault.
    if (operationalFaultList.size() > 0) {
      auto it = operationalFaultList.crbegin();
      newFault = (*it);  // operationalFaultList.begin();
    } else {
      // no operational fault yet.
      blk.reset();
      if (inherentFault != NULL)
        inherentFault->genRandomErrors(&blk, faultRateInfo->iRate->getEP(),
                                       ecc->chipRand);
      return worseErrorType(result, ecc->decode(this, blk));
    }
  }
  int overlap_level = 0;

  //----------------------------------------------------------
  // 3. check overlapping previous fault
  // - check the most severe one
  //----------------------------------------------------------
  currentPossibleFaultList.clear();
  activeFaultList.clear();
  activeFaultList.push_back(newFault);
  //Simplified version for multiple overlaps
  auto it1 = operationalFaultList.crbegin();
  bool overlap = false;
  for (++it1; it1 != operationalFaultList.crend(); ++it1) {
    if ((*it1)->overlap(newFault)) {
      overlap = true;
      currentPossibleFaultList.push_back(*it1);
      activeFaultList.push_back(*it1);
    }
  }
  if (!overlap) {
    blk.reset();
    if (ByInherentFault) {
      inherentFault->genRandomErrors(&blk, faultRateInfo->iRate->getEP(),
                                    ecc->chipRand);
    } else if (inherentFault != NULL) {
      inherentFault->genRandomError(&blk);
    }

    newFault->genRandomError(&blk);
    result = worseErrorType(result, ecc->decode(this, blk));
  } else {
    std::vector<Fault*> overlappedFaults = currentPossibleFaultList;
    overlappedFaults.push_back(newFault);
    std::vector<std::vector<Fault*>> combinations;
    generateCombinations(overlappedFaults, combinations);

    for (const auto& combo : combinations) {
      bool overlap = true;
      for (const auto& fault : combo) {
        if (!fault->overlap(combo.front())) {
          overlap = false;
          break;
        }
      }
      if (overlap) {
        blk.reset();
        if (ByInherentFault) {
          inherentFault->genRandomErrors(&blk, faultRateInfo->iRate->getEP(),
                                        ecc->chipRand);
        } else if (inherentFault != NULL) {
          inherentFault->genRandomError(&blk);
        }
        // get length of combo
        
        for (const auto& fault : combo) {
          fault->genRandomError(&blk);
        }
        result = worseErrorType(result, ecc->decode(this, blk));
      }
    }
  }


  //GONG: fine-grained retirement is currently inactive/ignored.
  if ((result==CE)&&ecc->getDoRetire()&&ecc->needRetire(this, newFault))
  {
    if (ecc->getMaxRetiredBlkCount() > 
                      retiredBlkCount+newFault->getAffectedBlkCount()){
      if (newFault->getAffectedBlkCount() >0 ){
        retiredBlkCount += newFault->getAffectedBlkCount();
        operationalFaultList.remove(newFault);
        activeFaultList.remove(newFault);
        delete newFault;
      }
    }else{
      retiredBlkCount = ecc->getMaxRetiredBlkCount();
      // printf("%s need %llu / %llu\n",
      //  newFaultType->first.c_str(),newFault->getAffectedBlkCount(),
      //  ecc->getMaxRetiredBlkCount()-retiredBlkCount);
    }
  }

  return result;
#else
    auto faultEnd = operationalFaultList.rend();
    for (auto it1 = operationalFaultList.rbegin(); it1 != faultEnd; ++it1) {
      Fault *fault1 = *it1;

      fault1->genRandomError(&blk);
      result = ecc->decode(this, blk);
      if (result == SDC) return SDC;

      auto it2 = it1;
      for (++it2; it2 != faultEnd; ++it2) {
        Fault *fault2 = (*it2);

        if (fault2->overlap(fault1)) {
          fault2->genRandomError(&blk);
          result = worseErrorType(result, ecc->decode(this, blk));
          if (result == SDC) return SDC;

          auto it3 = it2;
          for (++it3; it3 != faultEnd; ++it3) {
            Fault *fault3 = (*it3);
            if (fault3->overlap(fault1) && fault3->overlap(fault2)) {
              fault3->genRandomError(&blk);
              result = worseErrorType(result, ecc->decode(this, blk));
              if (result == SDC) return SDC;

              auto it4 = it3;
              for (++it4; it4 != faultEnd; ++it4) {
                Fault *fault4 = (*it4);
                if (fault4->overlap(fault1) && fault4->overlap(fault2) &&
                    fault4->overlap(fault3)) {
                  fault4->genRandomError(&blk);
                  result = worseErrorType(result, ecc->decode(this, blk));
                  if (result == SDC) return SDC;
                }
              }
            }
          }
        }
      }
    }

    return result;
#endif
}

void FaultDomain::resetInherentFault(Fault *fault, ECC *ecc) {
  faultRateInfo->removeLastRate();
  setInherentFault(fault, ecc, false);
}
void FaultDomain::setInherentFault(Fault *fault, ECC *ecc, bool first) {
  inherentFault = fault;
  if (first) InitInherentFaultRate();
  // set inherent Fault Rate
  InherentErrorPattern indram = (InherentErrorPattern)ecc->getInDRAM();
  faultRateInfo->iRate->setEP(indram);
  faultRateInfo->addFaultRate("inherent",
                              faultRateInfo->iRate->getRate(indram));
}

bool FaultDomain::overlapTest() {
  for (std::list<Fault *>::iterator it = operationalFaultList.begin();
       it != operationalFaultList.end(); it++) {
    if ((*it)->overlapped) return true;
  }
  return false;
}

void FaultDomain::updateInherentFault(ECC *ecc) {
  bool need_update = false;
  int indram_down = ecc->getInDRAMDown();
  double rate = 0.0;
  // check there are operational faults injected
  for (std::list<Fault *>::iterator it = operationalFaultList.begin();
       it != operationalFaultList.end(); it++) {
    if (!(*it)->getIsTransient()) {
      if ((*it)->overlapped) {
        need_update = true;
        faultRateInfo->iRate->setEP((InherentErrorPattern)indram_down);
        rate +=
            faultRateInfo->iRate->getRate((InherentErrorPattern)indram_down) *
            faultRateInfo->overlap_prob((*it)->getName());
      }
    }
  }

  if (need_update) {
    faultRateInfo->removeLastRate();
    faultRateInfo->addFaultRate("inherent", rate);
  }
}

//------------------------------------------------------------------------------
void FaultDomain::setInitialRetiredBlkCount(ECC *ecc, double rate) {
  retiredBlkCount = ecc->getInitialRetiredBlkCount(this, rate);
}

void FaultDomain::scrub() {
  if (operationalFaultList.size() > 0){
    activeFaultList.remove_if([](Fault *f) { return f->getIsTransient(); });
    operationalFaultList.remove_if(
      [](Fault *f) { 
        bool transient = f->getIsTransient();
        if (transient) delete f;
        return transient; 
      }
      );
  }
}

//------------------------------------------------------------------------------
void FaultDomain::retirePin(int pinID) {
  auto it = operationalFaultList.begin();
  while (it != operationalFaultList.end()) {
    if ((*it)->getIsSingleDQ() && ((*it)->getPinID() == pinID)) {
      // pin fault
      delete *it;
      it = operationalFaultList.erase(it);
      // continue without advancing the iterator
    } else {
      ++it;
    }
  }
  retiredPinIDList.push_back(pinID);
}

void FaultDomain::retireChip(int chipID) {
  auto it = operationalFaultList.begin();
  while (it != operationalFaultList.end()) {
    if ((*it)->getChipID() == chipID) {
      delete *it;
      it = operationalFaultList.erase(it);
      // continue without advancing the iterator
    } else {
      ++it;
    }
  }
  retiredChipIDList.push_back(chipID);
}

void FaultDomain::clear() {
  for (auto it = operationalFaultList.begin(); it != operationalFaultList.end();
       ++it) {
    delete *it;
  }
  operationalFaultList.clear();
  retiredChipIDList.clear();
  retiredPinIDList.clear();
  currentPossibleFaultList.clear();
  activeFaultList.clear();
}

void FaultDomain::print(FILE *fd) const {
  for (auto it = operationalFaultList.begin(); it != operationalFaultList.end();
       it++) {
    (*it)->print(fd);
  }
}

//------------------------------------------------------------------------------
#define CORRECT_DETECT 3
// 1 == Correct detection
// 2 == Only one return
// 3 == Original implementation

// GONG: For faster simulation, we assume that faulty chip information can be
// retrieved if it is stored somewhere.
void FaultDomain::FaultyChipDetect(std::list<int> *chip_list) {
  int num_chip_fault = 0;
  Fault *cur_fault;
  Fault *prev_fault = NULL;
  std::set<int> chip_set;  // set of faulty chips
  std::pair<std::set<int>::iterator, bool> ret;
  std::list<Fault *> prev_list;


#if CORRECT_DETECT==1
  if (operationalFaultList.size() == 1) {
    chip_set.insert((*operationalFaultList.begin())->getChipID());
    num_chip_fault = 1;
  } else {
    for (std::list<Fault *>::reverse_iterator it =
             operationalFaultList.rbegin();
         it != operationalFaultList.rend(); it++) {
      // check if it has chip-level fault (bank, rank)
      // ideally we can figure out faulty chip by accessing/correcting adjacent
      // blocks
      if (!(*it)->getIsSingleBeat()) {
        cur_fault = *it;
        if (prev_list.size() > 0)
        {
          for (std::list<Fault *>::iterator jt = prev_list.begin(); 
                                        jt != prev_list.end(); jt++){
            prev_fault = *jt;
            if (cur_fault->overlap(prev_fault)) {
              ret = chip_set.insert(cur_fault->getChipID());
              if (ret.second) num_chip_fault++;
            }
          }
        } 
      }
      prev_list.push_back(*it);
    }
  }
  if (num_chip_fault == 1) {
    // return *chip_set.begin();
    chip_list->push_back(*chip_set.begin());
  } else if (num_chip_fault > 1) {
    for (auto it = chip_set.begin(); it != chip_set.end(); it++) {
      chip_list->push_back((*it));
    }
  }
#elif CORRECT_DETECT==2
  if (operationalFaultList.size() == 1) {
    chip_set.insert((*operationalFaultList.begin())->getChipID());
    num_chip_fault = 1;
  } else {
    for (std::list<Fault *>::reverse_iterator it =
             operationalFaultList.rbegin();
         it != operationalFaultList.rend(); it++) {
      // check if it has chip-level fault (bank, rank)
      // ideally we can figure out faulty chip by accessing/correcting adjacent
      // blocks
      if (!(*it)->getIsSingleBeat()) {
        cur_fault = *it;
        if (prev_list.size() > 0)
        {
          for (std::list<Fault *>::iterator jt = prev_list.begin(); 
                                        jt != prev_list.end(); jt++){
            prev_fault = *jt;
            if (cur_fault->overlap(prev_fault)) {
              ret = chip_set.insert(cur_fault->getChipID());
              if (ret.second) num_chip_fault++;
            }
          }
        } else {
          if (cur_fault->overlap(prev_fault)) {
            ret = chip_set.insert(cur_fault->getChipID());
            if (ret.second) num_chip_fault++;
          }          
        }
      }
      prev_list.push_back(*it);
    }
  }
  if (num_chip_fault >= 1) {
    // return *chip_set.begin();
    chip_list->push_back(*chip_set.begin());
  }
#else
  // if only one fault exists
  if (operationalFaultList.size() == 1) {
    chip_set.insert((*operationalFaultList.begin())->getChipID());
    num_chip_fault = 1;
  } else {
    for (std::list<Fault *>::reverse_iterator it =
             operationalFaultList.rbegin();
         it != operationalFaultList.rend(); it++) {
      // check if it has chip-level fault (bank, rank)
      // ideally we can figure out faulty chip by accessing/correcting adjacent
      // blocks
      if (!(*it)->getIsSingleBeat()) {
        cur_fault = *it;
        if (cur_fault->overlap(prev_fault)) {
          ret = chip_set.insert(cur_fault->getChipID());
          if (ret.second) num_chip_fault++;
        }
        //(*it)->print();
      }
      prev_fault = *operationalFaultList.rbegin();
    }
  }
  if (num_chip_fault == 1) {
    // return *chip_set.begin();
    chip_list->push_back(*chip_set.begin());
  } else if (num_chip_fault == 2) {
    for (auto it = chip_set.begin(); it != chip_set.end(); it++) {
      chip_list->push_back((*it));
    }
  }

#endif //CORRECT_DETECT

}

void FaultDomain::printActiveFaults() {
  if (activeFaultList.size() > 0) {
    for (auto it = activeFaultList.begin();
         it != activeFaultList.end(); it++) {
      auto name = (*it)->getName();
      auto DQs = (*it)->getNumDQ();
      std::cout << DQs;
      std::cout << name << "-";
    }
    std::cout << std::endl;
  } else {
    //		std::cout<<"no operatioal faults"<<std::endsl;
  }
}
void FaultDomain::printOperationalFaults() {
  if (operationalFaultList.size() > 0) {
    for (auto it = operationalFaultList.begin();
         it != operationalFaultList.end(); it++) {
      auto name = (*it)->getName();
      auto DQs = (*it)->getNumDQ();
      std::cout << DQs;
      std::cout << name << "-";
    }
    std::cout << std::endl;
  } else {
    std::cout<<std::endl;
    //		std::cout<<"no operatioal faults"<<std::endsl;
  }
}
void FaultDomain::printTransientFaults() {
  if (operationalFaultList.size() > 0) {
    for (auto it = operationalFaultList.begin();
         it != operationalFaultList.end(); it++) {
      if (!(*it)->getIsTransient()){
        continue;
      }
      auto name = (*it)->getName();
      auto DQs = (*it)->getNumDQ();
      std::cout << DQs;
      std::cout << name << "-";
    }
    std::cout << std::endl;
  } else {
    //		std::cout<<"no operatioal faults"<<std::endsl;
  }
}

void FaultDomain::printVisualFaults(){
  //Visualize faults using a 2D array
  if (operationalFaultList.size() > 0) {
    for (auto it = operationalFaultList.begin();
         it != operationalFaultList.end(); it++) {
      if ((*it)->detailed_faults.size() > 0){
        for (auto jt = (*it)->detailed_faults.begin();
             jt != (*it)->detailed_faults.end(); jt++) {
          ADDR mask = (*jt)->getEffectiveMask();
          ADDR address = (*jt)->getAddr();
          int bankmask = (mask >> (column_address_bits + row_address_bits)) & (numofBanks-1);  
          int rankmask = (mask >> int(column_address_bits + row_address_bits + log2(numofBanks))) & (1); // 1 for 1 rank
          int colmask = (mask & ((1 << column_address_bits)-1));
          int rowmask = (mask >> column_address_bits) & ((1 << (row_address_bits))-1);

          int bankid = (address >> (column_address_bits + row_address_bits)) & (numofBanks-1);
          int rankid = (address >> int(column_address_bits + row_address_bits + log2(numofBanks))) & (1); // 1 for 1 rank
          int colid = (address & ((1 << column_address_bits)-1));
          int rowid = (address >> column_address_bits) & ((1 << (row_address_bits))-1);

          // Print it in one line
          // mask should be hex, address should be decimal
          std::cout << std::hex << bankmask << " " << rankmask << " " << colmask << " " << rowmask << " " << bankid << " " << rankid << " " << colid << " " << rowid << ",";
        }
        std::cout << "//";
      } else{
        ADDR mask = (*it)->getEffectiveMask();
        ADDR address = (*it)->getAddr();
        int bankmask = (mask >> (column_address_bits + row_address_bits)) & (numofBanks-1);  
        int rankmask = (mask >> int(column_address_bits + row_address_bits + log2(numofBanks))) & (1); // 1 for 1 rank
        int colmask = (mask & ((1 << column_address_bits)-1));
        int rowmask = (mask >> column_address_bits) & ((1 << (row_address_bits))-1);

        int bankid = (address >> (column_address_bits + row_address_bits)) & (numofBanks-1);
        int rankid = (address >> int(column_address_bits + row_address_bits + log2(numofBanks))) & (1); // 1 for 1 rank
        int colid = (address & ((1 << column_address_bits)-1));
        int rowid = (address >> column_address_bits) & ((1 << (row_address_bits))-1);

        // Print it in one line
        // mask should be hex, address should be decimal
        std::cout << std::hex << bankmask << " " << rankmask << " " << colmask << " " << rowmask << " " << bankid << " " << rankid << " " << colid << " " << rowid << ", //";
      }
    }
    std::cout << std::endl;
  }

}

void FaultDomain::setTester(class TesterSystem *tester, class ECC *ecc) {
  _tester = tester;
  faultRateInfo->setTester(tester, ecc);
}

bool FaultDomain::getBadCount(ECC *ecc) {
  long a = getRetiredBlkCount();
  long b = ecc->getMaxRetiredBlkCount();
  if (a > b)
    return true;
  else
    return false;
}

void FaultDomain::setSingleChipFault() {
  Fault *fault = Fault::genRandomFault("c", this);
  operationalFaultList.push_back(fault);
}

bool FaultDomain::permFaults() {
  for (std::list<Fault *>::iterator it = operationalFaultList.begin();
       it != operationalFaultList.end(); it++) {
    if (!(*it)->getIsTransient()) {
      return true;
    }
  }
  return false;
}

ADDR FaultDomain::OverlappedAddr(void){
  ADDR result = 0;
  //printf("Errors on current system: \n");
  for (auto it = currentPossibleFaultList.begin(); it != currentPossibleFaultList.end();it++){
    result = result | ((*it)->addr & ~((*it)->mask));
    //(*it)->print();
  }

  // operationalFaultList.rbegin() means the most recent fault
  auto mostRecentFault = (*operationalFaultList.rbegin());
  result = result | ((mostRecentFault)->addr & ~((mostRecentFault)->mask));
  //mostRecentFault->print();

  return result;
}

void FaultDomain::setFaultStats(ErrorType type,int year){
  float *arrays;
  int tmparrs[ERRORENUM]={0,};
  float count = 0;
  if (type == SDC){
    arrays = this->SDCstats[year];
  }else if(type==DUE){
    arrays = this->DUEstats[year];
  }else{
    assert(0);
  }

  if (activeFaultList.size() > 0) {
    for (auto it = activeFaultList.begin();
          it != activeFaultList.end(); it++) {
      std::string name = (*it)->getName();
      // if name is the same as the fault name, then it is a permanent fault
      if (name == "Sword"){
        tmparrs[SWORDENUM] += 1;
      } else if (name =="Sbit"){
        tmparrs[SBITENUM] += 1;  
      }else if(name =="Spin"){
        tmparrs[SBITENUM] += 1;
      } else if(name =="Scol"){
        tmparrs[SCOLENUM] += 1;
      } else if(name =="Lwordline"){
        tmparrs[LWLNUM] += 1;
      } else if(name =="Srow"){
        tmparrs[SROWENUM] += 1;
      } else if(name =="Sbank"){
        tmparrs[SBANKENUM] += 1;
      } else if(name =="Mbank"){
        tmparrs[MBANKENUM] += 1;
      } else if(name =="Mrank"){
        tmparrs[MRANKENUM] += 1;
      } else if(name =="Channel"){
        tmparrs[CHANNELENUM] += 1;
      } else if(name =="BLSA"){
        tmparrs[BLSA] += 1;
      } else if(name =="Bank_pattern"){
        tmparrs[BANKPATTERN] += 1;
      } else if(name =="CDEC"){
        tmparrs[CDEC] += 1;
      } else if(name =="CSL"){
        tmparrs[CSL] += 1;
      } else if(name =="Multi_module"){
        tmparrs[MMODULE] += 1;
      } else if(name =="RDEC"){
        tmparrs[RDEC] += 1;
      } else if(name =="SWD"){
        tmparrs[SWD] += 1;
      } else if(name =="Dist_bit"){
        tmparrs[DISTBIT] += 1;
      } else if(name =="MWL"){
        tmparrs[MWL] += 1;
      }else {
        printf("Error: %s\n",name.c_str());
        assert(0);
      }

    }
  }
  if(inherentFault && inherentFault->getNumInherents() > 0){
    tmparrs[INHERENTENUM1 + inherentFault->getNumInherents()-1] += 1;
  } 
  for(int j=0;j<ERRORENUM;j++){
    if (tmparrs[j] >0){
      count += 1;
    }
  }
  for(int j=0;j<ERRORENUM;j++){
    if (tmparrs[j] >0){
      arrays[j] += 1/count;
    }
  }
}

float* FaultDomain::getFaultStats(ErrorType type,int year){
  float *arrays;
  if (type == SDC){
    arrays = this->SDCstats[year-1];
  }else if(type==DUE){
    arrays = this->DUEstats[year-1];
  }else{
    assert(0);
  }
  return arrays;
} 

float** FaultDomain::getFaultStatsALL(ErrorType type){
  float **arrays;
  if (type == SDC){
    arrays = this->SDCstats;
  }else if(type==DUE){
    arrays = this->DUEstats;
  }else{
    assert(0);
  }
  return arrays;
} 

FaultDomainDDR::FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight)
      : FaultDomain(ranksPerDomain, devicesPerRank, pinsPerDevice, blkHeight,
                    new DefaultFaultRateInfo(false,true)) {
                      SDCstats = new float*[MAX_YEAR];
                      DUEstats = new float*[MAX_YEAR];
                      for (int year =0;year<MAX_YEAR;year++){
                        SDCstats[year] = new float[ERRORENUM];
                        DUEstats[year] = new float[ERRORENUM];
                        for (int i =0;i<ERRORENUM;i++){
                          SDCstats[year][i]=0;
                          DUEstats[year][i]=0;
                        }
                      }
                    }
FaultDomainDDR::FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config)
      : FaultDomain(ranksPerDomain, devicesPerRank, pinsPerDevice, blkHeight,
                    new DefaultFaultRateInfo(false,true), message_config) {
                      SDCstats = new float*[MAX_YEAR];
                      DUEstats = new float*[MAX_YEAR];
                      for (int year =0;year<MAX_YEAR;year++){
                        SDCstats[year] = new float[ERRORENUM];
                        DUEstats[year] = new float[ERRORENUM];
                        for (int i =0;i<ERRORENUM;i++){
                          SDCstats[year][i]=0;
                          DUEstats[year][i]=0;
                        }
                      }
                    }
FaultDomainDDR::FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config,bool HBM=false)
      : FaultDomain(ranksPerDomain, devicesPerRank, pinsPerDevice, blkHeight,
                    new DefaultFaultRateInfo(HBM,true), message_config) {
                      SDCstats = new float*[MAX_YEAR];
                      DUEstats = new float*[MAX_YEAR];
                      for (int year =0;year<MAX_YEAR;year++){
                        SDCstats[year] = new float[ERRORENUM];
                        DUEstats[year] = new float[ERRORENUM];
                        for (int i =0;i<ERRORENUM;i++){
                          SDCstats[year][i]=0;
                          DUEstats[year][i]=0;
                        }
                      }
                    }

FaultDomainDDR::FaultDomainDDR(int ranksPerDomain, int devicesPerRank, int pinsPerDevice,
                 int blkHeight, MSGConfig message_config,bool HBM=false, bool detailed = true)
      : FaultDomain(ranksPerDomain, devicesPerRank, pinsPerDevice, blkHeight,
                    new DefaultFaultRateInfo(HBM,detailed), message_config) {
                      SDCstats = new float*[MAX_YEAR];
                      DUEstats = new float*[MAX_YEAR];
                      for (int year =0;year<MAX_YEAR;year++){
                        SDCstats[year] = new float[ERRORENUM];
                        DUEstats[year] = new float[ERRORENUM];
                        for (int i =0;i<ERRORENUM;i++){
                          SDCstats[year][i]=0;
                          DUEstats[year][i]=0;
                        }
                      }
                    }
FaultDomainDDR::~FaultDomainDDR() {
  delete faultRateInfo;
  for(int i=0;i<MAX_YEAR;i++){
    delete [] SDCstats[i];
    delete [] DUEstats[i];
  }
  delete[] SDCstats;
  delete[] DUEstats;
 }


//-------------------- READ_HERE_END ---------------------

