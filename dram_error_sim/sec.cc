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

#include "sec.hh"

SEC::SEC(const char *name, int _bitN, int _bitR)
    : BinaryLinearCodec(name, _bitN, _bitR) {
  int enc_idx[bitN];
  int cnt_p = 0;  // count of parity bits
  int cnt_d = 0;  // count of data bits
  int pow_p = 1;

  // encoding index
  for (int i = 0; i < bitN; i++) {
    if (i + 1 == pow_p) {
      enc_idx[cnt_p + bitK] = i;
      pow_p *= 2;
      cnt_p++;
    } else {
      enc_idx[cnt_d] = i;
      cnt_d++;
    }
  }
  // G matrix calculation
  int pow = 2;
  for (int i = 0; i < bitR; i++) {
    for (int j = 0; j < bitK; j++) {
      if ((enc_idx[j] + 1) % pow >= pow / 2) {
        gMatrix[enc_idx[i + bitK] * bitK + j] = 1;
      } else {
        gMatrix[enc_idx[i + bitK] * bitK + j] = 0;
      }
    }
    pow *= 2;
  }
  for (int i = 0; i < bitK; i++) {
    for (int j = 0; j < bitK; j++) {
      if (i == j)
        gMatrix[enc_idx[i] * bitK + j] = 1;
      else
        gMatrix[enc_idx[i] * bitK + j] = 0;
    }
  }
  // H matrix calculation
  for (int i = 0; i < bitR; i++) {
    pow_p = 1;
    int cnt_d_ = 0;
    for (int j = 0; j < bitN; j++) {
      if (j + 1 == pow_p) {
        if (enc_idx[i + bitK] == j)
          hMatrix[i * bitN + j] = 1;
        else
          hMatrix[i * bitN + j] = 0;
        pow_p *= 2;
      } else {
        hMatrix[i * bitN + j] = gMatrix[enc_idx[i + bitK] * bitK + cnt_d_];
        cnt_d_++;
      }
    }
  }
}

ErrorType SEC::decode(ECCWord *msg, ECCWord *decoded,
                      std::set<int> *correctedPos) {
  decoded->clone(msg);
  if (msg->isZero()) return NE;
  bool SynError = genSyndrome(msg);

  if (SynError) {
    int pos = 0;
    for (int i = bitR - 1; i >= 0; i--) {
      if (syndrom[i]) pos += 1;
      if (i != 0) pos = pos << 1;
    }
    if (pos == 0 || pos > bitN){
      return DUE;  //{printf("pos: %i", pos);exit(0);}
    }
    decoded->invBit(pos - 1);
    // if (correctedPos!=NULL) {
    //    correctedPos->insert(pos-1);
    // }
    if (decoded->isZero())
      return CE;
    else
      return SDC;
  } else {
    return SDC;
  }
}


