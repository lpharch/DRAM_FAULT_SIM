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

#include "VECC.hh"
#include "DomainGroup.hh"
#include "FaultDomain.hh"
#include "codec.hh"
#include "hsiao.hh"
#include "rs.hh"

#if 0
//------------------------------------------------------------------------------
ErrorType VECCST::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
    Codec *codec = configList.back().codec;

    ECCWord msg = {codec->getBitN(), codec->getBitK()};
    ECCWord decoded = {codec->getBitN(), codec->getBitK()};

    ErrorType result = NE, newResult;
    for (int i=errorBlk.getBitN()/codec->getBitK()-1; i>=0; i--) {
        DataArrType *dataPtr = errorBlk.getDataPointer(codec->getBitK(), i);
        msg.extract(dataPtr, layout, errorBlk.getChannelWidth());

        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec->decode(&msg, &decoded, &correctedPosSet);
        }
        result = worse2ErrorType(result, newResult);
    }

    if (result==NE) {
        errorBlk.print();
        assert(result!=NE);
    }

    return result;
}

//------------------------------------------------------------------------------
VECC128bx4::VECC128bx4() : VECCST(AMD) {
    configList.push_back({0, 0, new RS<2, 8>("VECC 128b x4\t", 35, 3, 1)});
}

//------------------------------------------------------------------------------
VECC128bx8::VECC128bx8() : VECCST(LINEAR) {
    configList.push_back({0, 0, new RS<2, 8>("VECC 128b x8\t", 19, 3, 1)});
}

//------------------------------------------------------------------------------
VECC64bAMD::VECC64bAMD() : VECCST() {
    layout = AMD;
    codec = new RS<2, 8>("VECC 64b (AMD)\t", 18, 2, 1);
}

//------------------------------------------------------------------------------
VECC64bQPC::VECC64bQPC() : VECCST() {
    layout = PIN;
    codec = new RS<2, 8>("VECC 64b (QPC)\t", 72, 8, 4);
}

ErrorType VECC64bQPC::postprocess(FaultDomain *fd, ErrorType preResult) {
    if (correctedPosSet.size() > 2) {
        int chipPos = -1;
        for (auto it = correctedPosSet.cbegin(); it != correctedPosSet.cend(); it++) {
            int newChipPos = (*it)/4;
            if (chipPos==-1) {  // first
                chipPos = newChipPos;
            } else {
                if (newChipPos!=chipPos) {
                    correctedPosSet.clear();
                    return DUE;
                }
            }
        }
    }
    return preResult;
}

//------------------------------------------------------------------------------
VECC64bS8SCD8SD::VECC64bS8SCD8SD() : VECCST() {
    layout = AMD;
    codec = new RS<2, 8>("VECC 64b (S8SC-D8SD)\t", 19, 3, 1);
}

//------------------------------------------------------------------------------
VECC64bMultix4::VECC64bMultix4() : VECCST() {
    layout = MULTIX4;
    codec = new RS<2, 16>("VECC 64b (Multix4)\t", 17, 1, 0);
}

ErrorType VECC64bMultix4::decode(FaultDomain *fd, CacheLine &errorBlk) {
    int sym_size = 16;
    ErrorType result = NE, newResult;

    for (int i=errorBlk.getBeatHeight()/4-1; i>=0; i--) {
        ECCWord msg = {17*sym_size, 16*sym_size};
        ECCWord decoded = {17*sym_size, 16*sym_size};
        msg.reset();
        decoded.reset();

        DataArrType *dataPtr = errorBlk.getDataPointer(64*4, i);
        msg.extract(dataPtr, layout, errorBlk.getChannelWidth());
        
        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec->decode(&msg, &decoded);

            if (newResult==DUE) {
                // 1. error is detected by T1EC
                int nonZeroChecksumCount = 0;
                FaultRange *nonZeroChecksumFR = NULL;

                auto it2 = fd->faultRangeList.rbegin();
                FaultRange *newFault = (*it2);
                for (auto it = fd->faultRangeList.begin(); it != fd->faultRangeList.end(); it++) {
                    if (!newFault->overlap(*it)) {  // unrelated error
                        continue;
                    }

                    // 2. generate checksum
                    if ((*it)->isMultiRow()) {
                        uint32_t origChecksum = 0;
                        uint32_t errorChecksum = 0;

                        for (int j=0; j<256; j++) {
                            uint16_t data = rand() % 0x10000;
                            uint16_t error;
                            if (j==0) {
                                error = decoded.getSymbol(16, (*it)->getDeviceNum());
                            } else {
                                if ((*it)->isMultiDQ()) {
                                    error = rand() % 0x10000;
                                } else {
                                    int pinLoc = (*it)->getPinID()%8;
                                    error = ((rand()%2) << pinLoc) | (rand()%2 << (pinLoc+8));
                                }
                            }
                            origChecksum += data;
                            origChecksum = (origChecksum & 0xFFFF) + (origChecksum>>16);
                            errorChecksum += (data ^ error);
                            errorChecksum = (errorChecksum & 0xFFFF) + (errorChecksum>>16);
                        }
                        if (origChecksum!=errorChecksum) {
                            nonZeroChecksumCount++;
                            nonZeroChecksumFR = *it;
                        }
                    } else {
                        nonZeroChecksumCount++;
                        nonZeroChecksumFR = *it;
                    }
                }
                if (nonZeroChecksumCount==1) {
                    // correct it
                    decoded.setSymbol(16, nonZeroChecksumFR->getDeviceNum(), 0);

                    if (decoded.isZero()) {
                        //printf("CORRECTED! - uncompressed\n");
                        //msg.print();
                        //decoded.print();
                        //errorBlk.print();
                        newResult = CE;
                    } else {
                        //printf("SDC from checksum error - uncompressed\n");
                        //fd->print();
                        //decoded.print();
                        //errorBlk.print();
                        newResult = SDC;
                    }
                } else {
                    newResult = DUE;
                }
            } else {
                //printf("SDC from RS error - real uncompressed \n");
                //fd->print();
                //decoded.print();
                //errorBlk.print();
                newResult = SDC;
            }
        }

        result = worse2ErrorType(result, newResult);
    }
    // 2. half compressed possible? NO

//    assert(result!=NE);

//    result = postprocess(fd, result);

    return result;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ErrorType VECCTT::decode(FaultDomain *fd, CacheLine &errorBlk) {
    preprocess(fd);

    ECCWord msg = {codec1->getBitN(), codec1->getBitK()};
    ECCWord decoded = {codec1->getBitN(), codec1->getBitK()};

    ErrorType result = NE, newResult;
    assert(errorBlk.getBitN()%codec1->getBitN()==0);
    for (int i=errorBlk.getBitN()/codec1->getBitN()-1; i>=0; i--) {
        DataArrType *dataPtr = errorBlk.getDataPointer(codec1->getBitN(), i);
        msg.extract(dataPtr, layout, errorBlk.getChannelWidth());

        if (msg.isZero()) {        // error-free region of a block -> skip
            newResult = NE;
        } else {
            newResult = codec1->decode(&msg, &decoded, &correctedPosSet);
            if (newResult==DUE) {
                ECCWord msg2 = {codec2->getBitN(), codec2->getBitK()};
                ECCWord decoded2 = {codec2->getBitN(), codec2->getBitK()};

                msg2.extract(dataPtr, layout, errorBlk.getChannelWidth());

                newResult = codec2->decode(&msg2, &decoded2, &correctedPosSet);
            }
        }
        result = worse2ErrorType(result, newResult);
    }

    if (result==NE) {
        errorBlk.print();
        assert(result!=NE);
    }

    result = postprocess(fd, result);

    return result;
}

//------------------------------------------------------------------------------
VECC136bx4::VECC136bx4() : VECCTT() {
    layout = AMD;
    codec1 = new RS<2, 8>("VECC 136b x4\t", 34, 2, 0);
    codec2 = new RS<2, 8>("VECC 136b x4\t", 35, 3, 1);
}

//------------------------------------------------------------------------------
VECC144bx8::VECC144bx8() : VECCTT() {
    layout = LINEAR;
    codec1 = new RS<2, 8>("VECC 144b (S8SC-D8SD)\t", 18, 2, 0);
    codec2 = new RS<2, 8>("VECC 144b (S8SC-D8SD)\t", 19, 3, 1);
}

//------------------------------------------------------------------------------
VECC72bS8SCD8SD::VECC72bS8SCD8SD() : VECCTT() {
    layout = AMD;
    codec1 = new RS<2, 8>("VECC 72b (S8SC-D8SD)\t", 18, 2, 0);
    codec2 = new RS<2, 8>("VECC 72b (S8SC-D8SD)\t", 19, 3, 1);
}
#endif
