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
 * @file: linear_codec.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC declaration (linear codec)
 */

#ifndef __LINEAR_CODEC_HH__
#define __LINEAR_CODEC_HH__

#include <stdint.h>
#include "codec.hh"
#include "gf.hh"
#include "message.hh"

/**
 * @brief Genaral linear codec class
 */
template <int p, int m>
class LinearCodec : public Codec {
  // Constructor / destructor
 public:
  LinearCodec(const char *name, int symN, int symR);
  ~LinearCodec();
  // member methods
 public:
  virtual void encode(Block *data, ECCWord *encoded);
  virtual ErrorType decode(ECCWord *msg, ECCWord *decoded) = 0;

 protected:
  bool genSyndrome(ECCWord *msg);
  void print(FILE *fd);
  // member fields
 protected:
  int symN, symK, symR;
  GFElem<p, m> *syndrom;
  // P matrix: rxk
  GFElem<p, m>
      *gMatrix;  // G: k x n matrix (1D representation) / Identity matrix at MSB
  GFElem<p, m>
      *hMatrix;  // H: r x n matrix (1D representation) / Identity matrix at LSB
};

#endif /* __LINEAR_CODEC_HH__ */
