
#include "FlipCorrection.hh"
#include "FaultDomain.hh"
#include "hsiao.hh"
#include "Bamboo.hh"
#include "Huawei.hh"
#include "combination.hh"

b8CRC::b8CRC(const char *name, int _bitN, int _bitR, int enable_1bitfix): 
                            Codec(name, _bitN, _bitR){
  Block testBlock(_bitN);
  ECCWord encoded(_bitN, _bitN - _bitR);
  bool _enable_1bitfix = enable_1bitfix;

  for (int i = 0; i < 256; i++) {
    correctionTable[i] = 255;
  }
  for (int i = 0; i < _bitN - bitR; i++) {
    testBlock.reset();
    testBlock.setBit(i, 1);

    encode(&testBlock, &encoded);

    unsigned char crc = 0;
    for (int j = bitK; j < bitN; j++) {
      crc = (crc << 1) | encoded.getBit(j);
    }
    correctionTable[crc] = i;
  }
}

/* C code fore CRC encoding */
/* input : data
 * output : encoded data
 */
void b8CRC::encode(Block *data, ECCWord *encoded) {
  bool CRC[8] = {
      false,
  };
  encoded->reset();
  for (int i = 0; i < bitK; i++) {
    bool doInvert = data->getBit(i) ^ CRC[7];
    //// polynomial x^8 + x^7 + x^6 + x^3 + x^2 + x + 1
    //// 111001111 
    CRC[7] = CRC[6] ^ doInvert;
    CRC[6] = CRC[5] ^ doInvert;
    CRC[5] = CRC[4];
    CRC[4] = CRC[3];
    CRC[3] = CRC[2] ^ doInvert;
    CRC[2] = CRC[1] ^ doInvert;
    CRC[1] = CRC[0] ^ doInvert;
    CRC[0] = doInvert;
  }
  for (int i = 0; i < bitK; i++) {
    encoded->setBit(i, data->getBit(i));
  }
  for (int i = bitK; i < bitN; i++) {
    encoded->setBit(i, CRC[i - bitK]);
  }
}


ErrorType b8CRC::decode(ECCWord *msg, ECCWord *decoded,
                           std::set<int> *correctedPos) {
  bool CRC[8] = {
      false,
  };
  decoded->reset();
  bool isDataAllZero = true;
  bool isOnlyOnebitFlip = false;
  int flipCount=0;
  for (int i = 0; i < bitK; i++) {
    bool data = msg->getBit(i);
    if (data) {
      isDataAllZero = false;
      flipCount++;
    }
    decoded->setBit(i, data);
    bool doInvert = data ^ CRC[7];
    //// polynomial x^8 + x^7 + x^6 + x^3 + x^2 + x + 1
    //// 111001111 
    CRC[7] = CRC[6] ^ doInvert;
    CRC[6] = CRC[5] ^ doInvert;
    CRC[5] = CRC[4];
    CRC[4] = CRC[3];
    CRC[3] = CRC[2] ^ doInvert;
    CRC[2] = CRC[1] ^ doInvert;
    CRC[1] = CRC[0] ^ doInvert;
    CRC[0] = doInvert;
  }
  unsigned char crc = 0;
  bool isCheckAllZero = true;
  for (int i = bitK; i < bitN; i++) {
    crc = (crc << 1) | CRC[i - bitK];

    bool check = msg->getBit(i);
    if (check) {
      flipCount++;
      isCheckAllZero = false;
    }
    decoded->setBit(i, 0);
  }
  if (flipCount == 1) {
    isOnlyOnebitFlip = true;
  }
  if (isOnlyOnebitFlip) {
    if (_enable_1bitfix)
      return CE;
    else
      return DUE;
  }
  if (crc == 0) {
    return NE;
  } else {
    return DUE;
  }
}



b16CRC::b16CRC(const char *name, int _bitN, int _bitR, int enable_1bitfix): 
                            Codec(name, _bitN, _bitR){
  Block testBlock(_bitN);
  ECCWord encoded(_bitN, _bitN - _bitR);
  bool _enable_1bitfix = enable_1bitfix;

  for (int i = 0; i < _bitN - bitR; i++) {
    testBlock.reset();
    testBlock.setBit(i, 1);

    encode(&testBlock, &encoded);

    unsigned char crc = 0;
    for (int j = bitK; j < bitN; j++) {
      crc = (crc << 1) | encoded.getBit(j);
    }
  }
}

/* C code fore CRC encoding */
/* input : data
 * output : encoded data
 */
void b16CRC::encode(Block *data, ECCWord *encoded) {
  bool CRC[16] = {
      false,
  };
  encoded->reset();
  for (int i = 0; i < bitK; i++) {
    bool doInvert = data->getBit(i) ^ CRC[15];
    //// polynomial x^16 +x^14 +x^12 +x^11 +x^8 +x^5 +x^4 +x^2 +1
    //// 10101100100110101
    CRC[15] = CRC[14];
    CRC[14] = CRC[13] ^ doInvert;
    CRC[13] = CRC[12];
    CRC[12] = CRC[11] ^ doInvert;
    CRC[11] = CRC[10] ^ doInvert;
    CRC[10] = CRC[9];
    CRC[9] = CRC[8];
    CRC[8] = CRC[7] ^ doInvert;
    CRC[7] = CRC[6];
    CRC[6] = CRC[5];
    CRC[5] = CRC[4] ^ doInvert;
    CRC[4] = CRC[3] ^ doInvert;
    CRC[3] = CRC[2];
    CRC[2] = CRC[1] ^ doInvert;
    CRC[1] = CRC[0];
    CRC[0] = doInvert;
  }
  for (int i = 0; i < bitK; i++) {
    encoded->setBit(i, data->getBit(i));
  }
  for (int i = bitK; i < bitN; i++) {
    encoded->setBit(i, CRC[i - bitK]);
  }
}


ErrorType b16CRC::decode(ECCWord *msg, ECCWord *decoded,
                           std::set<int> *correctedPos) {
  bool CRC[16] = {
      false,
  };
  decoded->reset();
  bool isDataAllZero = true;
  bool isOnlyOnebitFlip = false;
  int flipCount=0;
  for (int i = 0; i < bitK; i++) {
    bool data = msg->getBit(i);
    if (data) {
      isDataAllZero = false;
      flipCount++;
    }
    decoded->setBit(i, data);
    bool doInvert = data ^ CRC[15];
    //// polynomial x^16 +x^14 +x^12 +x^11 +x^8 +x^5 +x^4 +x^2 +1
    //// 10101100100110101
    CRC[15] = CRC[14];
    CRC[14] = CRC[13] ^ doInvert;
    CRC[13] = CRC[12];
    CRC[12] = CRC[11] ^ doInvert;
    CRC[11] = CRC[10] ^ doInvert;
    CRC[10] = CRC[9];
    CRC[9] = CRC[8];
    CRC[8] = CRC[7] ^ doInvert;
    CRC[7] = CRC[6];
    CRC[6] = CRC[5];
    CRC[5] = CRC[4] ^ doInvert;
    CRC[4] = CRC[3] ^ doInvert;
    CRC[3] = CRC[2];
    CRC[2] = CRC[1] ^ doInvert;
    CRC[1] = CRC[0];
    CRC[0] = doInvert;
  }
  unsigned int crc = 0;
  bool isCheckAllZero = true;
  int fix_location = 0;
  for (int i = bitK; i < bitN; i++) {
    crc = (crc << 1) | CRC[i - bitK];

    bool check = msg->getBit(i);
    if (check) {
      flipCount++;
      fix_location = i;
      isCheckAllZero = false;
    }
    
  }
  if (flipCount == 1) {
    isOnlyOnebitFlip = true;
  }
  if (isOnlyOnebitFlip) {
    if (_enable_1bitfix){
      decoded->setBit(fix_location, 0);
      return CE;
    }
    else{
      return DUE;
    }
  }
  if (crc == 0) {
    return NE;
  } else {
    return DUE;
  }
}


FlipCRC_Bamboo::FlipCRC_Bamboo(int _metadataByte,int _crcbits):
                   Bamboo40b(0,false){
  if (_crcbits==8){
    onchip_codec = new b8CRC("BitFlip 8bCRC",136, _crcbits);
  }else if (_crcbits==16){
    onchip_codec = new b16CRC("BitFlip 16bCRC",144, _crcbits);
  }
  if (_metadataByte == 0) {
    this->setInDRAM(Septa);
    this->setInDRAMDown(DoubleDouble10);
  } else if (_metadataByte == 2) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(DoubleDouble10);
  } else if (_metadataByte == 3) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Triple);
  } else if (_metadataByte == 4) {
    this->setInDRAM(DoubleDouble10);
    this->setInDRAMDown(Triple);
  } else {
    exit(0);
  }
  crcbit = _crcbits;

  setBitN(128+_crcbits); // 136 for 8 bit , 144 for 16 bit
}

ErrorType FlipCRC_Bamboo::decodeInternal(FaultDomain *fd, CacheLine &errorBlk){
  ECCWord msg(144,128);
  ECCWord decoded(144,128);
  ErrorType result = NE;
  bool oneErrorCorrection = false;
  CacheLine errorCpy(errorBlk.getChipWidth(),errorBlk.getChannelWidth(),errorBlk.getBeatHeight());
  //Block errorCpy(errorBlk.getBitN());
  ECCLayout layout;
  if(crcbit == 8){
    ECCWord msg(136, 128);
    ECCWord decoded(136, 128);
    layout = ONCHIPx4_IECC8_BL16_128_Overfetch;
  }else if(crcbit == 16){
    ECCWord msg(144, 128);
    ECCWord decoded(144, 128);
    layout = ONCHIPx4_IECC16_BL16_128_Overfetch;
  }
  

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    // msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());
    oneErrorCorrection = false;
    msg.reset();
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType tempresult = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tempresult);
      if(tempresult == CE){
        /*
        for (int ii = 0; ii < errorBlk.getBitN(); ii++) {
          errorCpy.bitArr[ii] = errorBlk.bitArr[ii];
        } 
        */  

        for (int j = 0; j < 64; j++) {
          if (msg.getBit(j) != 0){
            errorBlk.setBit(i * 4 + j % 4 +
                    (j / 4) * errorBlk.getChannelWidth(),0);
            oneErrorCorrection = true;
            break;
          }
        }
        msg.reset();
        msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
      }
      if(!msg.isZero(16) && tempresult <= CE) {
        //printf("SDC Happens during CRC correction ");
        //msg.print(stdout);
        result = worseErrorType(result,SDC);
      }
    }
  }
  
  if(result==DUE){
    result = Bamboo40b::decodeInternal(fd, errorBlk);
  } 
  if(result == SDC)
  {
    printf("SDC from CRC detection\n");
    return result;
  }
  return result; 
}


FlipCRC_DUO::FlipCRC_DUO(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount):
                   DUO36bx4(_maxPin,_doPostprocess,_doRetire,_maxRetiredBlkCount){
  onchip_codec = new b8CRC("BitFlip 8bCRC",136,8);
}


ErrorType FlipCRC_DUO::decodeInternal(FaultDomain *fd, CacheLine &errorBlk){
  ECCWord msg = {136, 128};
  ECCWord decoded = {136, 128};
  ErrorType result = NE;
  bool oneErrorCorrection = false;
  CacheLine errorCpy(errorBlk.getChipWidth(),errorBlk.getChannelWidth(),errorBlk.getBeatHeight());
  //Block errorCpy(errorBlk.getBitN());
  

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    // msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());
    oneErrorCorrection = false;
    msg.reset();
    msg.extract(&errorBlk, ONCHIPx4_IECC8_BL17DUO_1, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType tempresult = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tempresult);
      if(tempresult == CE){
        
        /*Debug code
        for (int ii = 0; ii < errorBlk.getBitN(); ii++) {
          errorCpy.bitArr[ii] = errorBlk.bitArr[ii];
        }   
        */ 

        for (int j = 0; j < 64; j++) {
          if (msg.getBit(j) != 0){
            errorBlk.setBit(i * 4 + j % 4 +
                    (j / 4) * errorBlk.getChannelWidth(),0);
            oneErrorCorrection = true;
            break;
          }
        }
        // ignore in-DRAM redundant bits
        for (int j = 64; j < 72; j++) {
          if (oneErrorCorrection) 
            break;
          if (msg.getBit(j) != 0){
            errorBlk.setBit((i) * 4 + j % 4 + (j / 4 + 1) * errorBlk.getChannelWidth(),0);
            oneErrorCorrection = true;
            break;
          }
        }
      }
      msg.reset();
      msg.extract(&errorBlk, ONCHIPx4_IECC8_BL17DUO_1, i, errorBlk.getChannelWidth());
      if(!msg.isZero(8) && tempresult <= CE) {
        //printf("SDC Happens during CRC correction");
        //msg.print(stdout);
        result = worseErrorType(result,SDC);
      }
    }
  }
  
  if(result==DUE){
    result = DUO36bx4::decodeInternal(fd, errorBlk);
  } 
  if(result == SDC)
  {
    return result;
  }
  return result; 
}

//------------------------------------------------------------------------------
Onchip144_128::Onchip144_128() : ECC() {
  onchip_codec = new Hsiao("SEC-DED (Hsiao)\t18\t4\t", 72, 8);
  setBitN(72);
}

ErrorType Onchip144_128::decodeInternal(FaultDomain *fd, CacheLine &errorBlk){
  ECCWord msg = {72, 64};
  ECCWord decoded = {72, 64};
  ErrorType result = NE;

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    // msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());
    msg.reset();
    msg.extract(&errorBlk, ONCHIPx4, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType tempresult = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tempresult);
    }
  }
  return result;
}


CRCECC128::CRCECC128(int _crcbits):
                   ECC(ONCHIPx4_IECC16_BL16_128_Overfetch){
  if (_crcbits==8){
    onchip_codec = new b8CRC("BitFlip 8bCRC",136, _crcbits, true);
    setInDRAM(2);
  }else if (_crcbits==16){
    onchip_codec = new b16CRC("BitFlip 16bCRC",144, _crcbits, true);
    setInDRAM(2);
  }

  crcbit = _crcbits;
  setInDRAMDown(1);

  setBitN(128+_crcbits); // 136 for 8 bit , 144 for 16 bit
}

ErrorType CRCECC128::decodeInternal(FaultDomain *fd, CacheLine &errorBlk){
  ECCWord msg(144,128);
  ECCWord decoded(144,128);
  ErrorType result = NE;
  bool oneErrorCorrection = false;
  CacheLine errorCpy(errorBlk.getChipWidth(),errorBlk.getChannelWidth(),errorBlk.getBeatHeight());
  //Block errorCpy(errorBlk.getBitN());
  ECCLayout layout;
  if(crcbit == 8){
    ECCWord msg(136, 128);
    ECCWord decoded(136, 128);
    layout = ONCHIPx4_IECC8_BL16_128_Overfetch;

  }else if(crcbit == 16){
    ECCWord msg(144, 128);
    ECCWord decoded(144, 128);
    layout = ONCHIPx4_IECC16_BL16_128_Overfetch;

  }
  

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    // msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());
    oneErrorCorrection = false;
    msg.reset();
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType tempresult = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tempresult);
      //errorBlk.print();
      //msg.print();
    }
  }
  return result; 
}

CRCECC256::CRCECC256(int _crcbits):
                   ECC(ONCHIPx4_IECC16_BL16_256_Overfetch){
  if (_crcbits==8){
    onchip_codec = new b8CRC("BitFlip 8bCRC",136, _crcbits, true);
    setInDRAM(2);
  }else if (_crcbits==16){
    onchip_codec = new b16CRC("BitFlip 16bCRC",144, _crcbits, true);
    setInDRAM(2);
  }

  crcbit = _crcbits;
  
  setInDRAMDown(1);

  setBitN(256+_crcbits); // 136 for 8 bit , 144 for 16 bit
}

ErrorType CRCECC256::decodeInternal(FaultDomain *fd, CacheLine &errorBlk){
  ECCWord msg(272,256);
  ECCWord decoded(272,256);
  ErrorType result = NE;
  bool oneErrorCorrection = false;
  CacheLine errorCpy(errorBlk.getChipWidth(),errorBlk.getChannelWidth(),errorBlk.getBeatHeight());
  //Block errorCpy(errorBlk.getBitN());
  ECCLayout layout;
  if(crcbit == 8){
    ECCWord msg(264, 256);
    ECCWord decoded(264, 256);
    layout = ONCHIPx4_IECC8_BL16_256_Overfetch;
  }else if(crcbit == 16){
    ECCWord msg(272, 256);
    ECCWord decoded(272, 256);
    layout = ONCHIPx4_IECC16_BL16_256_Overfetch;
  }
  

  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    // msg.extract(&errorBlk, ONCHIPx4_2, i, errorBlk.getChannelWidth());
    oneErrorCorrection = false;
    msg.reset();
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth());
    if (!msg.isZero()) {
      ErrorType tempresult = onchip_codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tempresult);
      //errorBlk.print();
      //msg.print();
    }
  }
  return result; 
}



LargeRS::LargeRS(int _maxPin)
                     :ECC(ONCHIPx4_IECC32_BL16_256_Overfetch, false, false, 0),maxPin(_maxPin)
{
  codec = new RS<2, 16>("3.5PC\t16\t4\t", 18, 2, 1,1);
  setBitN(256);  
  ErasureLocation = new std::list<int>;
  setInDRAM(SingleSingleSingleOn18Symbol);
  setInDRAMDown(Double);
}

ErrorType LargeRS::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord msg = {codec->getBitN(), codec->getBitK()};
  ECCWord decoded = {codec->getBitN(), codec->getBitK()};
  correctedPosSet.clear();
  bool synError;
  bool parity, parityError;
  ErrorType result, tmp_result;
  result  =NE;

  if (errorBlk.isZero()) {
    return NE;
  }
  for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
    msg.reset();
    msg.extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
    if (!msg.isZero()) {
      //errorBlk.print();
      //msg.print();
      tmp_result = codec->decode(&msg, &decoded, &correctedPosSet);
      result = worseErrorType(result,tmp_result);
    }
  }
  return result;
}

unsigned long long LargeRS::getInitialRetiredBlkCount(FaultDomain *fd,
                                                        double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    double goodSymProb = pow(1 - cellFaultRate, fd->getBeatHeight());
    double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) +
                         fd->getChannelWidth() *
                             pow(goodSymProb, fd->getChannelWidth() - 1) *
                             (1 - goodSymProb);

    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 64;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    //unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long goodBlkCount = totalBlkCount * goodBlkProb;
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}

LargeRS_aiecc::LargeRS_aiecc(int _codecType)
                     :ECC(ONCHIPx4_IECC32_BL16_256_Overfetch, false, false, 0),codecType(_codecType)
{
  ErasureLocation = new std::list<int>;
  if (codecType == 0){
    // RS8(18,16)
    codec = new RS<2, 8>("3.5PC\t16\t4\t", 18, 2, 1);
    setBitN(144);
    setInDRAM(Double);
    setInDRAMDown(Single);
  } else if(codecType == 1){
    // RS8(36,32)
    codec = new RS<2, 8>("3.5PC\t16\t4\t", 36, 4, 2);
    setBitN(288); 
    setInDRAM(Triple);
    setInDRAMDown(Single);
  } else if(codecType == 2){
    // RS16(18,16)
    codec = new RS<2, 16>("3.5PC\t16\t4\t", 18, 2, 1);
    setBitN(288);  
    setInDRAM(Double);
    setInDRAMDown(Single);
  }
}

ErrorType LargeRS_aiecc::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord *msg;
  ECCWord *decoded;  
  msg = new ECCWord(288, 256);
  decoded = new ECCWord(288, 256);
  correctedPosSet.clear();
  bool synError;
  bool parity, parityError;
  ErrorType result, tmp_result;
  result  =NE;
  if (errorBlk.isZero()) {
    return NE;
  }
  if (codecType == 0){
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ECCWord msg1 = {144, 128};
        ECCWord msg2 = {144, 128};
        ECCWord decoded1 = {144, 128};
        ECCWord decoded2 = {144, 128};
        msg1.copyfromN_strideM(msg,0,2);
        msg2.copyfromN_strideM(msg,1,2);
        tmp_result = codec->decode(&msg1, &decoded1, &correctedPosSet);
        if (!decoded1.isZero(0,8)){
          tmp_result = DUE;
        }
        result = worseErrorType(tmp_result, result);

        tmp_result = codec->decode(&msg2, &decoded2, &correctedPosSet);
        if (!decoded2.isZero(0,8)){
          tmp_result = DUE;
        }
        result = worseErrorType(tmp_result, result);
      }
    }
  } else {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        //errorBlk.print();
        //msg.print();
        tmp_result = codec->decode(msg, decoded, &correctedPosSet);
        if (!decoded->isZero(0,16)){
          tmp_result = DUE;
        }
        result = worseErrorType(result,tmp_result);
      }
    }
  }
  delete msg;
  delete decoded;
  return result;
}

unsigned long long LargeRS_aiecc::getInitialRetiredBlkCount(FaultDomain *fd,
                                                        double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    double goodSymProb = pow(1 - cellFaultRate, fd->getBeatHeight());
    double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) +
                         fd->getChannelWidth() *
                             pow(goodSymProb, fd->getChannelWidth() - 1) *
                             (1 - goodSymProb);

    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 64;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    //unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long goodBlkCount = totalBlkCount * goodBlkProb;
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}


LargeRS_HBM::LargeRS_HBM(int _codecType)
                     :ECC(ONCHIPx4_IECC32_BL16_256_Overfetch, false, false, 0),codecType(_codecType)
{
  ErasureLocation = new std::list<int>;
  if (codecType == 0){
    // two RS8(18,16)
    codec = new RS<2, 8>("3.5PC\t16\t4\t", 18, 2, 1);
    setBitN(144);
    setInDRAM(Double);
    setInDRAMDown(Single);
  } else if(codecType == 1){
    // RS8(36,32)
    codec = new RS<2, 8>("3.5PC\t16\t4\t", 36, 4, 2);
    setBitN(288); 
    setInDRAM(Triple);
    setInDRAMDown(Single);
  } else if(codecType == 2){
    // RS16(18,16)
    codec = new RS<2, 16>("3.5PC\t16\t4\t", 18, 2, 1);
    setBitN(288);  
    setInDRAM(Double);
    setInDRAMDown(Single);
  }
}

ErrorType LargeRS_HBM::decodeInternal(FaultDomain *fd, CacheLine &errorBlk) {
  ECCWord *msg;
  ECCWord *decoded;  
  msg = new ECCWord(288, 256);
  decoded = new ECCWord(288, 256);
  correctedPosSet.clear();
  bool synError;
  bool parity, parityError;
  ErrorType result, tmp_result;
  result  =NE;
  if (errorBlk.isZero()) {
    return NE;
  }
  if (codecType == 0){
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        ECCWord msg1 = {144, 128};
        ECCWord msg2 = {144, 128};
        ECCWord decoded1 = {144, 128};
        ECCWord decoded2 = {144, 128};
        msg1.copyfromN(msg,0);
        msg2.copyfromN(msg,144);
        tmp_result = codec->decode(&msg1, &decoded1, &correctedPosSet);
        if (fd->HBM_setup & tmp_result == DUE){
          if(errorBlk.errorDQ >= fd->getChipWidth()){
            tmp_result = SDC;
          }
        }
        result = worseErrorType(tmp_result, result);
        if (fd->HBM_setup & tmp_result == DUE){
          if(errorBlk.errorDQ >= fd->getChipWidth()){
            tmp_result = SDC;
          }
        }
        tmp_result = codec->decode(&msg2, &decoded2, &correctedPosSet);
        result = worseErrorType(tmp_result, result);
      }
    }
  } else {
    for (int i = errorBlk.getChipCount() - 1; i >= 0; i--) {
      msg->reset();
      msg->extract(&errorBlk, layout, i, errorBlk.getChannelWidth(), errorBlk.get_messageConfig());
      if (!msg->isZero()) {
        //errorBlk.print();
        //msg.print();
        tmp_result = codec->decode(msg, decoded, &correctedPosSet);
        if (fd->HBM_setup & tmp_result == DUE){
          if(errorBlk.errorDQ >= fd->getChipWidth()){
            tmp_result = SDC;
          }
        }
        result = worseErrorType(result,tmp_result);
      }
    }
  } 

  delete msg;
  delete decoded;
  return result;
}

unsigned long long LargeRS_HBM::getInitialRetiredBlkCount(FaultDomain *fd,
                                                        double rate) {
  double cellFaultRate = rate;
  if (cellFaultRate == 0) {
    return 0;
  } else {
    double goodSymProb = pow(1 - cellFaultRate, fd->getBeatHeight());
    double goodBlkProb = pow(goodSymProb, fd->getChannelWidth()) +
                         fd->getChannelWidth() *
                             pow(goodSymProb, fd->getChannelWidth() - 1) *
                             (1 - goodSymProb);

    unsigned long long totalBlkCount =
        ((MRANK_MASK ^ DEFAULT_MASK) + 1) * fd->getChipWidth() / 64;
    std::binomial_distribution<int> distribution(totalBlkCount, goodBlkProb);
    //unsigned long long goodBlkCount = distribution(randomGenerator);
    unsigned long long goodBlkCount = totalBlkCount * goodBlkProb;
    unsigned long long badBlkCount = totalBlkCount - goodBlkCount;
    return badBlkCount;
  }
}