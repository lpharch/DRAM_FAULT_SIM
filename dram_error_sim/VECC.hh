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
 * @file: VECC.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * Virtualized ECC declaration
 */

#ifndef __VECC_HH__
#define __VECC_HH__

#include "ECC.hh"

#if 0
//------------------------------------------------------------------------------
class VECCST: public ECC {
public:
    VECCST(ECCLayout _layout) : ECC(_layout) {}

    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
};

//------------------------------------------------------------------------------
// VECC on 128b interface for x4 chipkill
//------------------------------------------------------------------------------
class VECC128bx4: public VECCST {
public:
    VECC128bx4();
};

//------------------------------------------------------------------------------
// VECC on 128b interface for x8 chipkill
//------------------------------------------------------------------------------
class VECC128bx8: public VECCST {
public:
    VECC128bx8();
};

//------------------------------------------------------------------------------
class VECC64bAMD: public VECCST {
public:
    VECC64bAMD();
};

//------------------------------------------------------------------------------
class VECC64bQPC: public VECCST {
public:
    VECC64bQPC();

    ErrorType postprocess(FaultDomain *fd, ErrorType preResult);
};

//------------------------------------------------------------------------------
class VECC64bS8SCD8SD: public VECCST {
public:
    VECC64bS8SCD8SD();
};

class VECC64bMultix4: public VECCST {
public:
    VECC64bMultix4();

    ErrorType decode(FaultDomain *fd, CacheLine &blk);
};


//------------------------------------------------------------------------------
class VECCTT: public ECC {
public:
    VECCTT() : ECC() {}

    ErrorType decode(FaultDomain *fd, CacheLine &errorBlk);
protected:
    Codec *codec1;
    Codec *codec2;
};

//------------------------------------------------------------------------------
// 101: VECC on 136b interface for x4 chipkill
//------------------------------------------------------------------------------
class VECC136bx4: public VECCTT {
public:
    VECC136bx4();
};

//------------------------------------------------------------------------------
// 121: VECC on 144b interface for x8 chipkill
//------------------------------------------------------------------------------
class VECC144bx8: public VECCTT {
public:
    VECC144bx8();
};

//------------------------------------------------------------------------------
class VECC72bS8SCD8SD: public VECCTT {
public:
    VECC72bS8SCD8SD();
};
#endif

#endif /* __VECC_HH__ */
