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
 * @file: linear_codec.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC implementation (linear codec)
 */

#ifndef __LINEAR_CODEC_CC__
#define __LINEAR_CODEC_CC__

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "linear_codec.hh"
#include "util.hh"

//--------------------------------------------------------------------
template <int p, int m>
LinearCodec<p, m>::LinearCodec(const char *name, int _symN, int _symR)
    : Codec(name, m * _symN, m * _symR) {
  assert(p == 2);
  symN = _symN;
  symK = _symN - _symR;
  symR = _symR;

  syndrom = new GFElem<p, m>[symR];
  gMatrix = new GFElem<p, m>[symK * symN];
  hMatrix = new GFElem<p, m>[symR * symN];
}

template <int p, int m>
LinearCodec<p, m>::~LinearCodec() {
  delete syndrom;
  delete gMatrix;
  delete hMatrix;
}

template <int p, int m>
void LinearCodec<p, m>::encode(Block *data, ECCWord *encoded) {
  // Step 1:
  // copy encoded message;
  encoded->clone(data);

  GFElem<p, m> *dataElemArr = new GFElem<p, m>[symK];
  for (int i = 0; i < symK; i++) {
    dataElemArr[i].setValue(data->getSymbol(m, i));
  }

  // Step 2:
  // use G matrix to calculate encoded message
  // output = input (1xk) x G (kxn)
  for (int i = symN - 1; i >= 0; i--) {
    GFElem<p, m> buffer;
    for (int j = symK - 1; j >= 0; j--) {
      buffer += dataElemArr[j] * gMatrix[j * symN + i];
    }
    encoded->setSymbol(m, i, buffer->getValue());
  }
}

template <int p, int m>
bool LinearCodec<p, m>::genSyndrome(ECCWord *msg) {
  bool synError = false;

  msg->print();
  GFElem<p, m> *msgElemArr = new GFElem<p, m>[symN];
  printf("AA(%d):", m);
  for (int i = 0; i < symN; i++) {
    msgElemArr[i].setValue(msg->getSymbol(m, i));
    printf("%d ", msg->getSymbol(m, i));
  }
  printf("\n");
  printf("AAA(%d, %d): ", symN, symN - symR);
  for (int i = 0; i < symN; i++) {
    printf("%d ", msgElemArr[i].getIndexValue());
  }
  printf("\n");

  // use H matrix to calculate syndrom
  // output = H (rxn) x input (nx1)
  for (int i = symR - 1; i >= 0; i--) {
    syndrom[i].setValue(0);
    for (int j = symN - 1; j >= 0; j--) {
      syndrom[i] += msgElemArr[j] * hMatrix[i * symN + j];  // hMatrix[i][j]
      printf("@(%d, %d), %d %d %d\n", i, j, syndrom[i].getIndexValue(),
             msgElemArr[j].getIndexValue(),
             hMatrix[i * symN + j].getIndexValue());
    }
    if (!syndrom[i].isZero()) {
      synError = true;
    }
  }
  printf("Syn: ");
  for (int i = symR - 1; i >= 0; i--) {
    printf("%d ", syndrom[i].getIndexValue());
  }
  printf("\n");
  return synError;
}

template <int p, int m>
void LinearCodec<p, m>::print(FILE *fd) {
  fprintf(fd, "G matrix\n");
  for (int i = symK - 1; i >= 0; i--) {
    for (int j = symN - 1; j >= 0; j--) {
      fprintf(fd, "%3d ", gMatrix[i * symN + j].getIndexValue());
    }
    fprintf(fd, "\n");
  }
  fprintf(fd, "H matrix\n");
  for (int i = 0; i < symR; i++) {
    for (int j = 0; j < symN; j++) {
      fprintf(fd, "%3d ", hMatrix[i * symN + j].getIndexValue());
    }
    fprintf(fd, "\n");
  }
}

#endif /* __LINEAR_CODEC_CC__ */
