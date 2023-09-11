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
 * @file: message.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * message implementation
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <random>
#include <sstream> 
#include <iostream>
#include <fstream>
#include <iomanip>


#include "uint128_t.h"
#include "message.hh"

//#include "DRAM.hh"

//----------------------------------------------------------
//
//----------------------------------------------------------
Block::Block(int bitSize) {
  bitN = bitSize;

  bitArr = new bool[bitN+4];
  errorDQ = 0;
  reset();
}

Block::~Block() { delete[] bitArr; }

//----------------------------------------------------------
void Block::reset() { memset(bitArr, 0, sizeof(bool) * bitN); }

//----------------------------------------------------------
bool Block::isZero(int redundancy) {
  for (int i = 0; i < bitN-redundancy; i++) {
    if (bitArr[i]) return false;
  }
  return true;
}

bool Block::isZero(int from, int to) {
  for (int i = from; i < to; i++) {
    if (bitArr[i]) return false;
  }
  return true;
}


//----------------------------------------------------------
void Block::clone(Block* src) {
  assert(bitN == src->bitN);
  memcpy(bitArr, src->bitArr, sizeof(bool) * bitN);
}

//----------------------------------------------------------
bool Block::equal(Block* ref) {
  if (bitN != ref->bitN) {
    return false;
  }
  for (int i = 0; i < bitN; i++) {
    if (bitArr[i] != ref->bitArr[i]) {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------
int Block::getSymbol(int size, int pos) {
  int result = 0;
  for (int i = size - 1; i >= 0; i--) {
    result = (result << 1) | bitArr[i + pos * size];
  }
  return result;
}

//----------------------------------------------------------
void Block::setSymbol(int size, int pos, int value) {
  for (int i = size - 1; i >= 0; i--) {
    bitArr[i + pos * size] = (value >> i) & 1;
  }
}

//----------------------------------------------------------
void Block::invSymbol(int size, int pos, int value) {
  for (int i = size - 1; i >= 0; i--) {
    bitArr[i + pos * size] ^= (value >> i) & 1;
  }
}

//----------------------------------------------------------
void Block::print() const {
  uint128_t buffer;
  std::stringstream hex; 

  hex.str("");
  buffer = 0;
  for (int i = bitN - 1; i >= 0; i--) {
    buffer = (buffer << 1) | bitArr[i];
    if (i % 4 == 0) {
      hex << std::hex << buffer;
      std::cout<<std::setfill('0') << std::setw(1) <<hex.str();

      //fprintf(fd, "%01X", buffer);
      buffer = 0;
      hex.str("");
    }
    if ((i % 16 == 0) && (i != 0)) {
      //fprintf(fd, "_");
      std::cout<<"_";
    }
    if ((i % 64 == 0) && (i != 0)) {
      std::cout<<" ";
      //fprintf(fd, " ");
    }
  }
  //fprintf(fd, " (%03d)\n", bitN);
  std::cout<<"(" << std::setfill('0') << std::setw(3) <<bitN <<")\n";

}

//----------------------------------------------------------
//
//----------------------------------------------------------
void ECCWord::extract(Block* data, ECCLayout layout, int pos,
                      int channelWidth, MSGConfig message_config) {
  if (layout == LINEAR) {
    memcpy(bitArr, &(data->bitArr[channelWidth * pos]),
           sizeof(bool) * channelWidth);
  } else if (layout == PIN) {
    // original layout (bits)
    // 0  1  2  3  4  5  6  7  - 8  9  10 11 12 13 14 15 - ... - 64 65 66 67 68
    // 69 70 71
    // 72 73 74 75 76 77 78 79 - 80 81 82 83 84 85 86 87 - ... -
    for (int i = 0; i < channelWidth; i++) {
      bitArr[i * 8 + 0] = data->bitArr[channelWidth * 0 + i];
      bitArr[i * 8 + 1] = data->bitArr[channelWidth * 1 + i];
      bitArr[i * 8 + 2] = data->bitArr[channelWidth * 2 + i];
      bitArr[i * 8 + 3] = data->bitArr[channelWidth * 3 + i];
      bitArr[i * 8 + 4] = data->bitArr[channelWidth * 4 + i];
      bitArr[i * 8 + 5] = data->bitArr[channelWidth * 5 + i];
      bitArr[i * 8 + 6] = data->bitArr[channelWidth * 6 + i];
      bitArr[i * 8 + 7] = data->bitArr[channelWidth * 7 + i];
    }
  } else if (layout == PIN_2) {
    // for a shorter codeword than cacheline
    for (int i = 0; i < channelWidth - 2; i++) {
      bitArr[i * 8 + 0] = data->bitArr[channelWidth * 0 + i];
      bitArr[i * 8 + 1] = data->bitArr[channelWidth * 1 + i];
      bitArr[i * 8 + 2] = data->bitArr[channelWidth * 2 + i];
      bitArr[i * 8 + 3] = data->bitArr[channelWidth * 3 + i];
      bitArr[i * 8 + 4] = data->bitArr[channelWidth * 4 + i];
      bitArr[i * 8 + 5] = data->bitArr[channelWidth * 5 + i];
      bitArr[i * 8 + 6] = data->bitArr[channelWidth * 6 + i];
      bitArr[i * 8 + 7] = data->bitArr[channelWidth * 7 + i];
    }
  } else if (layout == PIN9) {
    for (int i = 0; i < channelWidth; i++) {
      bitArr[i * 9 + 0] = data->bitArr[channelWidth * 0 + i];
      bitArr[i * 9 + 1] = data->bitArr[channelWidth * 1 + i];
      bitArr[i * 9 + 2] = data->bitArr[channelWidth * 2 + i];
      bitArr[i * 9 + 3] = data->bitArr[channelWidth * 3 + i];
      bitArr[i * 9 + 4] = data->bitArr[channelWidth * 4 + i];
      bitArr[i * 9 + 5] = data->bitArr[channelWidth * 5 + i];
      bitArr[i * 9 + 6] = data->bitArr[channelWidth * 6 + i];
      bitArr[i * 9 + 7] = data->bitArr[channelWidth * 7 + i];
      bitArr[i * 9 + 8] = data->bitArr[channelWidth * 8 + i];
      // printf("%x %d %d %d %d %d %d %d %d %d (%d)\n", getSymbol(9, i),
      //                data->bitArr[channelWidth*0+i],
      //                data->bitArr[channelWidth*1+i],
      //                data->bitArr[channelWidth*2+i],
      //                data->bitArr[channelWidth*3+i],
      //                data->bitArr[channelWidth*4+i],
      //                data->bitArr[channelWidth*5+i],
      //                data->bitArr[channelWidth*6+i],
      //                data->bitArr[channelWidth*7+i],
      //                data->bitArr[channelWidth*8+i],
      //                channelWidth);
    }
  }else if (layout == duoBL34){
    // assuming burst length is 34
    int BurstLength = 34;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 18
      for (int j = 0; j < BurstLength - 2; j++) {         // 8 x4
        bitArr[((BurstLength - 2) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 2) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 2) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 2) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    int offset = channelWidth * (BurstLength - 2);
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      //if (offset + i * 4 + 0 >=608) break;
      bitArr[offset + i * 8 + 0] = 
          data->bitArr[channelWidth * (BurstLength - 2) + chipWidth * i + 0];
      bitArr[offset + i * 8 + 1] =
          data->bitArr[channelWidth * (BurstLength - 2) + chipWidth * i + 1];
      bitArr[offset + i * 8 + 2] =
          data->bitArr[channelWidth * (BurstLength - 2) + chipWidth * i + 2];
      bitArr[offset + i * 8 + 3] =
          data->bitArr[channelWidth * (BurstLength - 2) + chipWidth * i + 3];
      bitArr[offset + i * 8 + 4] = 
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 8 + 5] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
      bitArr[offset + i * 8 + 6] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 2];
      bitArr[offset + i * 8 + 7] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 3];
    }
  } else if (layout == duoBL33){
    // assuming burst length is 33
    int BurstLength = 33;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 5
      for (int j = 0; j < BurstLength - 1; j++) {         // 32 x4
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    int offset = channelWidth * (BurstLength - 1);
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      //if (offset + i * 4 + 0 >=608) break;
      bitArr[offset + i * 4 + 0] = 
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 4 + 1] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
      bitArr[offset + i * 4 + 2] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 2];
      bitArr[offset + i * 4 + 3] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 3];
      }
    } else if (layout == duoBL9) {
    // assuming burst length is 9
    int BurstLength = 9;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 18
      for (int j = 0; j < BurstLength - 1; j++) {         // 8 x4
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    int offset = channelWidth * (BurstLength - 1);
    for (int i = 0; i < 16; i++) {
      bitArr[offset + i * 2 + 0] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 2 + 1] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
    }
  } else if (layout == duoBL9full) {  // 12.5% on-chip redundancy
    // assuming burst length is 9
    int BurstLength = 9;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 18
      for (int j = 0; j < BurstLength - 1; j++) {         // 8 x4
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    int offset = channelWidth * (BurstLength - 1);
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      bitArr[offset + i * 4 + 0] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 4 + 1] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
      bitArr[offset + i * 4 + 2] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 2];
      bitArr[offset + i * 4 + 3] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 3];
    }
  } else if (layout == duoBL17 | layout == ONCHIPx4_IECC8_BL17DUO_2) {
    // assuming burst length is 17
    int BurstLength = 17;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 18
      for (int j = 0; j < BurstLength - 1; j++) {         // 8 x4
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    int offset = channelWidth * (BurstLength - 1);
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      //if (offset + i * 4 + 0 >=608) break;
      bitArr[offset + i * 4 + 0] = 
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 4 + 1] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
      bitArr[offset + i * 4 + 2] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 2];
      bitArr[offset + i * 4 + 3] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 3];
    }
  } else if (layout == duoBL17x8) {
    // assuming burst length is 17
    int BurstLength = 17;
    int symSize = 8;
    int chipWidth = 8;
    for (int i = 0; i < channelWidth / chipWidth; i++) {  // 18
      for (int j = 0; j < BurstLength - 1; j++) {         // 8 x4
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 4] =
            data->bitArr[channelWidth * j + chipWidth * i + 4];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 5] =
            data->bitArr[channelWidth * j + chipWidth * i + 5];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 6] =
            data->bitArr[channelWidth * j + chipWidth * i + 6];
        bitArr[((BurstLength - 1) * i + j) * chipWidth + 7] =
            data->bitArr[channelWidth * j + chipWidth * i + 7];
      }
    }
    int offset = channelWidth * (BurstLength - 1);
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      bitArr[offset + i * 8 + 0] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 0];
      bitArr[offset + i * 8 + 1] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 1];
      bitArr[offset + i * 8 + 2] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 2];
      bitArr[offset + i * 8 + 3] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 3];
      //if (offset + i * 8 + 4 >=608) break;
      bitArr[offset + i * 8 + 4] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 4];
      bitArr[offset + i * 8 + 5] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 5];
      bitArr[offset + i * 8 + 6] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 6];
      bitArr[offset + i * 8 + 7] =
          data->bitArr[channelWidth * (BurstLength - 1) + chipWidth * i + 7];
    }
  } else if (layout == PIN16) {
    // assuming burst length is 16
    int BurstLength = 16;
    int symSize = 8;
    int chipWidth = 4;
    for (int i = 0; i < channelWidth / chipWidth; i++) {
      for (int j = 0; j < BurstLength; j++) {
        bitArr[(BurstLength * i + j) * chipWidth + 0] =
            data->bitArr[channelWidth * j + chipWidth * i + 0];
        bitArr[(BurstLength * i + j) * chipWidth + 1] =
            data->bitArr[channelWidth * j + chipWidth * i + 1];
        bitArr[(BurstLength * i + j) * chipWidth + 2] =
            data->bitArr[channelWidth * j + chipWidth * i + 2];
        bitArr[(BurstLength * i + j) * chipWidth + 3] =
            data->bitArr[channelWidth * j + chipWidth * i + 3];
      }
    }
    //    } else if (layout==PIN17) {
    //		// assuming burst length is 17
    //		int BurstLength = 17;
    //		int symSize = 8;
    //		int chipWidth = 4;
    //		for(int i=0; i< channelWidth/chipWidth; i++){
    //			for(int j=0; j< BurstLength; j++){
    //				bitArr[(BurstLength*i + j)*chipWidth + 0] =
    // data->bitArr[channelWidth * j + chipWidth * i + 0];
    //				bitArr[(BurstLength*i + j)*chipWidth + 1] =
    // data->bitArr[channelWidth * j + chipWidth * i + 1];
    //				bitArr[(BurstLength*i + j)*chipWidth + 2] =
    // data->bitArr[channelWidth * j + chipWidth * i + 2];
    //				bitArr[(BurstLength*i + j)*chipWidth + 3] =
    // data->bitArr[channelWidth * j + chipWidth * i + 3];
    //			}
    //		}
  } else if (layout == DPIN) {
    // original layout (2-bits)
    // 0  1  2  3  - 4  5  6  7  - ... - 32 33 34 35
    // 36 37 38 39 - 40 41 42 43 - ... - 68 69 70 71
    for (int i = 0; i < channelWidth / 2; i++) {
      bitArr[i * 8 + 0] = data->bitArr[channelWidth * 0 + i * 2 + 0];
      bitArr[i * 8 + 1] = data->bitArr[channelWidth * 0 + i * 2 + 1];
      bitArr[i * 8 + 2] = data->bitArr[channelWidth * 1 + i * 2 + 0];
      bitArr[i * 8 + 3] = data->bitArr[channelWidth * 1 + i * 2 + 1];
      bitArr[i * 8 + 4] = data->bitArr[channelWidth * 2 + i * 2 + 0];
      bitArr[i * 8 + 5] = data->bitArr[channelWidth * 2 + i * 2 + 1];
      bitArr[i * 8 + 6] = data->bitArr[channelWidth * 3 + i * 2 + 0];
      bitArr[i * 8 + 7] = data->bitArr[channelWidth * 3 + i * 2 + 1];
    }
  } else if (layout == AMD) {
    for (int i = 0; i < channelWidth / 4; i++) {
      bitArr[i * 8 + 0] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 4 + 0];
      bitArr[i * 8 + 1] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 4 + 1];
      bitArr[i * 8 + 2] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 4 + 2];
      bitArr[i * 8 + 3] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 4 + 3];
      bitArr[i * 8 + 4] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 4 + 0];
      bitArr[i * 8 + 5] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 4 + 1];
      bitArr[i * 8 + 6] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 4 + 2];
      bitArr[i * 8 + 7] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 4 + 3];
    }
  } else if (layout == AMD32BL) {
    for (int i = 0; i < channelWidth / 4; i++) {
      for(int k =0;k<8;k++){
        bitArr[i * 32 + 0 +4*k] =
            data->bitArr[channelWidth * (pos * 8 + k) + i * 4 + 0];
        bitArr[i * 32 + 1 +4*k] =
            data->bitArr[channelWidth * (pos * 8 + k) + i * 4 + 1];
        bitArr[i * 32 + 2 +4*k] =
            data->bitArr[channelWidth * (pos * 8 + k) + i * 4 + 2];
        bitArr[i * 32 + 3 +4*k] =
            data->bitArr[channelWidth * (pos * 8 + k) + i * 4 + 3];
      }
    }

      /*
      bitArr[i * 8 + 0] =
          data->bitArr[channelWidth * (pos * 8 + 0) + i * 4 + 0];
      bitArr[i * 8 + 1] =
          data->bitArr[channelWidth * (pos * 8 + 0) + i * 4 + 1];
      bitArr[i * 8 + 2] =
          data->bitArr[channelWidth * (pos * 8 + 0) + i * 4 + 2];
      bitArr[i * 8 + 3] =
          data->bitArr[channelWidth * (pos * 8 + 0) + i * 4 + 3];
      bitArr[i * 8 + 4] =
          data->bitArr[channelWidth * (pos * 8 + 1) + i * 4 + 0];
      bitArr[i * 8 + 5] =
          data->bitArr[channelWidth * (pos * 8 + 1) + i * 4 + 1];
      bitArr[i * 8 + 6] =
          data->bitArr[channelWidth * (pos * 8 + 1) + i * 4 + 2];
      bitArr[i * 8 + 7] =
          data->bitArr[channelWidth * (pos * 8 + 1) + i * 4 + 3];
      */
  } else if (layout == MULTIX8) {
    for (int i = 0; i < channelWidth / 8; i++) {
      bitArr[i * 16 + 0] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 0];
      bitArr[i * 16 + 1] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 1];
      bitArr[i * 16 + 2] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 2];
      bitArr[i * 16 + 3] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 3];
      bitArr[i * 16 + 4] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 4];
      bitArr[i * 16 + 5] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 5];
      bitArr[i * 16 + 6] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 6];
      bitArr[i * 16 + 7] =
          data->bitArr[channelWidth * (pos * 2 + 0) + i * 8 + 7];
      bitArr[i * 16 + 8] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 0];
      bitArr[i * 16 + 9] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 1];
      bitArr[i * 16 + 10] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 2];
      bitArr[i * 16 + 11] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 3];
      bitArr[i * 16 + 12] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 4];
      bitArr[i * 16 + 13] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 5];
      bitArr[i * 16 + 14] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 6];
      bitArr[i * 16 + 15] =
          data->bitArr[channelWidth * (pos * 2 + 1) + i * 8 + 7];
    }
  } else if (layout == MULTIX4) {
    for (int i = 0; i < channelWidth / 4; i++) {
      bitArr[i * 16 + 0] =
          data->bitArr[channelWidth * (pos * 4 + 0) + i * 4 + 0];
      bitArr[i * 16 + 1] =
          data->bitArr[channelWidth * (pos * 4 + 0) + i * 4 + 1];
      bitArr[i * 16 + 2] =
          data->bitArr[channelWidth * (pos * 4 + 0) + i * 4 + 2];
      bitArr[i * 16 + 3] =
          data->bitArr[channelWidth * (pos * 4 + 0) + i * 4 + 3];
      bitArr[i * 16 + 4] =
          data->bitArr[channelWidth * (pos * 4 + 1) + i * 4 + 0];
      bitArr[i * 16 + 5] =
          data->bitArr[channelWidth * (pos * 4 + 1) + i * 4 + 1];
      bitArr[i * 16 + 6] =
          data->bitArr[channelWidth * (pos * 4 + 1) + i * 4 + 2];
      bitArr[i * 16 + 7] =
          data->bitArr[channelWidth * (pos * 4 + 1) + i * 4 + 3];
      bitArr[i * 16 + 8] =
          data->bitArr[channelWidth * (pos * 4 + 2) + i * 4 + 0];
      bitArr[i * 16 + 9] =
          data->bitArr[channelWidth * (pos * 4 + 2) + i * 4 + 1];
      bitArr[i * 16 + 10] =
          data->bitArr[channelWidth * (pos * 4 + 2) + i * 4 + 2];
      bitArr[i * 16 + 11] =
          data->bitArr[channelWidth * (pos * 4 + 2) + i * 4 + 3];
      bitArr[i * 16 + 12] =
          data->bitArr[channelWidth * (pos * 4 + 3) + i * 4 + 0];
      bitArr[i * 16 + 13] =
          data->bitArr[channelWidth * (pos * 4 + 3) + i * 4 + 1];
      bitArr[i * 16 + 14] =
          data->bitArr[channelWidth * (pos * 4 + 3) + i * 4 + 2];
      bitArr[i * 16 + 15] =
          data->bitArr[channelWidth * (pos * 4 + 3) + i * 4 + 3];
    }
  } else if (layout == ONCHIPx4) {
    for (int i = 0; i < 18; i++) {  // height
      bitArr[i * 4 + 0] = data->bitArr[channelWidth * i + pos * 4 + 0];
      bitArr[i * 4 + 1] = data->bitArr[channelWidth * i + pos * 4 + 1];
      bitArr[i * 4 + 2] = data->bitArr[channelWidth * i + pos * 4 + 2];
      bitArr[i * 4 + 3] = data->bitArr[channelWidth * i + pos * 4 + 3];
    }
  } else if (layout == ONCHIPx4_2) {
    for (int i = 0; i < 17; i++) {  // height
      bitArr[i * 4 + 0] = data->bitArr[channelWidth * i + pos * 4 + 0];
      bitArr[i * 4 + 1] = data->bitArr[channelWidth * i + pos * 4 + 1];
      bitArr[i * 4 + 2] = data->bitArr[channelWidth * i + pos * 4 + 2];
      bitArr[i * 4 + 3] = data->bitArr[channelWidth * i + pos * 4 + 3];
    }
    /*
    for (int i=0; i<9; i++) {
        bitArr[i*8+0] = data->bitArr[channelWidth*0+pos*9+i];
        bitArr[i*8+1] = data->bitArr[channelWidth*1+pos*9+i];
        bitArr[i*8+2] = data->bitArr[channelWidth*2+pos*9+i];
        bitArr[i*8+3] = data->bitArr[channelWidth*3+pos*9+i];
        bitArr[i*8+4] = data->bitArr[channelWidth*4+pos*9+i];
        bitArr[i*8+5] = data->bitArr[channelWidth*5+pos*9+i];
        bitArr[i*8+6] = data->bitArr[channelWidth*6+pos*9+i];
        bitArr[i*8+7] = data->bitArr[channelWidth*7+pos*9+i];
    }
    */
    // for (int i=0; i<9; i++) {   // height
    //    bitArr[i*8+0] = data->bitArr[channelWidth*(i+0)+pos*4];
    //    bitArr[i*8+1] = data->bitArr[channelWidth*(i+1)+pos*4];
    //    bitArr[i*8+2] = data->bitArr[channelWidth*(i+2)+pos*4];
    //    bitArr[i*8+3] = data->bitArr[channelWidth*(i+3)+pos*4];
    //    bitArr[i*8+4] = data->bitArr[channelWidth*(i+4)+pos*4];
    //    bitArr[i*8+5] = data->bitArr[channelWidth*(i+5)+pos*4];
    //    bitArr[i*8+6] = data->bitArr[channelWidth*(i+6)+pos*4];
    //    bitArr[i*8+7] = data->bitArr[channelWidth*(i+7)+pos*4];
    //}
  } else if (layout == ONCHIPx8) {
    for (int i = 0; i < 9; i++) {  // height
      bitArr[i * 8 + 0] = data->bitArr[channelWidth * i + pos * 8 + 0];
      bitArr[i * 8 + 1] = data->bitArr[channelWidth * i + pos * 8 + 1];
      bitArr[i * 8 + 2] = data->bitArr[channelWidth * i + pos * 8 + 2];
      bitArr[i * 8 + 3] = data->bitArr[channelWidth * i + pos * 8 + 3];
      bitArr[i * 8 + 4] = data->bitArr[channelWidth * i + pos * 8 + 4];
      bitArr[i * 8 + 5] = data->bitArr[channelWidth * i + pos * 8 + 5];
      bitArr[i * 8 + 6] = data->bitArr[channelWidth * i + pos * 8 + 6];
      bitArr[i * 8 + 7] = data->bitArr[channelWidth * i + pos * 8 + 7];
    }
  } else if (layout == ONCHIP17x8) {
    for (int i = 0; i < 17; i++) {  // height
      bitArr[i * 8 + 0] = data->bitArr[channelWidth * i + pos * 8 + 0];
      bitArr[i * 8 + 1] = data->bitArr[channelWidth * i + pos * 8 + 1];
      bitArr[i * 8 + 2] = data->bitArr[channelWidth * i + pos * 8 + 2];
      bitArr[i * 8 + 3] = data->bitArr[channelWidth * i + pos * 8 + 3];
      bitArr[i * 8 + 4] = data->bitArr[channelWidth * i + pos * 8 + 4];
      bitArr[i * 8 + 5] = data->bitArr[channelWidth * i + pos * 8 + 5];
      bitArr[i * 8 + 6] = data->bitArr[channelWidth * i + pos * 8 + 6];
      bitArr[i * 8 + 7] = data->bitArr[channelWidth * i + pos * 8 + 7];
    }
  } else if (layout == ONCHIP17x4) {
    for (int i = 0; i < 17; i++) {  // height
      bitArr[i * 4 + 0] = data->bitArr[channelWidth * i + pos * 4 + 0];
      bitArr[i * 4 + 1] = data->bitArr[channelWidth * i + pos * 4 + 1];
      bitArr[i * 4 + 2] = data->bitArr[channelWidth * i + pos * 4 + 2];
      bitArr[i * 4 + 3] = data->bitArr[channelWidth * i + pos * 4 + 3];
    }
  }else if (layout == ONCHIP17x16) {
    for (int i = 0; i < 17; i++) {  // height
      for (int j = 0; j < 16; j++) {
        bitArr[i * 16 + j] = data->bitArr[channelWidth * i + pos * 16 + j];
      }
    }
  } else if (layout == ONCHIP17x32) {
    for (int i = 0; i < 17; i++) {  // height
      for (int j = 0; j < 32; j++) {
        bitArr[i * 32 + j] = data->bitArr[channelWidth * i + pos * 32 + j];
      }
    }
  } else if (layout == ONCHIP18x32) {
    for (int i = 0; i < 18; i++) {  // height
      for (int j = 0; j < 32; j++) {
        bitArr[i * 32 + j] = data->bitArr[channelWidth * i + pos * 32 + j];
      }
    }
  } else if (layout == ONCHIPx4_IECC8_BL17DUO_1){
    // For IECC of 8 bit and BL17 DUO, there are 19 Burst length.
    // Last two BL is for your IECC and 17th one is for DUO.
    // 1 represents IECC message format and 2 for DUO message format.
    for (int i = 0; i < 16; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[i * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
    for (int i = 17; i < 19; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[(i+15) * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
  } else if (layout == ONCHIPx4_IECC8_BL16_128_Overfetch){
    // For IECC of 8 bit and BL16 Bamboo, there are 18 Burst length.
    // Last 2 BL is for your IECC.
    // 1 represents IECC message format and 2 for DUO message format.
    for (int i = 0; i < 16*2; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[i * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
    for (int i = 16*2; i < 16*2+2; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[(i) * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
  } else if (layout == ONCHIPx4_IECC16_BL16_128_Overfetch){
    // For IECC of 16 bit and BL16 Bamboo, there are 20 Burst length.
    // Last 4 BL is for your IECC.
    // 1 represents IECC message format and 2 for DUO message format.
    for (int i = 0; i < 16*2; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[i * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
    for (int i = 16*2; i < 16*2+4; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[(i) * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
  } else if (layout == ONCHIPx4_IECC8_BL16_256_Overfetch){
    // For IECC of 8 bit and BL16 Bamboo, there are 18 Burst length.
    // Last 2 BL is for your IECC.
    // 1 represents IECC message format and 2 for DUO message format.
    for (int i = 0; i < 16*4; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[i * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
    for (int i = 16*4; i < 16*4+2; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[(i) * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
  } else if (layout == ONCHIPx4_IECC16_BL16_256_Overfetch){
    // For IECC of 16 bit and BL16 Bamboo, there are 20 Burst length.
    // Last 4 BL is for your IECC.
    // 1 represents IECC message format and 2 for DUO message format.
    for (int i = 0; i < 16*4; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[i * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
    for (int i = 16*4; i < 16*4+4; i++) {  // height
      for (int j = 0; j < 4; j++) {
        bitArr[(i) * 4 + j] = data->bitArr[channelWidth * i + pos * 4 + j];
      }
    }
  }
  else if (layout == ONCHIPx4_IECC32_BL16_256_Overfetch){
    // For IECC of 16 bit and BL16 Bamboo, there are 20 Burst length.
    // Last 4 BL is for your IECC.
    // 1 represents IECC message format and 2 for DUO message format.
    int height = message_config.get_height_base();
    int BaseBL = message_config.get_BaseBL();
    int extraBeat = message_config.get_extraBeat();
    int msgWidth = message_config.get_chipwidth();
    int chipnumber = message_config.get_chipnumber();
    int extraheight = message_config.get_extraheight();
    int overfetch_mult = message_config.get_overfetch_mult();
    RedundancyMode extramode = message_config.get_extramode();

    for (int k=0;k<BaseBL*overfetch_mult/height;k++){
      for (int i = 0; i < msgWidth; i++) {  
        for (int j = 0; j < height; j++) { // height
          // continuous 16 bits in bamboo style will be 2 symbols.
          bitArr[msgWidth*height*k  + i * height + j] = 
            data->bitArr[channelWidth * j + pos * msgWidth + i + channelWidth * height * k ];
        }
      }
    }
    if (extraBeat =! 0 and extraheight != 0){
      int offest_redundancy = BaseBL*overfetch_mult/height;
      for (int k=0;k< extraBeat/extraheight;k++){
        for (int i = 0; i < msgWidth; i++) {  // chip width
          for (int j = 0; j < extraheight; j++) { //height
            bitArr[msgWidth*height*(offest_redundancy) + msgWidth*extraheight*k  + i * extraheight + j] = 
                data->bitArr[channelWidth * j + pos * msgWidth + i + channelWidth * BaseBL * overfetch_mult +
                channelWidth * extraheight * k ];
          }
        }
      }
    }
    
  }
  else if (layout == USEMESGCONFIG){
    int height = message_config.get_height_base();
    int BaseBL = message_config.get_BaseBL();
    int extraBeat = message_config.get_extraBeat();
    int msgWidth = message_config.get_chipwidth();
    int chipnumber = message_config.get_chipnumber();
    int extraheight = message_config.get_extraheight();
    int overfetch_mult = message_config.get_overfetch_mult();
    int chipWidth = message_config.get_extrapin();
    RedundancyMode extramode = message_config.get_extramode();
    // NEED to add per BL return using pos for extra chip mode.

    if (extramode != EXTRACHIP){
      // if pos == -1 then it is not a right value
      assert(pos != -1);
      for (int k=0;k<BaseBL*overfetch_mult/height;k++){
        for (int i = 0; i < msgWidth; i++) {  
          for (int j = 0; j < height; j++) { // height
            // continuous 16 bits in bamboo style will be 2 symbols.
            bitArr[msgWidth*height*k  + i * height + j] = 
              data->bitArr[channelWidth * j + pos * chipWidth + i + channelWidth * height * k ];
          }
        }
      }
      if (extramode == EXTRABEAT){
        if (extraBeat =! 0 and extraheight != 0){
          int offest_redundancy = BaseBL*overfetch_mult/height;
          for (int k=0;k< extraBeat/extraheight;k++){
            for (int i = 0; i < chipWidth; i++) {  // chip width
              for (int j = 0; j < extraheight; j++) { //height
                bitArr[chipWidth*height*(offest_redundancy) + chipWidth*extraheight*k  + i * extraheight + j] = 
                    data->bitArr[channelWidth * j + pos * chipWidth + i + channelWidth * BaseBL * overfetch_mult +
                    channelWidth * extraheight * k ];
              }
            }
          }
        }
        
      }else if (extramode == EXTRAPIN){
        if (extraheight != 0){
          int offest_redundancy = BaseBL*overfetch_mult/height;
          for (int i = 0; i < chipWidth-msgWidth; i++) {  // chip width
            for (int j = 0; j < extraheight; j++) { //height
              bitArr[msgWidth*height*(offest_redundancy) +  i * extraheight + j] = 
                  data->bitArr[channelWidth * j + pos * chipWidth + msgWidth + i];
            }
          }
        }
      }
    }else{
      // redundancy comming from another chip
      // Need to bypass extra pin and extra beat
      if(pos == -1){
        // For entire chip return
        for (int chipnum = 0;chipnum < chipnumber;chipnum++){
          for(int k=0;k<BaseBL*overfetch_mult/height;k++){
            for (int i = 0; i < msgWidth; i++) {  
              for (int j = 0; j < height; j++) { // height
                // continuous 16 bits in bamboo style will be 2 symbols.
                bitArr[chipnum*BaseBL*overfetch_mult*msgWidth + msgWidth*height*k  + i * height + j] = 
                  data->bitArr[channelWidth * j + chipnum * chipWidth + i + channelWidth * height * k ];
              }
            }
          }
        }
      }else {
        for (int chipnum = 0;chipnum < chipnumber;chipnum++){
          for (int i = 0; i < msgWidth; i++) {  
            for (int j = 0; j < height; j++) { // height
              // continuous 16 bits in bamboo style will be 2 symbols.
              bitArr[chipnum*height*overfetch_mult*msgWidth + i * height + j] = 
                data->bitArr[channelWidth * j + chipnum * chipWidth + i + channelWidth * height * pos ];
            }
          }
        }
      }
      if (extraheight != 0){
        assert(pos != -1);
        int offest_redundancy = BaseBL*overfetch_mult/height;
        for (int i = 0; i < chipWidth; i++) {  // chip width
          for (int j = 0; j < extraheight; j++) { //height
            bitArr[msgWidth*height*(offest_redundancy) +  i * extraheight + j] = 
                data->bitArr[channelWidth * j + pos * chipWidth + i + channelWidth * BaseBL * overfetch_mult];
          }
        }
      }
    }
  }
   else {
    assert(0);
  }
}

                      

//----------------------------------------------------------
void CacheLine::print() const {
  uint128_t buffer;
  std::stringstream hex; 
  std::streambuf *coutbuf;
  
  for (int i = beatHeight - 1; i >= 0; i--) {
    buffer = 0;
    for (int j = channelWidth - 1; j >= 0; j--) {
      
      buffer = (buffer << 1) | bitArr[channelWidth * i + j];
      if ((j % chipWidth) == 0) {
        hex << std::hex << buffer;
        std::cout<<std::setfill('0') << std::setw(int(chipWidth/4)) <<hex.str();
        if (j > 0) {
          //fprintf(fd, "_");
          std::cout<<"_";
        }
        hex.str("");
        buffer = 0;
      }
    }
    //fprintf(fd, "\n");
    std::cout<<"\n";
  }
  // flush the buffer
  std::cout.flush();  

}
