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

#include "DUO.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"
#include "combination.hh"
//#define BF_search

DUO::DUO(int _maxPin): ECC(PIN9), maxPin(_maxPin){
}

DUO::DUO(int _maxPin, bool _doPostprocess, bool _doRetire,
      int _maxRetiredBlkCount, ECCLayout layout) : 
      ECC(layout, _doPostprocess, _doRetire, _maxRetiredBlkCount),maxPin(_maxPin)
{
  

}

//------------------------------------------------------------------------------
DUO64bx4::DUO64bx4(int _maxPin) : DUO(PIN9){
  codec = new RS<2, 8>("3.5PC\t16\t4\t", 64, 7, maxPin);
  this->setInDRAM(Triple);
  this->setInDRAMDown(Double);
  setBitN(544);
}

ErrorType DUO64bx4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  // Up to 3-sym corrections
  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());

  result = codec->decode(&msg, &decoded, &correctedPosSet);

  if ((result == CE) || (result == SDC)) {
    parity = 0;
    for (int i = 0; i < 15; i++) {
      parity ^= decoded.getBit(36 * i);
    }
    parityError = (parity != decoded.getBit(512));

    if (parityError) {
      correctedPosSet.clear();
      return DUE;
    }
  } else if (result == DUE) {
    // burst error correction
    result = codec->decodeBurstDUO64bx4(&msg, &decoded, 4, &correctedPosSet);

    if ((result == CE) || (result == SDC)) {
      parity = 0;
      for (int i = 0; i < 15; i++) {
        parity ^= decoded.getBit(36 * i);
      }
      parityError = (parity != decoded.getBit(512));

      if (parityError) {
        correctedPosSet.clear();
        return DUE;
      }
    }
  } else {
    assert(0);  // NE
  }

  return result;
}

unsigned long long DUO64bx4::getInitialRetiredBlkCount(FaultDomain *fd,
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

void DUO64bx4::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  return;
}

//------------------------------------------------------------------------------
// GONG

DUO72bx4::DUO72bx4(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount)
    : DUO(_maxPin, _doPostprocess, _doRetire, _maxRetiredBlkCount, duoBL9)
  {
  // DUO72bx4::DUO72bx4(int _maxPin) : ECC(PIN), maxPin(_maxPin) {
  codec = new RS<2, 8>("3.5PC\t16\t4\t", 76, 12, 6);
  rs_dual_first = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 8);
  rs_dual_second =
      new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 10);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Septa);
  this->setInDRAMDown(Triple);
  setBitN(612);
}

ErrorType DUO72bx4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
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
    int BF_access;
#ifdef BF_search
    BF_access = bruteForceSearch(fd,&chip_list,&errorBlk,18);
    if (BF_access>sizeof(BF_Stat)/sizeof(int))
      BF_access = sizeof(BF_Stat)/sizeof(int)-1;
    BF_Stat[BF_access]++;
#else
    fd->FaultyChipDetect(&chip_list);
#endif
    int size = chip_list.size();
    if (size == 2) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 4;
        // set 8 symbols of each x4 chip as ersure symbols
        for (int j = startPos; j < startPos + 4; j++) {
          ErasureLocation->push_back(j);
        }
      }

      // correct one symbol by using 4bit parity
      CorrectByParity_internal(&tmp_msg, &errorBlk, &chip_list);  // faultyChip);

      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
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

void DUO72bx4::CorrectByParity_internal(ECCWord *msg, Block *errorBlk, std::list<int>* chip_list) {  // int chipID){
  bool cor0, cor1;
  cor0 = cor1 = false;
  int offset = 18 * 8 * 4;
  int chipID[2];
  int cnt = 0;
  for (std::list<int>::iterator it = chip_list->begin(); it != chip_list->end();
       it++) {
    chipID[cnt] = (*it);  // printf("chipID[%i]: %i\n", cnt, chipID[cnt]);
    cnt++;
  }
  // FIXME: this on-chip parity correction was wrong. Need to add more accurate
  // codes.
  for (int chip = 0; chip < 16; chip++) {
    if (chip != chipID[0] && chip != chipID[1]) {
      cor0 ^= msg->getBit(offset + chip * 2 + 0);
      cor1 ^= msg->getBit(offset + chip * 2 + 1);
    }
  }
  cor0 ^= errorBlk->getBit(offset + 8 * 8 + 0);
  cor1 ^= errorBlk->getBit(offset + 8 * 8 + 1);
  cor0 ^= errorBlk->getBit(offset + 8 * 8 + 2);
  cor1 ^= errorBlk->getBit(offset + 8 * 8 + 3);

  msg->setBit(offset + chipID[0] * 2 + 0, cor0);
  msg->setBit(offset + chipID[0] * 2 + 1, cor1);
  msg->setBit(offset + chipID[1] * 2 + 0, cor0);
  msg->setBit(offset + chipID[1] * 2 + 1, cor1);
}

unsigned long long DUO72bx4::getInitialRetiredBlkCount(FaultDomain *fd,
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

void DUO72bx4::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip * 4;
  // set 8 symbols of each x4 chip as ersure symbols
  for (int j = startPos; j < startPos + 4; j++) {
    ErasureLocation->push_back(j);
  }
}

DUO72bx8::DUO72bx8(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount)
    : DUO(_maxPin, _doPostprocess, _doRetire, _maxRetiredBlkCount, duoBL9)
  {
  codec = new RS<2, 8>("3.5PC\t16\t4\t", 76, 12, 6);
  rs_dual_first = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 8);
  rs_dual_second =
      new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 10);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Septa);
  this->setInDRAMDown(Triple);
  setBitN(612);
}

ErrorType DUO72bx8::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  // errorBlk.print();
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
    int BF_access;
#ifdef BF_search
    BF_access = bruteForceSearch(fd,&chip_list,&errorBlk,9);
    if (BF_access>sizeof(BF_Stat)/sizeof(int))
      BF_access = sizeof(BF_Stat)/sizeof(int)-1;
    BF_Stat[BF_access]++;
#else
    fd->FaultyChipDetect(&chip_list);
#endif
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();

      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 8;
        // correct one symbol by using 4bit parity
        CorrectByParity_internal(&tmp_msg, &errorBlk, faultyChip);
        // set 8 symbols of each x4 chip as ersure symbols
        for (int j = startPos; j < startPos + 8; j++) {
          ErasureLocation->push_back(j);
        }
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
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
      } else {
        // GONG: testing aggressive correction
        tmp_msg.clone(&msg);
        std::set<int> erasure_set;
        erasure_set.insert(72);
        erasure_set.insert(73);
        erasure_set.insert(74);
        erasure_set.insert(75);

        if(faultyChip==0 || faultyChip==1) {
          ErasureLocation->push_back(72);
          erasure_set.erase(72);
        }else if(faultyChip==2 || faultyChip==3)
        {
          ErasureLocation->push_back(73);
          erasure_set.erase(73);
        }else if(faultyChip==4 || faultyChip==5)
        {
          ErasureLocation->push_back(74);
          erasure_set.erase(74);
        }else if(faultyChip==6 || faultyChip==7)
        {
          ErasureLocation->push_back(75);
          erasure_set.erase(75);
        }
        //try all three combinations
        for(auto it = erasure_set.begin(); it != erasure_set.end(); it++){
          tmp_correctedPos.clear();
          ErasureLocation->push_back(*it);
          tmp_result = rs_dual_second->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos, ErasureLocation);
          if(tmp_result==CE || tmp_result==SDC){
            break;
          } 
          else{
            ErasureLocation->pop_back();
          }
        }

        if(tmp_result==CE || tmp_result==SDC){
          return tmp_result;
        }
        else{
          return DUE;
        }
      }
    } else {  // DUE?
    }

  } else {
    assert(0);  // NE
  }

  return result;
}


void DUO72bx8::CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID){
  bool cor0, cor1, cor2, cor3;
  cor0 = cor1 = cor2 = cor3 = false;
  int offset = 9 * 8 * 8;
  for (int chip = 0; chip < 8; chip++) {
    if (chip != chipID) {
      cor0 ^= errorBlk->getBit(offset + chip * 8 + 0);
      cor1 ^= errorBlk->getBit(offset + chip * 8 + 1);
      cor2 ^= errorBlk->getBit(offset + chip * 8 + 2);
      cor3 ^= errorBlk->getBit(offset + chip * 8 + 3);
    }
  }
  cor0 ^= errorBlk->getBit(offset + 8 * 8 + 0);
  cor1 ^= errorBlk->getBit(offset + 8 * 8 + 1);
  cor2 ^= errorBlk->getBit(offset + 8 * 8 + 2);
  cor3 ^= errorBlk->getBit(offset + 8 * 8 + 3);

  msg->setBit(offset + chipID * 4 + 0, cor0);
  msg->setBit(offset + chipID * 4 + 1, cor1);
  msg->setBit(offset + chipID * 4 + 2, cor2);
  msg->setBit(offset + chipID * 4 + 3, cor3);
}

unsigned long long DUO72bx8::getInitialRetiredBlkCount(FaultDomain *fd,
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


void DUO72bx8::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip * 8;
  // correct one symbol by using 4bit parity
  // set 8 symbols of each x4 chip as ersure symbols
  for (int j = startPos; j < startPos + 8; j++) {
    ErasureLocation->push_back(j);
  }
}

//------------------------------------------------------------------------------
DUO36bx4::DUO36bx4(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount)
    : DUO(_maxPin, _doPostprocess, _doRetire, _maxRetiredBlkCount, duoBL17)
  {
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 76, 12, maxPin, 9);
  decoder = new RS<2, 8>("1.5PC\t16\t4\t", 76, 12, maxPin, 9);
  rs_dual_first = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 8);
  rs_dual_second =
      new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12, 10);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Septa);
  this->setInDRAMDown(Triple);
  setBitN(612);
}

//-------------------- READ_HERE ---------------------
ErrorType DUO36bx4::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  // errorBlk.print();
  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  // Up to 6-sym corrections
  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());
  result = decoder->decode(&msg, &decoded, &correctedPosSet);

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
    // IF result is DUE, try erasure fix
    ErrorType tmp_result = result;
    std::list<int> chip_list;
    int faultyChip;
    int BF_access;
#ifdef BF_search
    // Enable BF search.
    // input : current errorBLK
    // output: chip_list; chiplist which would have chip level faults. 
    //         BF_access; total brute force access count. 
    BF_access = bruteForceSearch(fd,&chip_list,&errorBlk,9);

    // Statistic update
    if (BF_access>sizeof(BF_Stat)/sizeof(int))
      BF_access = sizeof(BF_Stat)/sizeof(int)-1;
    BF_Stat[BF_access]++;

#else
    fd->FaultyChipDetect(&chip_list);
#endif    
    // Remaining part would be the same... Erasure search
    // If you believe previously implemented code, Don't need to review under this part.

    //-------------------- READ_HERE_END ---------------------
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 8;
        // printf("chipID: %d\tstartPos: %d\n", chip->getChipID(), startPos);
        // correct one symbol by using 4bit parity
        CorrectByParity_internal(&tmp_msg, &errorBlk, faultyChip);
        // set 8 symbols of each x4 chip as ersure symbols
        for (int j = startPos; j < startPos + 8; j++) {
          ErasureLocation->push_back(j);
        }
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                     ErasureLocation);

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        printf("SDC happen during erasure fix\n\n");
        tmp_msg.print();
        tmp_decoded.print();
        return SDC;
      } else {
        // GONG: testing aggressive correction
        tmp_msg.clone(&msg);
        std::set<int> erasure_set;
        erasure_set.insert(72);
        erasure_set.insert(73);
        erasure_set.insert(74);
        erasure_set.insert(75);


        if(faultyChip==0 || faultyChip==1) 	{
                ErasureLocation->push_back(72);
                erasure_set.erase(72);
        }else if(faultyChip==2 || faultyChip==3) {
                ErasureLocation->push_back(73);
                erasure_set.erase(73);
        }else if(faultyChip==4 || faultyChip==5) {
                ErasureLocation->push_back(74);
                erasure_set.erase(74);
        }else if(faultyChip==6 || faultyChip==7) {
                ErasureLocation->push_back(75);
                erasure_set.erase(75);
        }
        //try all three combinations
        for(std::set<int>::iterator it = erasure_set.begin(); it !=
        erasure_set.end(); it++){
                tmp_correctedPos.clear();
                ErasureLocation->push_back(*it);
                tmp_result = rs_dual_second->decode(&tmp_msg, &tmp_decoded,
        &tmp_correctedPos, ErasureLocation);
                if(tmp_result==CE || tmp_result==SDC) break;
                else{
                  ErasureLocation->pop_back();
                }
        }

        if(tmp_result==CE || tmp_result==SDC){
          if(tmp_result==SDC){
            printf("SDC happen During 4bit+erasure fix\n\n");
            tmp_msg.print();
            tmp_decoded.print();
         }
         return tmp_result;
        }
        else{
          return DUE;
        }
      }
    } else {  // DUE?
    }
  } else {
    assert(0);  // NE
  }
  if (result == SDC){
    printf("SDC happen at the END?\n\n");
    msg.print();
    decoded.print();
  }
  return result;
}




//-------------------- READ_HERE ---------------------


unsigned long long within_row_addr(){
        return 0x8000;
};
unsigned long long within_bank_addr(){
        return 1;
};

/* Only the same row access...
DUE
0.00002000000
0.00008000000
0.00016000000
0.00021000000
SDC
0.00000000000
0.00004000000
0.00005000000
0.00005000000

10X more samples 
DUE
0.00001100000
0.00004300000
0.00011700000
0.00018900000
SDC
0.00000500000
0.00002900000
0.00006300000
0.00010900000
*/
//-------------------- READ_HERE_END ---------------------


unsigned long long DUO36bx4::getInitialRetiredBlkCount(FaultDomain *fd,
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

bool DUO36bx4::ParityCheck(ECCWord *decoded, Block *errorBlk) {
  bool parity0, parity1, parity2, parity3, parityError;
  parity0 = parity1 = parity2 = parity3 = false;
  for (int i = 0; i < 9 * 17; i++) {
    parity0 ^= decoded->getBit(4 * i + 0);
    parity1 ^= decoded->getBit(4 * i + 1);
    parity2 ^= decoded->getBit(4 * i + 2);
    parity3 ^= decoded->getBit(4 * i + 3);
  }
  parityError = (parity0 ^ errorBlk->getBit(36 * 16 + 32)) |
                (parity1 ^ errorBlk->getBit(36 * 16 + 33)) |
                (parity2 ^ errorBlk->getBit(36 * 16 + 34)) |
                (parity3 ^ errorBlk->getBit(36 * 16 + 35));
  return parityError;
}

void DUO36bx4::CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID) {
  bool cor0, cor1, cor2, cor3;
  cor0 = cor1 = cor2 = cor3 = false;
  int offset = 9 * 8 * 8;
  for (int chip = 0; chip < 8; chip++) {
    if (chip != chipID) {
      cor0 ^= errorBlk->getBit(offset + chip * 4 + 0);
      cor1 ^= errorBlk->getBit(offset + chip * 4 + 1);
      cor2 ^= errorBlk->getBit(offset + chip * 4 + 2);
      cor3 ^= errorBlk->getBit(offset + chip * 4 + 3);
      // printf("@ chip %i: %i %i %i %i \n", chip, cor0, cor1, cor2, cor3);
    }
  }
  cor0 ^= errorBlk->getBit(offset + 8 * 4 + 0);
  cor1 ^= errorBlk->getBit(offset + 8 * 4 + 1);
  cor2 ^= errorBlk->getBit(offset + 8 * 4 + 2);
  cor3 ^= errorBlk->getBit(offset + 8 * 4 + 3);

  msg->setBit(offset + chipID * 4 + 0, cor0);
  msg->setBit(offset + chipID * 4 + 1, cor1);
  msg->setBit(offset + chipID * 4 + 2, cor2);
  msg->setBit(offset + chipID * 4 + 3, cor3);
}

void DUO36bx4::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip * 8;
  // set 8 symbols of each x4 chip as ersure symbols
  for (int j = startPos; j < startPos + 8; j++) {
    ErasureLocation->push_back(j);
  }
}


////////////////////////////////////////////////////////////////////////////////////

DUO64bx4_::DUO64bx4_(int _maxPin, bool _doPostprocess, bool _doRetire,
                     int _maxRetiredBlkCount)
    : DUO(_maxPin, _doPostprocess, _doRetire, _maxRetiredBlkCount,duoBL9full)
  {
  codec = new RS<2, 8>("3.5PC\t16\t4\t", 72, 8, 4);
  rs_dual_first = new RS_DUAL<2, 8>("5 symbol erasure + 1 symbol error", 72, 8, 5);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Penta);
  this->setInDRAMDown(Double);
  setBitN(612);  // actually 576 bits but number of blocks calculation assumes
                 // +6.25% on-chip & 12.5% chip redundancy
}

ErrorType DUO64bx4_::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};
  correctedPosSet.clear();
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
    if (size == 1) {  // faulty chip found
      ECCWord tmp_msg = {codec->getBitN(), codec->getBitK()};
      ECCWord tmp_decoded = {codec->getBitN(), codec->getBitK()};
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 4;
        // set 8 symbols of each x4 chip as ersure symbols
        for (int j = startPos; j < startPos + 4; j++) {
          ErasureLocation->push_back(j);
        }
        ErasureLocation->push_back(64 + (faultyChip / 2));
      }

      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                     ErasureLocation);
      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        printf("SDC from erasure RS correction\n");
        errorBlk.print();
        return SDC;
      }
    } else {  // DUE?
    }

  } else {
    assert(0);  // NE
  }

  return result;
}

unsigned long long DUO64bx4_::getInitialRetiredBlkCount(FaultDomain *fd,
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


void DUO64bx4_::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip * 4;
  // set 8 symbols of each x4 chip as ersure symbols
  for (int j = startPos; j < startPos + 4; j++) {
    ErasureLocation->push_back(j);
  }
  ErasureLocation->push_back(64 + (faultychip / 2));

}

//------------------------------------------------------------------------------
DUO20bx4_34::DUO20bx4_34(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount)
    : DUO(_maxPin,_doPostprocess, _doRetire, _maxRetiredBlkCount, duoBL34)
  {
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 85, 21, maxPin, 17);
  decoder = new RS<2, 8>("1.5PC\t16\t4\t", 85, 21, maxPin, 17);
  rs_dual_first = new RS_DUAL<2, 8>("17 symbol erasure + 2 symbol error", 85, 21, 17);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Septa); //Elev
  this->setInDRAMDown(Quad);
  setBitN(680);
}

ErrorType DUO20bx4_34::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  // errorBlk.print();
  bool synError;
  bool parity, parityError;
  ErrorType result;

  if (errorBlk.isZero()) {
    return NE;
  }

  // Up to 6-sym corrections
  msg.extract(&errorBlk, layout, 0, errorBlk.getChannelWidth());
  result = decoder->decode(&msg, &decoded, &correctedPosSet);

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
    int BF_access;
#ifdef BF_search
    BF_access = bruteForceSearch(fd,&chip_list,&errorBlk,5);
    if (BF_access>sizeof(BF_Stat)/sizeof(int))
      BF_access = sizeof(BF_Stat)/sizeof(int)-1;
    BF_Stat[BF_access]++;
#else
    fd->FaultyChipDetect(&chip_list);
#endif
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
        ErasureLocation->push_back(16*5+faultyChip);
      }
      std::set<int> tmp_correctedPos;
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                     ErasureLocation);

      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        printf("SDC happen during erasure fix\n\n");
        tmp_msg.print();
        tmp_decoded.print();
        return SDC;
      } 
    } else {  // DUE?
    }
  } else {
    assert(0);  // NE
  }
  if (result == SDC){
    printf("SDC happen at the END?\n\n");
    msg.print();
    decoded.print();
  }
  return result;
}


unsigned long long DUO20bx4_34::getInitialRetiredBlkCount(FaultDomain *fd,
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

void DUO20bx4_34::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip*16;
      for (int j = startPos; j < startPos + 16; j++) {
        ErasureLocation->push_back(j);
      }

      ErasureLocation->push_back(16*5+faultychip);
}

//------------------------------------------------------------------------------
DUO20bx4_33::DUO20bx4_33(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount)
    : DUO(_maxPin,_doPostprocess, _doRetire, _maxRetiredBlkCount, duoBL33)
  {
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 82, 18, maxPin, 17);
  rs_dual_first = new RS_DUAL<2, 8>("17 symbol erasure", 82, 18, 16);
  rs_dual_second = new RS_DUAL<2, 8>("17 symbol erasure + 2 symbol error", 82, 18, 17);
  ErasureLocation = new std::list<int>;
  this->setInDRAM(Octa);
  this->setInDRAMDown(Single);
  setBitN(660);
}

ErrorType DUO20bx4_33::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = ECCWord(codec->getBitN(), codec->getBitK());
  ECCWord decoded = ECCWord(codec->getBitN(), codec->getBitK());

  ECCWord tmp_msg = ECCWord(codec->getBitN(), codec->getBitK());
  ECCWord tmp_decoded = ECCWord(codec->getBitN(), codec->getBitK());

  std::set<int> tmp_correctedPos;
  std::list<int> chip_list;


  // errorBlk.print();
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
    int faultyChip;
    int BF_access;
#ifdef BF_search
    BF_access = bruteForceSearch(fd,&chip_list,&errorBlk,5);
    if (BF_access>sizeof(BF_Stat)/sizeof(int))
      BF_access = sizeof(BF_Stat)/sizeof(int)-1;
    BF_Stat[BF_access]++;
#else
    fd->FaultyChipDetect(&chip_list);
#endif
    int size = chip_list.size();
    if (size == 1) {  // faulty chip found
      
      tmp_msg.clone(&msg);
      ErasureLocation->clear();
      for (auto it = chip_list.begin(); it != chip_list.end(); it++) {
        faultyChip = (*it);
        int startPos = faultyChip * 16;
        CorrectByParity_internal(&tmp_msg, &errorBlk, faultyChip);
        for (int j = startPos; j < startPos + 16; j++) {
          ErasureLocation->push_back(j);
        }
      }
      
      
      tmp_result = rs_dual_first->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos,
                                     ErasureLocation);
      
      if (tmp_result == CE) {
        this->correctedPosSet.clear();
        for (std::set<int>::iterator it = tmp_correctedPos.begin();
             it != tmp_correctedPos.end(); ++it) {
          this->correctedPosSet.insert(*it);
        }
        return CE;
      } else if (tmp_result == SDC) {
        printf("SDC happen during erasure fix\n\n");
        tmp_msg.print();
        tmp_decoded.print();
        return SDC;
      } else {
        // GONG: testing aggressive correction
        tmp_msg.clone(&msg);
        std::set<int> erasure_set;
        erasure_set.insert(80);
        erasure_set.insert(81);
  
        if(faultyChip==0 || faultyChip==1) 	{
                ErasureLocation->push_back(80);
                erasure_set.erase(80);
        }else if(faultyChip==2 || faultyChip==3) {
                ErasureLocation->push_back(81);
                erasure_set.erase(81);
        }
        //try all three combinations
        for(std::set<int>::iterator it = erasure_set.begin(); it != erasure_set.end(); it++){
          tmp_correctedPos.clear();
          ErasureLocation->push_back(*it);
          tmp_result = rs_dual_second->decode(&tmp_msg, &tmp_decoded, &tmp_correctedPos, ErasureLocation);
          if(tmp_result==CE || tmp_result==SDC) {
            break;
          }
          else{
            ErasureLocation->pop_back();
          }
        }

        if(tmp_result==CE || tmp_result==SDC){
          if(tmp_result==SDC){
            printf("SDC happen During 4bit+erasure fix\n\n");
            tmp_msg.print();
            tmp_decoded.print();
         }
         return tmp_result;
        }
        else{
          return DUE;
        }
      }
    } else {  // DUE?
    }
  } else {
    assert(0);  // NE
  }

  if (result == SDC){
    printf("SDC happen at the END?\n\n");
    msg.print();
    decoded.print();
  }
  return result;
}

void DUO20bx4_33::CorrectByParity_internal(ECCWord *msg, Block *errorBlk, int chipID) {
  bool cor0, cor1, cor2, cor3;
  cor0 = cor1 = cor2 = cor3 = false;
  int offset = 5*32*4; // num_chip*BL*chip_width
  for (int chip = 0; chip < 4; chip++) {
    if (chip != chipID) {
      cor0 ^= errorBlk->getBit(offset + chip * 4 + 0);
      cor1 ^= errorBlk->getBit(offset + chip * 4 + 1);
      cor2 ^= errorBlk->getBit(offset + chip * 4 + 2);
      cor3 ^= errorBlk->getBit(offset + chip * 4 + 3);
      // printf("@ chip %i: %i %i %i %i \n", chip, cor0, cor1, cor2, cor3);
    }
  }
  cor0 ^= errorBlk->getBit(offset + 4 * 4 + 0);
  cor1 ^= errorBlk->getBit(offset + 4 * 4 + 1);
  cor2 ^= errorBlk->getBit(offset + 4 * 4 + 2);
  cor3 ^= errorBlk->getBit(offset + 4 * 4 + 3);

  msg->setBit(offset + chipID * 4 + 0, cor0);
  msg->setBit(offset + chipID * 4 + 1, cor1);
  msg->setBit(offset + chipID * 4 + 2, cor2);
  msg->setBit(offset + chipID * 4 + 3, cor3);
}

unsigned long long DUO20bx4_33::getInitialRetiredBlkCount(FaultDomain *fd,
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

void DUO20bx4_33::getErasureLocation_internal(std::list<int>* ErasureLocation,int faultychip){
  int startPos = faultychip*16;
      for (int j = startPos; j < startPos + 16; j++) {
        ErasureLocation->push_back(j);
      }
}

int DUO::bruteForceSearch(FaultDomain *fd, list<int> *chip_list,
                CacheLine* blk,int num_chips){
  
  //Initialization
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};
  CacheLine _blk = {
      fd->pinsPerDevice,
      (fd->devicesPerRank - (int)fd->retiredChipIDList.size()) * fd->pinsPerDevice -
          (int)fd->retiredPinIDList.size(),
      fd->blkHeight};
  
  int BF_access_cnt = 0;
  std::vector< vector <int> >candidates;
  std::vector<int> lists;
  Fault* newFault = *fd->operationalFaultList.rbegin();
  ADDR cur_addr= 0;
  std::list<int> *ErasureLocation;
  ErasureLocation = new std::list<int>;

  int confidency = 0;

  //Total number of chips == 9
  for(int i =0;i<num_chips;i++){
    lists.push_back(i);
  }

  //Generate Candidate faulty cihps
  Combination< int,vector<int> >(lists,1,candidates);
  //candidates == {0,1,2,3,4,5,6,7,8}
  
  const int MAX_TRIAL = 16;
  for (int trial=0 ; trial<MAX_TRIAL; trial++){  
    std::vector< vector <int> > tmp_candidates;
    //For entire chips 
    for(auto it = candidates.begin();it != candidates.end();it++){
      msg.extract(blk, layout, 0, blk->getChannelWidth());
      ErasureLocation->clear();
      int faultyChip1 = (*it)[0];
      
      CorrectByParity(&msg,blk,faultyChip1);
      getErasureLocation(ErasureLocation,faultyChip1);

      std::set<int> tmp_correctedPos;
      ErrorType tmp_result = rs_dual_first->decode(&msg, &decoded, &tmp_correctedPos,
                                      ErasureLocation);
      if(tmp_result == CE || tmp_result == SDC){
        tmp_candidates.push_back(*it);
      }
      BF_access_cnt++;
    }

    if (tmp_candidates.size()>0){
      candidates = tmp_candidates;
      if (candidates.size() != tmp_candidates.size())
        confidency = 0;
      confidency++;
    } 

    // After we try the first original blk, use other blks.
    blk = &_blk;
    _blk.reset();


    cur_addr = fd->OverlappedAddr();
    cur_addr += within_row_addr()*trial;

    Fault *Testfault = new SingleBitFault(fd, true);
    Testfault->addr = cur_addr;

    //printf("Next address : %llx\n",cur_addr);
    //printf("Overlapped faults : \n");

    for (auto it = fd->operationalFaultList.begin(); it != fd->operationalFaultList.end();it++){
      if (Testfault->overlap(*it)){
        (*it)->genRandomError(blk);
        //(*it)->print();
      }
    }
    
    fd->inherentFault->genRandomError(blk);
    delete Testfault;
    //printf("\n\n");

  } // end while
  //printf("confidency : %d\n",confidency);
  for(auto jt = candidates.begin();jt != candidates.end();jt++)
  {
    if(confidency>MAX_TRIAL/2){
      chip_list->push_back((*jt)[0]);
    }
  }
  delete ErasureLocation;
  
  //assert(candidates.size() == 1);
  return BF_access_cnt;
}
void DUO::getErasureLocation(std::list<int>* ErasureLocation,int faultychip){
  getErasureLocation_internal(ErasureLocation,faultychip);
}
void DUO::CorrectByParity(ECCWord *msg, Block *errorBlk, int chipID){
  CorrectByParity_internal(msg,errorBlk,chipID);
}

DUO36bx4_meta::DUO36bx4_meta(int _maxPin, bool _doPostprocess, bool _doRetire,
      int maxRetiredBlkCount, int meta_bytes):
      DUO36bx4(_maxPin,_doPostprocess,_doRetire,maxRetiredBlkCount){

  
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 76, 12-meta_bytes, maxPin, 9);
  decoder = new RS<2, 8>("1.5PC\t16\t4\t", 76, 12-meta_bytes, maxPin, 9);
  rs_dual_first = new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12-meta_bytes, 8);
  rs_dual_second =
      new RS_DUAL<2, 8>("8 symbol erasure + 2 symbol error", 76, 12-meta_bytes, 10);
  ErasureLocation = new std::list<int>;
  
  switch (meta_bytes)
  {
  case 0:
    this->setInDRAM(Septa);
    this->setInDRAMDown(Triple);
    break;
  case 1:
    this->setInDRAM(Hexa);
    this->setInDRAMDown(Triple);
    break;
  case 2:
    this->setInDRAM(Hexa);
    this->setInDRAMDown(Double);
    break;
  case 3:
    this->setInDRAM(Penta);
    this->setInDRAMDown(Double);
    break;
  default:
    assert(1);
    break;
  }
  setBitN(612);

}


DUO20bx4_34_meta::DUO20bx4_34_meta(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount, int meta_bytes)
    : DUO20bx4_34(_maxPin,_doPostprocess, _doRetire, _maxRetiredBlkCount){
  codec = new RS<2, 8>("1.5PC\t16\t4\t", 85, 21-meta_bytes, maxPin, 17);
  rs_dual_first = new RS_DUAL<2, 8>("17 symbol erasure + 2 symbol error", 85, 21-meta_bytes, 17);
  ErasureLocation = new std::list<int>;
  decoder = new RS<2, 8>("1.5PC\t16\t4\t", 85, 21-meta_bytes, maxPin, 17);

  switch (meta_bytes){
  case 0:
    this->setInDRAM(Septa); //Elev
    this->setInDRAMDown(Quad);
    break;
  case 1:
    this->setInDRAM(Septa); //Deca
    this->setInDRAMDown(Triple);
    break;
  case 2:
    this->setInDRAM(Septa); //Deca
    this->setInDRAMDown(Double);
    break;
  case 3:
    this->setInDRAM(Septa); //Nona
    this->setInDRAMDown(Double);
    break;
  default:
    assert(1);
    break;
  } 
  setBitN(680);
}
