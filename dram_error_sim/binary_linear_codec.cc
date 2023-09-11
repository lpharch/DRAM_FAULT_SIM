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
 * @file: binary_linear_codec.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC implementation (binary linear codec)
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "binary_linear_codec.hh"

//--------------------------------------------------------------------
BinaryLinearCodec::BinaryLinearCodec(const char *name, int _bitN, int _bitR)
    : Codec(name, _bitN, _bitR) {
  syndrom = new uint8_t[bitR];
  gMatrix = new uint8_t[bitK * bitN];
  hMatrix = new uint8_t[bitR * bitN];
}

BinaryLinearCodec::~BinaryLinearCodec() {
  delete[] syndrom;
  delete[] gMatrix;
  delete[] hMatrix;
}

void BinaryLinearCodec::encode(Block *data, ECCWord *encoded) {
  // Step 1:
  // reset encoded message;
  encoded->reset();

  // Step 2:
  // use G matrix to calculate encoded message
  // output = input (1xk) x G (kxn)
  for (int i = bitN - 1; i >= 0; i--) {
    int buffer = 0;
    for (int j = bitK - 1; j >= 0; j--) {
      buffer ^= data->getBit(j) & gMatrix[j * bitN + i];
    }
    encoded->setBit(i, buffer);
  }
}

bool BinaryLinearCodec::genSyndrome(ECCWord *msg) {
  bool synError = false;
  // use H matrix to calculate syndrom
  // output = H (rxn) x input (nx1)
  for (int i = bitR - 1; i >= 0; i--) {
    syndrom[i] = 0;
    for (int j = bitN - 1; j >= 0; j--) {
      syndrom[i] ^= (msg->getBit(j) & hMatrix[i * bitN + j]);  // hMatrix[i][j]
    }
    if (syndrom[i]) {
      synError = true;
    }
  }
  return synError;
}

void BinaryLinearCodec::print(FILE *fd) {
  fprintf(fd, "G matrix\n");
  for (int i = bitK - 1; i >= 0; i--) {
    for (int j = bitN - 1; j >= 0; j--) {
      fprintf(fd, "%d ", gMatrix[i * bitN + j]);
    }
    fprintf(fd, "\n");
  }
  fprintf(fd, "H matrix\n");
  for (int i = bitR - 1; i >= 0; i--) {
    for (int j = bitN - 1; j >= 0; j--) {
      fprintf(fd, "%d ", hMatrix[i * bitN + j]);
    }
    fprintf(fd, "\n");
  }
}
