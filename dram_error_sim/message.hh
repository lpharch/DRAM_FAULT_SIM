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
 * @file: message.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * message declaration
 */

#ifndef __MESSAGE_HH__
#define __MESSAGE_HH__

#include <stdio.h>
#include <string>

#include "block.hh"
#include "util.hh"
enum RedundancyMode{
  EXTRABEAT,
  EXTRACHIP,
  EXTRAPIN,
  BOTH
};

class MSGConfig{
 public:
  MSGConfig(int _msg_height_base, int _DRAM_BaseBL, int _DRAM_extraBeat, 
                          int _msg_extraheight, int _DRAM_overfetch_mult, int _CHIP_width, int _CHIP_number, int _Extra_pin=0,
                          RedundancyMode _mode=EXTRABEAT)
        : msg_height_base(_msg_height_base),
        DRAM_BaseBL(_DRAM_BaseBL),
        DRAM_extraBeat(_DRAM_extraBeat),
        msg_extraheight(_msg_extraheight),
        DRAM_overfetch_mult(_DRAM_overfetch_mult),
        CHIP_width(_CHIP_width),
        CHIP_number(_CHIP_number),
        Extra_pin(_Extra_pin),
        extramode(_mode)
        {}
 public:
  int get_height_base(){return msg_height_base;}
  int get_BaseBL(){return DRAM_BaseBL;}
  int get_extraBeat(){return DRAM_extraBeat;}
  int get_extraheight(){return msg_extraheight;}
  int get_overfetch_mult(){return DRAM_overfetch_mult;}
  int get_chipwidth(){return CHIP_width;}
  int get_chipnumber(){return CHIP_number;}
  int get_extrapin(){return Extra_pin + CHIP_width;}
  RedundancyMode get_extramode(){return extramode;}
  void set_msg_height_base(int _msg_height_base){msg_height_base = _msg_height_base;}
  
 protected:
  int msg_height_base; //!< Bamboo sytle burst length
  int DRAM_BaseBL; //!< blkHeight = DRAM_BaseBL * DRAM_overfetch_mult + DRAM_extraBeat
  int DRAM_extraBeat; //!< Redundant Beat for ECC
  int msg_extraheight; //!< For redundant symbol, bamboo style burstlength
  int DRAM_overfetch_mult; //!< number of extra burst length for over fetching
  int CHIP_width; //!< number of all pins in a chip including redundant pins
  int CHIP_number;
  int Extra_pin; //!< number of extra pins for ecc
  RedundancyMode extramode;
};
//----------------------------------------------------------
// MSB: redudancy
// LSB: data

/*! \addtogroup Data_Block
 * @{
 *
 * @class ECCWord
 * \brief data block for ECC codeword
 */

class ECCWord : public Block {
  /*! @} */
  // constructor / destructor
 public:
  //! Constructor
  /*! \param bitN codeword size
                  \param bitK data size
                  */
  ECCWord(int bitN, int _bitK) : Block(bitN), bitK(_bitK) {}
  ECCWord() : Block(0), bitK(0) {}

  // member methods
 public:
  int getBitK() { return bitK; }
  //! Extract a codeword from a data block (cacheline)
  /*!\param data Source cacheline
           \param layout ECC layout that can vary depending on ECC scheme
           \param pos The n-th codeword in a cacheline
           \param channelWidth Channel width information of DIMM
          */
  void extract(Block* data, ECCLayout layout, int pos,
               int channelWidth, MSGConfig message_config=MSGConfig(0,0,0,0,0,0,0));  // Extract ECC words from data block "data",
                                   // according to layout

  // member fields
 private:
  int bitK;
};

//----------------------------------------------------------
// data layout: <---------- DQ ------------>
//             | 0  1  2  3  4   5 ...    71
//            BL 72 73 74 75 76 77 ...    143
//             | ...

/** @class CacheLine
 * \brief data block for cacheline
 */
class CacheLine : public Block {
  // constructor/destructor
 public:
  //! Constructor
  /*! \param _chipWidth Bit-width from a chip
          \param _channelWidth Bit-width of a entire channel
          \param _beatHeight Burst length
          */
  CacheLine(int _chipWidth, int _channelWidth, int _beatHeight, MSGConfig _message_config=MSGConfig(0,0,0,0,0,0,0))
      : Block(_channelWidth * _beatHeight),
        chipWidth(_chipWidth),
        channelWidth(_channelWidth),
        beatHeight(_beatHeight),
        message_config(_message_config) {}
  // member methods
 public:
  int getChipWidth() { return chipWidth; }
  int getChipCount() { return channelWidth / chipWidth; }
  int getChannelWidth() { return channelWidth; }
  int getBeatHeight() { return beatHeight; }
  MSGConfig get_messageConfig(){return message_config;}
  int setBeatHeight(int _beatHeight) { beatHeight = _beatHeight; }

  void print() const;
  // member fields
 protected:
  int chipWidth, channelWidth, beatHeight;
  MSGConfig message_config;
};


#endif /* __MESSAGE_HH__ */
