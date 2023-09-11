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

#include "LOT.hh"

//------------------------------------------------------------------------------
// LOT-ECC: LOcalized and Tiered Reliability Mechanisms
//------------------------------------------------------------------------------
LOTECC::LOTECC() : ECC(LINEAR) {}

ErrorType LOTECC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ErrorType result;

  // random data
  ECCWord dataMsg = {errorBlk.getBitN(), 0};
  for (int i = 0; i < errorBlk.getChannelWidth(); i++) {
    dataMsg.setSymbol(8, i, rand() % 0x100);
  }

  ECCWord errorMsg = {errorBlk.getBitN(), 0};
  errorMsg.extract(&errorBlk, LINEAR, 0, errorBlk.getChannelWidth());

  // check LED
  int errorDetectedCnt = 0;
  int errorDetectedChipID = -1;
  int dataArr[9][9];
  int errorArr[9][9];
  for (int chipID = 0; chipID < 9; chipID++) {
    uint32_t origChecksum = 0;
    dataArr[chipID][8] = dataMsg.getSymbol(8, chipID + 63) & 0x01;  // 7(1)+0
    dataArr[chipID][7] = (dataMsg.getSymbol(8, chipID + 54) >> 1);  // 0+7
    dataArr[chipID][6] = ((dataMsg.getSymbol(8, chipID + 54) & 0x01) << 6) |
                         (dataMsg.getSymbol(8, chipID + 45) >> 2);  // 1+6
    dataArr[chipID][5] = ((dataMsg.getSymbol(8, chipID + 45) & 0x03) << 5) |
                         (dataMsg.getSymbol(8, chipID + 36) >> 3);  // 2+5
    dataArr[chipID][4] = ((dataMsg.getSymbol(8, chipID + 36) & 0x07) << 4) |
                         (dataMsg.getSymbol(8, chipID + 27) >> 4);  // 3+4
    dataArr[chipID][3] = ((dataMsg.getSymbol(8, chipID + 27) & 0x0F) << 3) |
                         (dataMsg.getSymbol(8, chipID + 18) >> 5);  // 4+3
    dataArr[chipID][2] = ((dataMsg.getSymbol(8, chipID + 18) & 0x1F) << 2) |
                         (dataMsg.getSymbol(8, chipID + 9) >> 6);  // 5+2
    dataArr[chipID][1] = ((dataMsg.getSymbol(8, chipID + 9) & 0x3F) << 1) |
                         (dataMsg.getSymbol(8, chipID) >> 7);  // 6+1
    dataArr[chipID][0] = dataMsg.getSymbol(8, chipID) & 0x7F;  // 7+0

    uint32_t errorChecksum = 0;
    errorArr[chipID][8] = errorMsg.getSymbol(8, chipID + 63) & 0x1;   // 7(1)+0
    errorArr[chipID][7] = (errorMsg.getSymbol(8, chipID + 54) >> 1);  // 0+7
    errorArr[chipID][6] = ((errorMsg.getSymbol(8, chipID + 54) & 0x01) << 6) |
                          (errorMsg.getSymbol(8, chipID + 45) >> 2);  // 1+6
    errorArr[chipID][5] = ((errorMsg.getSymbol(8, chipID + 45) & 0x03) << 5) |
                          (errorMsg.getSymbol(8, chipID + 36) >> 3);  // 2+5
    errorArr[chipID][4] = ((errorMsg.getSymbol(8, chipID + 36) & 0x07) << 4) |
                          (errorMsg.getSymbol(8, chipID + 27) >> 4);  // 3+4
    errorArr[chipID][3] = ((errorMsg.getSymbol(8, chipID + 27) & 0x0F) << 3) |
                          (errorMsg.getSymbol(8, chipID + 18) >> 5);  // 4+3
    errorArr[chipID][2] = ((errorMsg.getSymbol(8, chipID + 18) & 0x1F) << 2) |
                          (errorMsg.getSymbol(8, chipID + 9) >> 6);  // 5+2
    errorArr[chipID][1] = ((errorMsg.getSymbol(8, chipID + 9) & 0x3F) << 1) |
                          (errorMsg.getSymbol(8, chipID) >> 7);  // 6+1
    errorArr[chipID][0] = errorMsg.getSymbol(8, chipID) & 0x7F;  // 7+0
    // calculate LED
    for (int j = 0; j < 9; j++) {
      origChecksum += dataArr[chipID][j];
      origChecksum = (origChecksum & 0x7F) + (origChecksum >> 7);
      errorChecksum += (dataArr[chipID][j] ^ errorArr[chipID][j]);
      errorChecksum = (errorChecksum & 0x7F) + (errorChecksum >> 7);
      // if (origChecksum != errorChecksum)
      // printf("%d %d %02X %02X %02X %02X\n", chipID, j, origChecksum,
      // errorChecksum, dataArr[chipID][j], errorArr[chipID][j]);
    }
    // erros on checksum itself
    errorChecksum ^= (errorMsg.getSymbol(8, chipID + 63) >> 1);

    if (origChecksum != errorChecksum) {
      errorDetectedChipID = chipID;
      errorDetectedCnt++;
    }
  }

  if (errorDetectedCnt == 0) {
    // do nothing
    result = SDC;
  } else if (errorDetectedCnt > 1) {
    result = DUE;
  } else {
    // generate error on second block
    Fault *newFault = fd->operationalFaultList.back();

    CacheLine errorBlk2 = {errorBlk.getChipWidth(), errorBlk.getChannelWidth(),
                           errorBlk.getBeatHeight()};
    for (auto it = fd->operationalFaultList.begin();
         it != fd->operationalFaultList.end(); it++) {
      if (newFault->overlap(*it) &&
          (*it)->getIsMultiColumn()) {  // other data on the same row buffer can
                                        // be affected
        (*it)->genRandomError(&errorBlk2);
      }
    }

    // assume data+LED block is the first block (A)
    int parityPlusT4[9];
    for (int j = 0; j < 8; j++) {
      parityPlusT4[j] = errorBlk2.getSymbol(8, j + 9 * 0);
    }
    parityPlusT4[8] = errorBlk.getSymbol(1, 568)
                      << 7;  // from data + LED region
    parityPlusT4[8] ^= errorBlk2.getSymbol(8, 8 + 9 * 0);

    // first check T4
    for (int j = 0; j < 9; j++) {
      if (j != errorDetectedChipID) {
        unsigned T4 = 0;
        for (int k = 0; k < 8; k++) {
          T4 ^= ((parityPlusT4[j] >> k) & 1);
        }
        assert(T4 <= 1);
        if (T4 != 0) {
          return DUE;
        }
      }
      // and remove T4
      parityPlusT4[j] &= 0x7F;
    }

    // Now, correct parity with "parity of parity"
    int parity = 0;
    for (int deviceID = 0; deviceID < 9; deviceID++) {
      if (deviceID != errorDetectedChipID) {
        parity ^= parityPlusT4[deviceID];
      }
    }
    parityPlusT4[errorDetectedChipID] = parity;

    // Now, correct data with parity
    int parityArr[9];
    for (int j = 0; j < 9; j++) {
      parityArr[j] = parityPlusT4[j];
    }
    for (int deviceID = 0; deviceID < 9; deviceID++) {
      if (deviceID != errorDetectedChipID) {
        for (int j = 0; j < 9; j++) {
          parityArr[j] ^= errorArr[deviceID][j];
        }
      }
    }
    for (int j = 0; j < 9; j++) {
      errorArr[errorDetectedChipID][j] = parityArr[j];
    }

    // check any error
    result = CE;
    for (int blockID = 0; blockID < 9; blockID++) {
      if (blockID != errorDetectedChipID) {
        for (int j = 0; j < 9; j++) {
          if (errorArr[blockID][j]) {
            result = SDC;  // found an error on chip other than detected.
          }
        }
      }
    }
    // if (result==CE) {
    //    printf("CE %d @%d\n", errorDetectedCnt, errorDetectedChipID);
    //    errorBlk.print();
    //    errorBlk2.print();
    //    for (int j=8; j>=0; j--) {
    //        for (int deviceID=8; deviceID>=0; deviceID--) {
    //            printf("%02x ", errorArr[deviceID][j]);
    //        }
    //        printf("%02x\n", parityArr[j]);
    //    }
    //}
    // if (result==SDC) {
    //    printf("SDC\n");
    //    fd->print();
    //    errorBlk.print();
    //    printf("Detection #%d at %d\n", errorDetectedCnt,
    //    errorDetectedChipID);
    //    errorBlk2.print();
    //    printf("-----\n");
    //    for (int deviceID=8; deviceID>=0; deviceID--) {
    //        printf("%02x ", parityPlusT4[deviceID]);
    //    }
    //    printf("\n");
    //    for (int j=8; j>=0; j--) {
    //        for (int deviceID=8; deviceID>=0; deviceID--) {
    //            printf("%02x ", errorArr[deviceID][j]);
    //        }
    //        printf("%02x\n", parityArr[j]);
    //    }
    //}
  }

  assert(result != NE);

  return result;
}
