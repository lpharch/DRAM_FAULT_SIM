
/**
 * @file: FlipCorrection
 * @author: Jeageun Jung
 */
#ifndef __ONDIELARGE_H__
#define __ONDIELARGE_H__

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
   b8CRC(const char *name, int _bitN, int _bitR);
   void encode(Block *data, ECCWord *encoded);
   ErrorType decode(ECCWord *msg, ECCWord *decoded,
                    std::set<int> *correctedPos = NULL);
  
  protected:
   unsigned char correctionTable[256];
};

class b16CRC: public Codec {
  public:
   b16CRC(const char *name, int _bitN, int _bitR);
   void encode(Block *data, ECCWord *encoded);
   ErrorType decode(ECCWord *msg, ECCWord *decoded,
                    std::set<int> *correctedPos = NULL);
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



#endif /* __ONDIELARGE_H__ */
