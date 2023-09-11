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
 * @file: Bamboo.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * @brief Bamboo-ECC declaration
 */

#ifndef __BAMBOO_HH__
#define __BAMBOO_HH__

#include "ECC.hh"
#include "rs.hh"

/**
 * @brief SPC on 66b interface
 */
class SPC66bx4 : public ECC {
 public:
  SPC66bx4();
};

/**
 * @brief SPC-TPD on 68b interface
 */
class SPCTPD68bx4 : public ECC {
 public:
  SPCTPD68bx4();
};

/**
 * @brief QPC on 72b interface for x4 chipkill
 */
class QPC72b : public ECC {
 public:
  QPC72b(int correction, int maxPins, bool _doPostprocess = true);
  QPC72b() : QPC72b(4, 2) {}

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

 protected:
  int maxPins;
};

/**
 * @brief QPC on 76b interface for AIECC
 */
class QPC76b : public ECC {
 public:
  QPC76b(int correction, int maxPins, bool _doPostprocess = true);
  QPC76b() : QPC76b(4, 2) {}

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);

 protected:
  int maxPins;
};
/**
 * @brief QPC on 80b interface for x8 chipkill
 */
class OPC80b : public ECC {
 public:
  OPC80b();

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief OPC on 144b interface for x8 chipkill
 */
class OPC144b : public ECC {
 public:
  OPC144b();

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief QPC with chip retirement
 */
class QPCCR : public ECC {
  QPCCR() {
    configList.push_back({0, 0, new RS<2, 8>("QPC\t18\t4\t", 72, 8, 4)});
    configList.push_back({0, 0, new RS<2, 8>("SPC-TPD\t17\t4\t", 68, 4, 1)});
  }
  ErrorType postprocess(FaultDomain *fd, ErrorType preResult) {
    if (fd->retiredChipIDList.size() < 1) {
      if (correctedPosSet.size() > 2) {
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
    }
    return preResult;
  }
};

/**
 * @brief QPC with pin retirement
 */
class QPCPR : public ECC {};
#endif /* __BAMBOO_HH__ */
