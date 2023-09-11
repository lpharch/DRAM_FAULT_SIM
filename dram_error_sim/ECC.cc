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

#include "DomainGroup.hh"
#include "ECC.hh"
#include "FaultDomain.hh"
#include "codec.hh"
#include "hsiao.hh"
#include "rs.hh"

extern std::default_random_engine randomGenerator;

//------------------------------------------------------------------------------
ErrorType worse2ErrorType(ErrorType a, ErrorType b) {
  // if ((a==SDC) || (b==SDC)) {
  //    return SDC;
  //}
  if ((a == DUE) || (b == DUE)) {
    return DUE;
  }
  return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
ErrorType ECC::decode(FaultDomain *fd, CacheLine &errorBlk) {
  ErrorType result;

  // clear corrected position information
  clear();

  // do decoding
  result = decodeInternal(fd, errorBlk);

  if (doPostprocess) {
    result = postprocess(fd, result);
  }
  return result;
}

ErrorType ECC::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  // find appropriate CODEC
  Codec *codec = NULL;
  for (auto it = configList.begin(); it != configList.end(); it++) {
    if ((fd->getRetiredChipCount() <= it->maxDeviceRetirement) &&
        (fd->getRetiredPinCount() <= it->maxPinRetirement)) {
      codec = it->codec;
      // assert((codec->getBitN()%errorBlk.getChannelWidth())==0);
    }
  }
  if (codec == NULL) {
    if (errorBlk.isZero())
      return NE;
    else
      return SDC;
  }

  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};

  ErrorType result = NE, newResult;

  for (int i = errorBlk.getBitN() / codec->getBitN() - 1; i >= 0; i--) {
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());

    if (msg.isZero()) {  // error-free region of a block -> skip
      newResult = NE;
    } else {
      newResult = codec->decode(&msg, &decoded, &correctedPosSet);
    }
    // printf("decode internal msg\n");
    // msg.print();
    // printf("decode internal decoded\n");
    // decoded.print();

    result = worse2ErrorType(result, newResult);
  }

  return result;
}

//------------------------------------------------------------------------------
unsigned long long ECC::getInitialRetiredBlkCount(FaultDomain *fd,
                                                  Fault *fault) {
  double cellFaultRate = fault->getCellFaultRate();
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int blkSize = fd->getChannelWidth() * fd->getBeatHeight();
    double goodBlkProb = pow(1 - cellFaultRate, blkSize);
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChannelWidth() / blkSize;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

unsigned long long ECC::getInitialRetiredBlkCount(FaultDomain *fd,
                                                  double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    int blkSize = fd->getChannelWidth() * fd->getBeatHeight();
    double goodBlkProb = pow(1 - cellFaultRate, blkSize);
    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChannelWidth() / blkSize;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}
