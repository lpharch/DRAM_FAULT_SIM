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

#include "MultiECC.hh"
#include "rs.hh"

//------------------------------------------------------------------------------
// “Low-power, low-storage-overhead chipkill correct via multi-line error
// correction"
//------------------------------------------------------------------------------
MultiECC::MultiECC() : ECC(MULTIX8) {
  configList.push_back({0, 0, new RS<2, 16>("S16DC (M-ECC)\t", 9, 1, 0)});
}

ErrorType MultiECC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  Codec *codec = configList.front().codec;

  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  ErrorType result = NE, newResult;
  for (int i = errorBlk.getBeatHeight() / 2 - 1; i >= 0; i--) {
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());

    if (msg.isZero()) {  // error-free region of a block -> skip
      newResult = NE;
    } else {
      newResult = codec->decode(&msg, &decoded);

      if (newResult == DUE) {
        // 1. error is detected by T1EC
        int nonZeroChecksumCount = 0;
        Fault *nonZeroChecksumFault = NULL;

        Fault *newFault = fd->operationalFaultList.back();
        for (auto it = fd->operationalFaultList.begin();
             it != fd->operationalFaultList.end(); it++) {
          if (!newFault->overlap(*it)) {  // unrelated error
            continue;
          }

          // 2. generate checksum
          if ((*it)->getIsMultiRow()) {
            uint32_t origChecksum = 0;
            uint32_t errorChecksum = 0;

            for (int j = 0; j < 256; j++) {
              uint16_t data = rand() % 0x10000;
              uint16_t error;
              if (j == 0) {
                error = decoded.getSymbol(16, (*it)->getChipID());
              } else {
                if (!(*it)->getIsSingleDQ()) {
                  error = rand() % 0x10000;
                } else {
                  int pinLoc = (*it)->getPinID() % 8;
                  error =
                      ((rand() % 2) << pinLoc) | (rand() % 2 << (pinLoc + 8));
                }
              }
              origChecksum += data;
              origChecksum = (origChecksum & 0xFFFF) + (origChecksum >> 16);
              errorChecksum += (data ^ error);
              errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum >> 16);
            }
            if (origChecksum != errorChecksum) {
              nonZeroChecksumCount++;
              nonZeroChecksumFault = *it;
            }
          } else {
            nonZeroChecksumCount++;
            nonZeroChecksumFault = *it;
          }
        }
        if (nonZeroChecksumCount == 1) {
          // correct it
          decoded.setSymbol(16, nonZeroChecksumFault->getChipID(), 0);

          if (decoded.isZero()) {
            newResult = CE;
          } else {
            newResult = SDC;
          }
        } else {
          newResult = DUE;
        }
      } else {
        newResult = SDC;
      }
    }

    result = worse2ErrorType(result, newResult);
  }

  assert(result != NE);

  return result;
}
