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

#include "Bamboo.hh"

//------------------------------------------------------------------------------
// SPC on 66b interface
//------------------------------------------------------------------------------
SPC66bx4::SPC66bx4() : ECC(PIN) {
  configList.push_back({0, 0, new RS<2, 8>("SPC\t17\t4\t", 66, 2, 1)});
}

//------------------------------------------------------------------------------
// SPC-TPD on 68b interface
//------------------------------------------------------------------------------
SPCTPD68bx4::SPCTPD68bx4() : ECC(PIN) {
  configList.push_back({0, 0, new RS<2, 8>("SPC-TPD\t17\t4\t", 68, 4, 1)});
}

//------------------------------------------------------------------------------
// QPC on 72b interface
//------------------------------------------------------------------------------
QPC72b::QPC72b(int correction, int _maxPins, bool _doPostprocess)
    : ECC(PIN, _doPostprocess) {
  maxPins = _maxPins;
  configList.push_back({0, 0, new RS<2, 8>("QPC\t18\t4\t", 72, 8, correction)});
}

ErrorType QPC72b::postprocess(FaultDomain *fd, ErrorType preResult) {
  if (correctedPosSet.size() > maxPins) {
    int chipPos = -1;
    for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend();
         it++) {
      int newChipPos = (*it) / 4;
      if (chipPos == -1) {  // first chip location
        chipPos = newChipPos;
      } else {
        if (chipPos != newChipPos) {
          correctedPosSet.clear();
          return DUE;
        }
      }
    }
  }
  return preResult;
}

//------------------------------------------------------------------------------
// QPC on 76b interface (for AIECC)
//------------------------------------------------------------------------------
QPC76b::QPC76b(int correction, int _maxPins, bool _doPostprocess)
    : ECC(PIN, _doPostprocess) {
  maxPins = _maxPins;
  configList.push_back({0, 0, new RS<2, 8>("QPC\t19\t4\t", 76, 8, correction)});
}

ErrorType QPC76b::postprocess(FaultDomain *fd, ErrorType preResult) {
  if (correctedPosSet.size() > maxPins) {
    int chipPos = -1;
    for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend();
         it++) {
      int newChipPos = (*it) / 4;
      if (chipPos == -1) {  // first chip location
        chipPos = newChipPos;
      } else {
        if (chipPos != newChipPos) {
          correctedPosSet.clear();
          return DUE;
        }
      }
    }
  }
  return preResult;
}

//------------------------------------------------------------------------------
// QPC on 80b interface for x8 chipkill
//------------------------------------------------------------------------------
OPC80b::OPC80b() : ECC(PIN, true) {
  configList.push_back({0, 0, new RS<2, 8>("OPC80b\t10\t8\t", 80, 16, 8)});
}

ErrorType OPC80b::postprocess(FaultDomain *fd, ErrorType preResult) {
  if (correctedPosSet.size() > 2) {
    int chipPos = -1;
    for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend();
         it++) {
      int newChipPos = (*it) / 8;
      if (chipPos == -1) {  // first
        chipPos = newChipPos;
      } else {
        if (newChipPos != chipPos) {
          correctedPosSet.clear();
          return DUE;
        }
      }
    }
  }
  return preResult;
}

//------------------------------------------------------------------------------
// OPC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
OPC144b::OPC144b() : ECC(PIN, true) {
  configList.push_back({0, 0, new RS<2, 8>("OPC144b\t18\t8\t", 144, 16, 8)});
}

ErrorType OPC144b::postprocess(FaultDomain *fd, ErrorType preResult) {
  if (correctedPosSet.size() > 2) {
    int chipPos = -1;
    for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend();
         it++) {
      int newChipPos = (*it) / 8;
      if (chipPos == -1) {  // first
        chipPos = newChipPos;
      } else {
        if (newChipPos != chipPos) {
          correctedPosSet.clear();
          return DUE;
        }
      }
    }
  }
  return preResult;
}
