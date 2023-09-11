
/**
 * @file: FlipCorrection
 * @author: Jeageun Jung
 */
#ifndef __FLIPCORRECT_H__
#define __FLIPCORRECT_H__

#include "Bamboo.hh"
#include "ECC.hh"
#include "XED.hh"
#include "bch.hh"
#include "prior.hh"
#include "rs.hh"
#include "sec.hh"
#include "DUO.hh"
#include "Huawei.hh"

/**
 * @brief Flip correction for 72,64 CRC code.
 */
class b8CRC: public Codec {
  public:
   b8CRC(const char *name, int _bitN, int _bitR, int enable_1bitfix = true);
   void encode(Block *data, ECCWord *encoded);
   ErrorType decode(ECCWord *msg, ECCWord *decoded,
                    std::set<int> *correctedPos = NULL);
  
  protected:
   unsigned char correctionTable[256];
   bool _enable_1bitfix;
};

class b16CRC: public Codec {
  public:
   b16CRC(const char *name, int _bitN, int _bitR, int enable_1bitfix = true);
   void encode(Block *data, ECCWord *encoded);
   ErrorType decode(ECCWord *msg, ECCWord *decoded,
                    std::set<int> *correctedPos = NULL);
   bool _enable_1bitfix;
};

class Onchip144_128 : public ECC{
  public:
    Onchip144_128();
    ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);

  protected:
    Codec *onchip_codec;
};

/**
 * @brief DUO for 36bit-width channel (x4 device) with on-chip (in-dram) ECC.
 * It is not possible to implement in real case. DUO already exposes the ECC data.
 */
class FlipCRC_DUO : public DUO36bx4 {
 public:
  FlipCRC_DUO(int _maxPin, bool _doPostprocess, bool _doRetire,
                   int _maxRetiredBlkCount);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);


 protected:
  Codec *onchip_codec;
};

class FlipCRC_Bamboo : public Bamboo40b {
 public:
  FlipCRC_Bamboo(int _metadataByte,int _crcbits);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);


 protected:
  Codec *onchip_codec;
  int crcbit;
};

/**
 * @brief CRC for DDR5 BL16, 128bit data protection
 */
class CRCECC128 : public ECC {
 public:
  CRCECC128(int _crcbits);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);


 protected:
  Codec *onchip_codec;
  int crcbit;
};

/**
 * @brief CRC for DDR5 BL16, 256bit data protection
 */
class CRCECC256 : public ECC {
 public:
  CRCECC256(int _crcbits);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);


 protected:
  Codec *onchip_codec;
  int crcbit;
};

/**
 * @brief RS for DDR5 BL16, 256bit data protection
 */
class LargeRS :public ECC{
 public:
  LargeRS(int _maxPin);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  std::list<int> *ErasureLocation;
 protected:
  RS<2, 16> *codec;
  RS<2, 16> *decoder;
  int maxPin;
};

class LargeRS_aiecc :public ECC{
 public:
  LargeRS_aiecc(int _codecType);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  std::list<int> *ErasureLocation;
 protected:
  Codec *codec;
  Codec *decoder;
  int codecType;
};


class LargeRS_HBM :public ECC{
 public:
  LargeRS_HBM(int _codecType);
  ErrorType decodeInternal(FaultDomain *fd, CacheLine &errorBlk);
  unsigned long long getInitialRetiredBlkCount(FaultDomain *fd, double rate);
  std::list<int> *ErasureLocation;
 protected:
  Codec *codec;
  Codec *decoder;
  int codecType;
};


#endif /* __FLIPCORRECT_H__ */
