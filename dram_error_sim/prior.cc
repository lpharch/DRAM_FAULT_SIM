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

#include "prior.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"
#include "rs.hh"

//------------------------------------------------------------------------------
// SEC-DED on 72-bit interface
//------------------------------------------------------------------------------
SECDED72b::SECDED72b() : ECC(LINEAR)
{
  configList.push_back({0, 0, new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8)});
}

//------------------------------------------------------------------------------
// S4SCS-D4SD on 144b interface for x4 chipkill
//------------------------------------------------------------------------------
S4SCD4SD144b::S4SCD4SD144b() : ECC(LINEAR)
{
  configList.push_back({0, 0, new RS<2, 4>("S4SCD4SD 144b\t", 36, 4, 1)});
}

//------------------------------------------------------------------------------
// S8SC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
S8SC80b::S8SC80b() : ECC(LINEAR, true)
{
  configList.push_back({0, 0, new RS<2, 8>("S8SC80b\t10\t8\t", 10, 2, 1)});
}

ErrorType S8SC80b::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 1)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}

//------------------------------------------------------------------------------
// S8SC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
S8SC144b::S8SC144b() : ECC(LINEAR, true)
{
  configList.push_back({0, 0, new RS<2, 8>("S8SC144b\t18\t8\t", 18, 2, 1)});
}

ErrorType S8SC144b::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 1)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}

//------------------------------------------------------------------------------
// AMD chipkill on 72b interface for x4 chipkill
//------------------------------------------------------------------------------
AMDChipkill72b::AMDChipkill72b(bool _doPostprocess) : ECC(AMD, _doPostprocess)
{
  configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 18, 2, 1)});
  setBitN(136);
  this->setInDRAM(DoubleDouble18);
  this->setInDRAMDown(Double);
}

// AMDChipkill72b::AMDChipkill72b(bool _doPostprocess, bool _doRetire, int max)
// : ECC(AMD, _doPostprocess, _doRetire, max) {
//    configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 18, 2, 1)});
//	this->setInDRAM(Triple);
//	this->setInDRAMDown(Double);
//	setBitN(612);
//}

ErrorType AMDChipkill72b::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 1)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}

//------------------------------------------------------------------------------
// AMD chipkill on flexible interface for x4 chipkill
//------------------------------------------------------------------------------
AMDChipkill_FLEX::AMDChipkill_FLEX(bool _doPostprocess, bool _onchipecc, int _half_chipkill) : ECC(AMD, _doPostprocess)
{
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8); 
  onchipecc = _onchipecc;
  half_chipkill = _half_chipkill;
  if (half_chipkill==1){
    configList.push_back({0, 0, new RS<2, 8>("S4SCD4SD 144b\t", 36, 2, 1)});
  } else if (half_chipkill==2){
    configList.push_back({0, 0, new RS<2, 8>("S4SCD4SD 144b\t", 18, 2, 1)});
  } else {
    configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 10, 2, 1)});
  }
  setBitN(640);
  
  
  if (_onchipecc){
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Double);
  } else{
    this->setInDRAM(SingleSingle10);
    this->setInDRAMDown(Single);
  }
}
AMDChipkill_FLEX::~AMDChipkill_FLEX()
{
  delete onchip_codec;
}

// AMDChipkill72b::AMDChipkill72b(bool _doPostprocess, bool _doRetire, int max)
// : ECC(AMD, _doPostprocess, _doRetire, max) {
//    configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 18, 2, 1)});
//	this->setInDRAM(Triple);
//	this->setInDRAMDown(Double);
//	setBitN(612);
//}
ErrorType AMDChipkill_FLEX::decodeInternal(FaultDomain *fd, CacheLine &errorBlk)
{
  //onchip ECC
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};
  MSGConfig IECC_message = MSGConfig(16,16,0,8,1,errorBlk.getChipWidth()-1,errorBlk.getChipCount(),1,EXTRAPIN);
  ErrorType result = NE;
  if (errorBlk.isZero()) {
    return NE;
  }
  if(onchipecc == true){
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg.extract(&errorBlk, USEMESGCONFIG, i, errorBlk.getChannelWidth(),IECC_message);

      if (!msg.isZero()) {
        onchip_codec->decode(&msg, &decoded, &correctedPosSet);
        if (correctedPosSet.size() == 1) {
          auto it = correctedPosSet.begin();
          int width = IECC_message.get_extrapin();
          int height = IECC_message.get_height_base();
          // Check if the corrected position is in the ECC region
          ECCWord MASK = {36,32};
          MASK.reset();
          MASK.copyfromN_bringM(&msg,int(*it/32)*32,32);
          if (!MASK.isZero()){
            errorBlk.invBit(i * width + ((*it) % height)*errorBlk.getChannelWidth() + ((*it)/height));
            correctedPosSet.clear();
          }
        }
      }
    }
  }
  

  if (errorBlk.isZero()) {
    return CE;
  } else {
    Codec *codec = configList.front().codec;
    ECCWord msg_chipkill = {codec->getBitN(), codec->getBitK()};
    ECCWord decoded_chipkill = {codec->getBitN(), codec->getBitK()};
    ErrorType internal_result = NE;
    auto msgconfig = errorBlk.get_messageConfig();
    int total_loops = msgconfig.get_BaseBL() / msgconfig.get_height_base();
    for(int i = 0; i < total_loops; i++){
      msg_chipkill.extract(&errorBlk, USEMESGCONFIG, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      internal_result = codec->decode(&msg_chipkill, &decoded_chipkill, &correctedPosSet);
      result = worseErrorType(result, internal_result);
    }
    
    return result;
  }
  return result;
}

ErrorType AMDChipkill_FLEX::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 1)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}

//------------------------------------------------------------------------------
// AMD chipkill on 20b interface for x4 chipkill
//------------------------------------------------------------------------------
AMDChipkill20b::AMDChipkill20b(bool _doPostprocess) : ECC(AMD32BL, _doPostprocess)
{
  configList.push_back({0, 0, new RS<2, 8>("S8SC w/ H (AMD)\t", 24, 8, 4)});
  setBitN(768);
  this->setInDRAM(Penta);
  this->setInDRAMDown(Double);
}

ErrorType AMDChipkill20b::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 1)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}

//------------------------------------------------------------------------------
// AMD double-chipkill on 144b interface for x4 chipkill
//------------------------------------------------------------------------------
AMDDChipkill144b::AMDDChipkill144b(bool _doPostprocess)
    : ECC(AMD, _doPostprocess)
{
  configList.push_back({0, 0, new RS<2, 8>("D8SC w/ H (AMD)\t", 36, 4, 2)});
}

ErrorType AMDDChipkill144b::postprocess(FaultDomain *fd, ErrorType preResult)
{
  if (correctedPosSet.size() > 2)
  {
    correctedPosSet.clear();
    return DUE;
  }
  else
  {
    return preResult;
  }
}
