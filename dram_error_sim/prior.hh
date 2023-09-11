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
 * @file: prior.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Prior ECC work declaration
 */

#ifndef __PRIOR_HH__
#define __PRIOR_HH__

#include "ECC.hh"

/**
 * @brief No ECC for comparison
 */
class ECCNone : public ECC {
 public:
  ECCNone() : ECC() {}
};

/**
 * @brief SEC-DED on 72-bit interface
 */
class SECDED72b : public ECC {
 public:
  SECDED72b();
};

/**
 * @brief S4SCS-D4SD on 144b interface for x4 chipkill
 */
class S4SCD4SD144b : public ECC {
 public:
  S4SCD4SD144b();
};

/**
 * @brief S8SC on 80b interface for x8 chipkill
 */
class S8SC80b : public ECC {
 public:
  S8SC80b();

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief S8SC on 144b interface for x8 chipkill
 */
class S8SC144b : public ECC {
 public:
  S8SC144b();

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief AMD chipkill on 72b interface for x4 chipkill
 */
class AMDChipkill72b : public ECC {
 public:
  AMDChipkill72b(bool _doPostprocess = true);
  //    AMDChipkill72b(bool _doPostprocess, bool _doRetire);

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

class AMDChipkill20b : public ECC {
 public:
  AMDChipkill20b(bool _doPostprocess = true);
  //    AMDChipkill20b(bool _doPostprocess, bool _doRetire);

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

/**
 * @brief AMD chipkill on flexible interface for x4 chipkill
 */
class AMDChipkill_FLEX : public ECC {
 public:
  AMDChipkill_FLEX(bool _doPostprocess = true, bool _onchipecc = true, int _half_chipkill = 0);
  //    AMDChipkill72b(bool _doPostprocess, bool _doRetire);
  ~AMDChipkill_FLEX();
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
 protected:
  Codec *onchip_codec;
  bool onchipecc;
  int half_chipkill;
};


/**
 * @brief AMD chipkill on 144b interface
 */
class AMDDChipkill144b : public ECC {
 public:
  AMDDChipkill144b(bool _doPostprocess = true);

  ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

#endif /* __PRIOR_HH__ */
