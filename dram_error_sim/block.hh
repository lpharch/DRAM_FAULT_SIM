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
 * @file: block.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * message declaration
 */

#ifndef __BLOCK_HH__
#define __BLOCK_HH__

#include <stdio.h>
#include <string>

#include "util.hh"

/**
 * //----------------------------------------------------------
 * @brief enumeration of ECC layout
 */
typedef enum {
  LINEAR,
  AMD,
  AMD32BL,
  PIN,
  DPIN,
  MULTIX4,
  MULTIX8,
  PIN_2,
  PIN9,
  PIN16,
  ONCHIPx4,
  ONCHIPx4_2,
  ONCHIPx8,
  duoBL9,
  duoBL9full,
  duoBL17,
  duoBL17x8,
  duoBL33,
  duoBL34,
  ONCHIP17x4,
  ONCHIP17x8,
  ONCHIP17x16,
  ONCHIP17x32,
  ONCHIP18x32,
  ONCHIPx4_IECC8_BL17DUO_1,
  ONCHIPx4_IECC8_BL17DUO_2,
  ONCHIPx4_IECC8_BL16_128_Overfetch,
  ONCHIPx4_IECC16_BL16_128_Overfetch,
  ONCHIPx4_IECC8_BL16_256_Overfetch,
  ONCHIPx4_IECC16_BL16_256_Overfetch,
  ONCHIPx4_IECC32_BL16_256_Overfetch,
  USEMESGCONFIG,

} ECCLayout;

/*! \addtogroup Data_Block
 * @brief Classes related to data block
 * @details The general data block is defined in Block. CacheLine is data block
 * fetched from memory and used in ECC and ECCWord is data block used in Codec.
 * Each CacheLine can consist of multiple ECCWord s depending on both ECC and
 * Codec.
 * @{
 *
 * @class Block
 * @brief A general data block.
 * @details CacheLine and ECCWord (codeword) inherit this class for the cases
 * sizes of both are different.
 */
class Block {
  /*! @} */
  // constructor / destructor
 public:
  Block(int bitSize);
  ~Block();

  // member methods
 public:
  int getBitN() { return bitN; }
  void reset();
  //! Check block has no error
  bool isZero(int redundancy=0);
  bool isZero(int from, int to);
  void clone(Block *src);
  bool equal(Block *ref);
  int getSymbol(int size, int pos);
  void setSymbol(int size, int pos, int value);
  void invSymbol(int size, int pos, int value);
  bool getBit(int pos) { return bitArr[pos]; }
  void setBit(int pos, bool value) { bitArr[pos] = value; }
  void invBit(int pos) { bitArr[pos] ^= true; }
  void print() const;
  void copy(const Block *src) {
    for (int i = 0; i < bitN; i++) {
      bitArr[i] = src->bitArr[i];
    }
    errorDQ = src->errorDQ;
  }
  void copyfromN(const Block *src,int N) {
    for (int i = 0; i < bitN; i++) {
      bitArr[i] = src->bitArr[i+N];
    }
    errorDQ = src->errorDQ;
  }
    void copyfromN_bringM(const Block *src,int N,int M) {
    for (int i = 0; i < M; i++) {
      bitArr[i] = src->bitArr[i+N];
    }
    errorDQ = src->errorDQ;
  }
  void copyfromN_strideM(const Block *src,int N,int M) {
    for (int i = 0; i < bitN; i++) {
      bitArr[i] = src->bitArr[i*M+N];
    }
    errorDQ = src->errorDQ;
  }
  Block &operator^=(const Block &rhs) {
    for (int i = 0; i < bitN; i++) {
      bitArr[i] ^= rhs.bitArr[i];
    }
    return *this;
  }

  friend class ECCWord;
  friend class CacheLine;

 protected:
  int bitN;

 public:
  //! Boolean array of size bitN to store block data
  bool *bitArr;
  int errorDQ;
};

#endif /* __BLOCK_HH__ */
