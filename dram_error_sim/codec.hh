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
 * @file: codec.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * CODEC declaration
 */

#ifndef __CODEC_HH__
#define __CODEC_HH__

#include <stdint.h>
#include <string.h>
#include <list>
#include <set>
#include <unordered_map>
#include "message.hh"

// NE: no error
// CE: detected and corrected error
// DUE: detected but uncorrected error
// ME: detected but miscorrected error
// UE: undetected error
// SDC: ME + UE
#include "common.hh"

typedef uint64_t ErrorInfo;
// typedef std::unordered_map<int, int> ErrorInfo; // position, value

/**@addtogroup Error_Correction
 * @{
 *
 * @class Codec
 * @brief General codec class.
 * @details This class defines a general error correcting code itself. Encoding,
 * decoding, and syndrome generation are defined in a general way.
 * RS codes and BCH codes are the examples of specific error correcting codes
 * that inherit this class.
 * @details ECC class defines how to use these codec. For example, a general
 * SEC-DED scheme (SECDED72b) that handle data at cacheline granularity (e.g.,
 * 64B) invokes decode of Hsiao (72,64) 8 times in a row while a general
 * chipkill scheme (AMDChipkill72b) invokes decode of RS(18,16) 4 times.
 */
class Codec {
  /**@} */
  //! Constructor
 public:
  Codec(const char *name, int _bitN, int _bitR)
      : bitN(_bitN), bitR(_bitR), bitK(_bitN - _bitR) {
    strcpy(this->name, name);
  }
  // member methods
 public:
  const char *getName() { return name; }
  inline int getBitN() { return bitN; }
  inline int getBitR() { return bitR; }
  inline int getBitK() { return bitK; }
  //! Encode
  /*! \param data original data block
           \param encoded data block
  */
  virtual void encode(Block *data, ECCWord *encoded) = 0;
  //! Decode
  /*! \param msg original (received) data block that might have errors in it
           \param decoded decoded data block
           \correctedPos corrected (pin or symbol) positions
           */
  virtual ErrorType decode(ECCWord *msg, ECCWord *decoded,
                           std::set<int> *correctedPos = NULL) = 0;

  virtual int getChipID(int pos) const { return pos; }
  void resetHistory() {
    correctedChips.clear();
    correctedPins.clear();
  }
  void insertCorrectionInfo(int symID, int value) {
    correctedChips.insert(getChipID(symID));
    correctedPins.insert(symID);
  }
  virtual bool miscorrectSymDetect() const { return false; }
  //! Print Syndromes
  virtual void printSyn() {}
  //! Print H matrix
  virtual void printHmatrix() {}
  //! Generate Syndromes
  //! \param msg Received data block to decode
  virtual bool genSyndrome(ECCWord *msg) {}
  std::list<int> correctedCount;
  // member fields
 private:
  char name[256];

 protected:
  int bitN;  //! number of bits in a codeword
  int bitR;  //! number of redundant (check) bits
  int bitK;  //! number of data bits

  std::set<int> correctedChips;  //! correction information (chip locations)
  std::set<int> correctedPins;   //! correction information (pin locations)
};

#endif /* __CODEC_HH__ */
