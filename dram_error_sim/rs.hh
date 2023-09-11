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
 * @file: rs.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * CODEC declaration (RS)
 */

#ifndef __RS_HH__
#define __RS_HH__

#include <stdint.h>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "codec.hh"
#include "gf.cc"  // template
#include "gf.hh"
#include "linear_codec.cc"  // template
#include "linear_codec.hh"
#include "message.hh"

//#define SYNDROME_TABLE
/**
 * @class RS
 * @brief A general RS code of maximum code length n=2^m-1 and check-symbol
 * length r
 */
template <int p, int m>
class RS : public Codec {  // GF2
 public:
  //! Constructor
  RS(const char *name, int _symN, int _symR, int _symT, int _symB = 4)
      : Codec(name, m * _symN, m * _symR), gPoly(0) {
    symN = _symN;
    symK = _symN - _symR;
    symR = _symR;
    symT = _symT;
    symB = _symB;  // GONG: number of symbols for burst decoding (correction)

    syndrome = new GFElem<p, m>[symR];

    // 1. length check
    // Maximum code length in bits: n_max = 2^m -1;
    // Minimum code length in bits: n_min = 2^(m-1);
    int n_max = (1 << m) - 1;
    int n_min = (1 << (m - 1));

    if (symN > n_max) {
      printf("Invalid n for GF: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
      // assert(0);
    }
    // if (symN < n_min) {
    //    printf("Shortened code: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
    //}

    //! 2. generate generator polynomial
    genGenPoly();

// findHD3NeighborCodewords();

#ifdef SYNDROME_TABLE
    findCorrectableSyndrome();
#endif /* SYNDROME_TABLE */
  }
  ~RS() { delete[] syndrome; }

  // member methods
 public:
  //! encode function
  void encode(Block *data, ECCWord *encoded) {
    GFPoly<p, m> dataPoly(data);
    GFPoly<p, m> remainderPoly(symR);

    dataPoly <<= symR;  // shift by (n-k)
    remainderPoly = dataPoly % gPoly;
  }
  //! decode function
  ErrorType decode(ECCWord *msg, ECCWord *decoded,
                   std::set<int> *correctedPos) {
    // step 1: copy the message data
    decoded->clone(msg);

    // step 2: generate syndrome
    bool synError = genSyndrome(msg);

    // Step 3: if all of syndrome bits are zero, the word can be assumed to be
    // error free
    if (synError) {
#ifdef SYNDROME_TABLE
      assert(m * symR <= 64);
      uint64_t syndromeKey = 0ull;
      for (int i = 0; i < symR; i++) {
        syndromeKey ^= syndrome[i].getValue() << (i * m);
      }
      auto got = correctableSyndromes.find(syndromeKey);
      if (got != correctableSyndromes.end()) {
        uint64_t correctionInfo = got->second;
        while (correctionInfo != 0ull) {
          decoded->invSymbol(m, (correctionInfo >> 8) & 0xFF,
                             correctionInfo & 0xFF);  // position, value
          correctionInfo >>= 16;
        }
        insertCorrectionInfo((correctionInfo >> 8) & 0xFF,
                             correctionInfo & 0xFF);
        if (decoded->isZero()) {
          return CE;
        } else {
          return SDC;
        }
      }

      return DUE;
#else
      GFPoly<p, m> elp(symR);
      GFPoly<p, m> prev_elp(symR);
      GFPoly<p, m> reg(symR);
      int ll = 0;
      int mm = 1;
      GFElem<p, m> prev_discrepancy;
      int count = 0;
      int root[symT];
      int loc[symT];

      elp[0].setPolyValue(1);
      prev_elp[0].setPolyValue(1);
      prev_discrepancy.setPolyValue(1);

      // Berlekamp–Massey algorithm from Wikipedia
      for (int n = 0; n < symR; n++) {
        GFElem<p, m> discrepancy = syndrome[n];
        for (int i = 1; i <= ll; i++) {
          discrepancy += elp[i] * syndrome[n - i];
        }
        if (discrepancy.isZero()) {
          mm++;
        } else if ((2 * ll) <= n) {
          GFPoly<p, m> temp(symR);
          temp = elp;
          elp -= (prev_elp << mm) * discrepancy / prev_discrepancy;
          ll = n + 1 - ll;
          prev_elp = temp;
          prev_discrepancy = discrepancy;
          mm = 1;
        } else {
          elp -= (prev_elp << mm) * discrepancy / prev_discrepancy;
          mm++;
        }
      }
      // printf("L=%d ", ll);
      // elp.print();

      if (ll <= symT) {  // can correct error
        // Chien search
        reg = elp;
        for (int i = 0; i < (1 << m) - 1; i++) {
          GFElem<p, m> q;
          q.setPolyValue(0);
          for (int j = 0; j <= ll; j++) {
            q += reg[j];
            reg[j] *= GFElem<p, m>(j);
          }

          if (q.isZero()) {
            root[count] = i;
            loc[count] = (i != 0) ? ((1 << m) - 1) - i : 0;
            if (loc[count] >= symN) {
              return DUE;
            }
            count++;
            if(count > ll){
              return DUE;
            }
          }
        }
      } else {
        return DUE;
      }

      if (count == ll) {
        // Forney algorithm
        GFElem<p, m> z[symT + 1];
        for (int i = 1; i <= ll; i++) {
          z[i] = syndrome[i - 1] + elp[i];
          for (int j = 1; j < i; j++) {
            z[i] += syndrome[j - 1] * elp[i - j];
          }
        }

        for (int i = 0; i < ll; i++) {
          GFElem<p, m> err;
          err.setIndexValue(0);
          for (int j = 1; j <= ll; j++) {
            err += z[j] * GFElem<p, m>(j * root[i]);
          }
          if (!err.isZero()) {
            GFElem<p, m> q;
            q.setIndexValue(0);
            for (int j = 0; j < ll; j++) {
              if (i != j) {
                GFElem<p, m> temp((loc[j] + root[i]) % ((1 << m) - 1));
                GFElem<p, m> temp1;
                temp1.setIndexValue(0);
                temp += temp1;
                q *= temp;
              }
            }
            err /= q;
          }
          // printf("STG#2: i:%d LOC:%d ERR:%d(%d) (ll%d)\n", i, loc[i],
          // err.getIndexValue(), err.getPolyValue(), ll);
          insertCorrectionInfo(loc[i], err.getIndexValue() + 1);
          decoded->invSymbol(m, loc[i],
                             err.getIndexValue() + 1);  // position, value
          if (correctedPos != NULL) {
            correctedPos->insert(loc[i]);
          }
          // GONG
          // printf("corrtedPos %i\n", loc[i]);
        }
        if (decoded->isZero()) {
          return CE;
        } else {
          return SDC;
        }
      } else {
        return DUE;
      }
#endif /* SYNDROME_TABLE */
    } else {
      if (decoded->isZero()) return CE;
      return SDC;
    }
  }
  //! old burst decoding function. not used anymore.
  ErrorType decodeBurstDUO64bx4(ECCWord *msg, ECCWord *decoded, int burstLength,
                                std::set<int> *correctedPos) {
    // step 1: copy the message data
    decoded->clone(msg);

    // step 2: generate syndrome
    bool synError = genSyndrome(msg);

    // Step 3: if all of syndrome bits are zero, the word can be assumed to be
    // error free
    if (synError) {
      GFElem<p, m> s[symR];
      GFElem<p, m> e[symB];

      // assumption: 4-burst symbol errors aligned to 4 symbol boundaries
      for (int startPos = 0; startPos < 64; startPos += 4) {
        // error location: startPos+0, +1, +2, +3
        // syndromes are shifted by startPos*N
        s[0] = syndrome[0] / GFElem<p, m>((startPos)*1);
        s[1] = syndrome[1] / GFElem<p, m>((startPos)*2);
        s[2] = syndrome[2] / GFElem<p, m>((startPos)*3);
        s[3] = syndrome[3] / GFElem<p, m>((startPos)*4);

        // error values
        // GONG:
        // coefficients can be calculated by gaussian elimination (generating
        // inverse matrix) on Galois Field.
        // A * e = s  -> e = A^(-1) * s
        e[0] = s[0] * GFElem<p, m>(218) + s[1] * GFElem<p, m>(505) +
               s[2] * GFElem<p, m>(503) + s[3] * GFElem<p, m>(212);
        e[1] = s[0] * GFElem<p, m>(504) + s[1] * GFElem<p, m>(225) +
               s[2] * GFElem<p, m>(201) + s[3] * GFElem<p, m>(499);
        e[2] = s[0] * GFElem<p, m>(501) + s[1] * GFElem<p, m>(200) +
               s[2] * GFElem<p, m>(221) + s[3] * GFElem<p, m>(497);
        e[3] = s[0] * GFElem<p, m>(209) + s[1] * GFElem<p, m>(497) +
               s[2] * GFElem<p, m>(496) + s[3] * GFElem<p, m>(206);

        // recalculate syndrom
        GFElem<p, m> s2[8];
        s2[4] = (e[0] + e[1] * GFElem<p, m>(5) + e[2] * GFElem<p, m>(10) +
                 e[3] * GFElem<p, m>(15)) *
                GFElem<p, m>(startPos * 5);
        s2[5] = (e[0] + e[1] * GFElem<p, m>(6) + e[2] * GFElem<p, m>(12) +
                 e[3] * GFElem<p, m>(18)) *
                GFElem<p, m>(startPos * 6);
        s2[6] = (e[0] + e[1] * GFElem<p, m>(7) + e[2] * GFElem<p, m>(14) +
                 e[3] * GFElem<p, m>(21)) *
                GFElem<p, m>(startPos * 7);
        s2[7] = (e[0] + e[1] * GFElem<p, m>(8) + e[2] * GFElem<p, m>(16) +
                 e[3] * GFElem<p, m>(24)) *
                GFElem<p, m>(startPos * 8);

        //
        // printf("-- %2d : %03x %03x %03x %03x : (%03x %03x) (%03x %03x) (%03x
        // %03x)\n",
        //                startPos,
        //                e[3].getIndexValue()+1,
        //                e[2].getIndexValue()+1,
        //                e[1].getIndexValue()+1,
        //                e[0].getIndexValue()+1,
        //                syndrome[4].getIndexValue(),
        //                s2[4].getIndexValue(),
        //                syndrome[5].getIndexValue(),
        //                s2[5].getIndexValue(),
        //                syndrome[6].getIndexValue(),
        //                s2[6].getIndexValue());
        //

        if ((syndrome[4] == s2[4]) && (syndrome[5] == s2[5]) &&
            (syndrome[6] == s2[6])) {
          ECCWord temp = {64 * 9, 57 * 9};
          temp.clone(msg);
          temp.invSymbol(m, startPos, e[0].getIndexValue() + 1);
          temp.invSymbol(m, startPos + 1, e[1].getIndexValue() + 1);
          temp.invSymbol(m, startPos + 2, e[2].getIndexValue() + 1);
          temp.invSymbol(m, startPos + 3, e[3].getIndexValue() + 1);

          bool parity = 0, parityError;
          for (int i = 0; i < 15; i++) {
            parity ^= temp.getBit(36 * i);
          }
          parityError = (parity != temp.getBit(512));

          if (parityError) {
            continue;
          }

          insertCorrectionInfo(startPos, e[0].getIndexValue() + 1);
          insertCorrectionInfo(startPos + 1, e[1].getIndexValue() + 1);
          insertCorrectionInfo(startPos + 2, e[2].getIndexValue() + 1);
          insertCorrectionInfo(startPos + 3, e[3].getIndexValue() + 1);
          decoded->invSymbol(m, startPos, e[0].getIndexValue() + 1);
          decoded->invSymbol(m, startPos + 1, e[1].getIndexValue() + 1);
          decoded->invSymbol(m, startPos + 2, e[2].getIndexValue() + 1);
          decoded->invSymbol(m, startPos + 3, e[3].getIndexValue() + 1);
          // GONG
          // decoded->print();
          if (decoded->isZero()) {
            return CE;
          } else {
            return SDC;
          }
        }
      }
      return DUE;
    } else {
      return SDC;
    }
  }  // end of decodeBurst
  void printSyn() {
    for (int i = 0; i < symR; i++) {
      //printf("Syn[%i]: %2x (%i)\n", i, syndrome[i].getPolyValue(),
      //       syndrome[i].getIndexValue());
    }
  }

  bool genSyndrome(ECCWord *msg) {
    bool synError = false;

    GFElem<p, m> msgElemArr[symN];
    for (int i = 0; i < symN; i++) {
      msgElemArr[i].setValue(msg->getSymbol(m, i));
    }

    for (int i = 0; i < symR; i++) {
      syndrome[i].setValue(0);
      for (int j = 0; j < symN; j++) {
        syndrome[i] +=
            msgElemArr[j] * GFElem<p, m>(((i + 1) * j) % ((1 << m) - 1));
      }
      if (!syndrome[i].isZero()) {
        synError = true;
      }
    }
    return synError;
  }

 private:
  void genGenPoly() {
    gPoly.setCoeff(0, 0);  // 1

    GFPoly<p, m> temp(1);  // x + a^i
    temp.setCoeff(1, 0);
    for (int i = 0; i < symR; i++) {
      temp.setCoeff(0, i + 1);
      gPoly *= temp;
    }

    // printf("Generator poly: %d %d\n", m, symR);
    // gPoly.print();
  }

  void findHD3NeighborCodewords() {
    ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
    ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};

    int position1, value1, position2, value2, position3, value3;
    uint64_t neighbor_cnt = 0;

    for (int position1 = 0; position1 < symN; position1++) {
      for (int value1 = 1; value1 < (1 << m); value1++) {
        for (int position2 = position1 + 1; position2 < symN; position2++) {
          for (int value2 = 1; value2 < (1 << m); value2++) {
            for (int position3 = position2 + 1; position3 < symN; position3++) {
              for (int value3 = 1; value3 < (1 << m); value3++) {
                msg.reset();
                msg.invSymbol(m, position1, value1);
                msg.invSymbol(m, position2, value2);
                msg.invSymbol(m, position3, value3);

                ErrorType result = decode(&msg, &decoded);

                if (result == SDC) {
                  neighbor_cnt++;
                }
              }
            }
          }
        }
      }
    }

    printf("Neighbors (HD=3): %lld\n", neighbor_cnt);
  }
#ifdef SYNDROME_TABLE
  bool findCorrectableSyndrome() {
    // find out correctable syndroms
    ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
    ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};
    for (int position1 = 0; position1 < symN; position1++) {
      for (int value1 = 1; value1 < (1 << m); value1++) {
        msg.reset();
        msg.invSymbol(m, position1, value1);

        ErrorInfo errorInfo = (((uint64_t)position1) << 8) | ((uint64_t)value1);
        // ErrorInfo errorInfo = 0ull;
        // errorInfo.insert(std::make_pair<int, int>((int) position1, (int)
        // value1));
        ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
        if (result == SDC) return false;

        if (symT >= 2) {
          for (int position2 = position1 + 1; position2 < symN; position2++) {
            for (int value2 = 1; value2 < (1 << m); value2++) {
              msg.reset();
              msg.invSymbol(m, position1, value1);
              msg.invSymbol(m, position2, value2);

              ErrorInfo errorInfo =
                  (((uint64_t)position2) << 24) | (((uint64_t)value2) << 16) |
                  (((uint64_t)position1) << 8) | ((uint64_t)value1);
              // ErrorInfo errorInfo;
              // errorInfo.insert(std::make_pair<int, int>((int) position1,
              // (int) value1));
              // errorInfo.insert(std::make_pair<int, int>((int) position2,
              // (int) value2));
              ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
              if (result == SDC) return false;

              if (symT >= 3) {
                printf("--(%d, %d)\n", position1, position2);

                for (int position3 = position2 + 1; position3 < symN;
                     position3++) {
                  for (int value3 = 1; value3 < (1 << m); value3++) {
                    msg.reset();
                    msg.invSymbol(m, position1, value1);
                    msg.invSymbol(m, position2, value2);
                    msg.invSymbol(m, position3, value3);

                    ErrorInfo errorInfo = (((uint64_t)position3) << 40) |
                                          (((uint64_t)value3) << 32) |
                                          (((uint64_t)position2) << 24) |
                                          (((uint64_t)value2) << 16) |
                                          (((uint64_t)position1) << 8) |
                                          ((uint64_t)value1);
                    // ErrorInfo errorInfo;
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position1, (int) value1));
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position2, (int) value2));
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position3, (int) value3));
                    ErrorType result =
                        decodeCorrectable(&msg, &decoded, errorInfo);
                    if (result == SDC) return false;

                    if (symT >= 4) {
                      for (int position4 = position3 + 1; position4 < symN;
                           position4++) {
                        for (int value4 = 1; value4 < (1 << m); value4++) {
                          msg.reset();
                          msg.invSymbol(m, position1, value1);
                          msg.invSymbol(m, position2, value2);
                          msg.invSymbol(m, position3, value3);
                          msg.invSymbol(m, position4, value4);

                          ErrorInfo errorInfo = (((uint64_t)position4) << 56) |
                                                (((uint64_t)value4) << 48) |
                                                (((uint64_t)position3) << 40) |
                                                (((uint64_t)value3) << 32) |
                                                (((uint64_t)position2) << 24) |
                                                (((uint64_t)value2) << 16) |
                                                (((uint64_t)position1) << 8) |
                                                ((uint64_t)value1);
                          // ErrorInfo errorInfo;
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position1, (int) value1));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position2, (int) value2));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position3, (int) value3));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position4, (int) value4));
                          ErrorType result =
                              decodeCorrectable(&msg, &decoded, errorInfo);
                          if (result == SDC) return false;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return true;
  }
  ErrorType decodeCorrectable(ECCWord *msg, ECCWord *decoded,
                              ErrorInfo errorInfo) {
    // step 1: copy the message data
    // decoded->clone(msg);

    // step 2: generate syndrome
    bool synError = genSyndrome(msg);

    // Step 3: if all of syndrome bits are zero, the word can be assumed to be
    // error free
    if (synError) {
      assert(m * symR <= 64);
      uint64_t syndromeKey = 0ull;
      for (int i = 0; i < symR; i++) {
        syndromeKey ^= (syndrome[i].getValue() << (i * m));
      }
      correctableSyndromes.insert(std::make_pair<uint64_t, ErrorInfo>(
          (uint64_t)syndromeKey, (ErrorInfo)errorInfo));
      return CE;
    } else {
      return SDC;
    }
  }
#endif /* SYNDROME_TABLE */

  // member fields
 public:
  int symN, symK, symR, symT, symB;
  GFPoly<p, m> gPoly;
  GFElem<p, m> *syndrome;

  bool *H;  // bit-level parity relationship
#ifdef SYNDROME_TABLE
  std::unordered_map<uint64_t, ErrorInfo> correctableSyndromes;
#endif /* SYNDROME_TABLE */
};

template <int p, int m>
class RS2 : public Codec {  // GF2
                            // Constructor / destructor
 public:
  RS2(const char *name, int _symN, int _symR, int _symT, int _pos1, int _pos2,
      int _pos3)
      : Codec(name, m * _symN, m * _symR),
        gPoly(0),
        pos1(_pos1),
        pos2(_pos2),
        pos3(_pos3) {
    symN = _symN;
    symK = _symN - _symR;
    symR = _symR;
    symT = _symT;

    syndrome = new GFElem<p, m>[symR];

    // 1. length check
    // Maximum code length in bits: n_max = 2^m -1;
    // Minimum code length in bits: n_min = 2^(m-1);
    int n_max = (1 << m) - 1;
    int n_min = (1 << (m - 1));

    if (symN > n_max) {
      printf("Invalid n for GF: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
      // assert(0);
    }
    // if (symN < n_min) {
    //    printf("Shortened code: %d (m=%d) -> %d~%d\n", symN, m, n_min, n_max);
    //}

    // 2. generate generator polynomial
    genGenPoly();

    // findHD3NeighborCodewords();

    // findCorrectableSyndrome();
  }
  ~RS2() { delete[] syndrome; }

  // member methods
 public:
  void encode(Block *data, ECCWord *encoded) {}
  ErrorType decode(ECCWord *msg, ECCWord *decoded,
                   std::set<int> *correctedPos) {
    return SDC;
  }
  virtual bool genSyndrome(ECCWord *msg) {
    bool synError = false;

    GFElem<p, m> msgElemArr[symN];
    for (int i = 0; i < symN; i++) {
      msgElemArr[i].setPolyValue(msg->getSymbol(m, i));
      // if (msg->getSymbol(m, i)!=0) {
      //    printf("%d %d %d\n", msg->getSymbol(m, i), msgElemArr[i].getValue(),
      //    msgElemArr[i].getIndexValue());
      //}
    }

    for (int i = 0; i < symR; i++) {
      syndrome[i].setValue(0);
      for (int j = 0; j < symN; j++) {
        if (i == 0) {
          syndrome[i] +=
              msgElemArr[j] * GFElem<p, m>(((i + pos1) * j) % ((1 << m) - 1));
        } else if (i == 1) {
          syndrome[i] +=
              msgElemArr[j] * GFElem<p, m>(((i + pos2) * j) % ((1 << m) - 1));
        } else if (i == 2) {
          syndrome[i] +=
              msgElemArr[j] * GFElem<p, m>(((i + pos3) * j) % ((1 << m) - 1));
        }
        // syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+1)*j)%((1<<m)-1));
        // syndrome[i] += msgElemArr[j] * GFElem<p, m>(((i+m)*j)%((1<<m)-1));
      }
      if (!syndrome[i].isZero()) {
        synError = true;
      }
    }
    return synError;
  }

  bool findCorrectableSyndrome() {
    // find out correctable syndroms
    ECCWord msg = {Codec::getBitN(), Codec::getBitK()};
    ECCWord decoded = {Codec::getBitN(), Codec::getBitK()};
    for (int position1 = 0; position1 < symN; position1++) {
      for (int value1 = 1; value1 < (1 << m); value1 *= 2) {
        msg.reset();
        msg.invSymbol(m, position1, value1);

        ErrorInfo errorInfo =
            (((uint64_t)position1) << (m)) | ((uint64_t)value1);
        // ErrorInfo errorInfo = 0ull;
        // errorInfo.insert(std::make_pair<int, int>((int) position1, (int)
        // value1));
        ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
        if (result == SDC) return false;

        if (symT >= 2) {
          for (int position2 = position1 + 1; position2 < symN; position2++) {
            for (int value2 = 1; value2 < (1 << m); value2 *= 2) {
              msg.reset();
              msg.invSymbol(m, position1, value1);
              msg.invSymbol(m, position2, value2);

              ErrorInfo errorInfo = (((uint64_t)position2) << (3 * m)) |
                                    (((uint64_t)value2) << (2 * m)) |
                                    (((uint64_t)position1) << (m)) |
                                    ((uint64_t)value1);
              // ErrorInfo errorInfo;
              // errorInfo.insert(std::make_pair<int, int>((int) position1,
              // (int) value1));
              // errorInfo.insert(std::make_pair<int, int>((int) position2,
              // (int) value2));
              ErrorType result = decodeCorrectable(&msg, &decoded, errorInfo);
              if (result == SDC) return false;

              if (symT >= 3) {
                printf("--(%d, %d)\n", position1, position2);

                for (int position3 = position2 + 1; position3 < symN;
                     position3++) {
                  for (int value3 = 1; value3 < (1 << m); value3 *= 2) {
                    msg.reset();
                    msg.invSymbol(m, position1, value1);
                    msg.invSymbol(m, position2, value2);
                    msg.invSymbol(m, position3, value3);

                    ErrorInfo errorInfo = (((uint64_t)position3) << (5 * m)) |
                                          (((uint64_t)value3) << (4 * m)) |
                                          (((uint64_t)position2) << (3 * m)) |
                                          (((uint64_t)value2) << (2 * m)) |
                                          (((uint64_t)position1) << (m)) |
                                          ((uint64_t)value1);
                    // ErrorInfo errorInfo;
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position1, (int) value1));
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position2, (int) value2));
                    // errorInfo.insert(std::make_pair<int, int>((int)
                    // position3, (int) value3));
                    ErrorType result =
                        decodeCorrectable(&msg, &decoded, errorInfo);
                    if (result == SDC) return false;

                    if (symT >= 4) {
                      for (int position4 = position3 + 1; position4 < symN;
                           position4++) {
                        for (int value4 = 1; value4 < (1 << m); value4 *= 2) {
                          msg.reset();
                          msg.invSymbol(m, position1, value1);
                          msg.invSymbol(m, position2, value2);
                          msg.invSymbol(m, position3, value3);
                          msg.invSymbol(m, position4, value4);

                          ErrorInfo errorInfo =
                              (((uint64_t)position4) << (7 * m)) |
                              (((uint64_t)value4) << (6 * m)) |
                              (((uint64_t)position3) << (5 * m)) |
                              (((uint64_t)value3) << (4 * m)) |
                              (((uint64_t)position2) << (3 * m)) |
                              (((uint64_t)value2) << (2 * m)) |
                              (((uint64_t)position1) << (m)) |
                              ((uint64_t)value1);
                          // ErrorInfo errorInfo;
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position1, (int) value1));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position2, (int) value2));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position3, (int) value3));
                          // errorInfo.insert(std::make_pair<int, int>((int)
                          // position4, (int) value4));
                          ErrorType result =
                              decodeCorrectable(&msg, &decoded, errorInfo);
                          if (result == SDC) return false;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return true;
  }

 private:
  void genGenPoly() {
    gPoly.setCoeff(0, 0);  // 1

    GFPoly<p, m> temp(1);  // x + a^i
    temp.setCoeff(1, m);
    for (int i = 0; i < symR; i++) {
      temp.setCoeff(0, i + m + 1);
      gPoly *= temp;
    }

    // printf("Generator poly: %d %d\n", m, symR);
    // gPoly.print();
  }

  ErrorType decodeCorrectable(ECCWord *msg, ECCWord *decoded,
                              ErrorInfo errorInfo) {
    // step 1: copy the message data
    // decoded->clone(msg);

    // step 2: generate syndrome
    bool synError = genSyndrome(msg);

    // Step 3: if all of syndrome bits are zero, the word can be assumed to be
    // error free
    if (synError) {
      assert(m * symR <= 64);
      uint64_t syndromeKey = 0ull;
      for (int i = 0; i < symR; i++) {
        syndromeKey ^= (syndrome[i].getValue() << (i * m));
      }
      auto it = correctableSyndromes.find(syndromeKey);
      if (it != correctableSyndromes.end()) {
        return SDC;
      }
      correctableSyndromes.insert(std::make_pair<uint64_t, ErrorInfo>(
          (uint64_t)syndromeKey, (ErrorInfo)errorInfo));
      return CE;
    } else {
      return SDC;
    }
  }

  // member fields
 public:
  int symN, symK, symR, symT;
  int pos1, pos2, pos3;
  GFPoly<p, m> gPoly;
  GFElem<p, m> *syndrome;
  std::unordered_map<uint64_t, ErrorInfo> correctableSyndromes;
};

/**
 * @class RS_DUAL
 * @brief this decoder can correct error and erasures concurrently
 */
template <int p, int m>
class RS_DUAL : public Codec {
 public:
  //! Constructor
  /*! \param name name of decoder
                  \param _symN number of total symbol in a codeword
                  \param _symR number of redundant symbols
                  \param _symB number of burst error symbols
   */
  RS_DUAL(const char *name, int _symN, int _symR, int _symB)
      : Codec(name, m * _symN, m * _symR) {
    symN = _symN;
    symR = _symR;
    symB = _symB;
    symK = _symN - _symR;
    indexMax = (p << (m - 1)) - 1;
    syndrome = new GFElem<p, m>[symR];
    errata = new GFElem<p, m>[symR + symB];
    errata_raw = new GFElem<p, m>[symR + symB];
    erasure = new GFElem<p, m>[symR + symB];
    location = new int[symR];
    error = new GFElem<p, m>[symR];
    mu = new GFElem<p, m>[symR + symB];      // mu
    la = new GFElem<p, m>[symR + symB];      // lambda
    tmp_mu = new GFElem<p, m>[symR + symB];  // mu
    tmp_la = new GFElem<p, m>[symR + symB];  // lambda
    reg = new GFElem<p, m>[symR+16];
  };
  ~RS_DUAL() {
    delete[] syndrome;
    delete[] errata;
    delete[] errata_raw;
    delete[] erasure;
    delete[] location;
    delete[] error;
    delete[] mu;
    delete[] la;
    delete[] tmp_mu;
    delete[] tmp_la;
    delete[] reg;
  };
  bool hasSDC;
  int symN;
  int symK;
  int symR;
  int _L;    // number of errors including erasure
  int symB;  // number of erasures
  int indexMax;
  int *location;
  GFElem<p, m> *syndrome;
  GFElem<p, m> *erasure;  // erasure polynomial
  GFElem<p, m> *errata;
  GFElem<p, m> *errata_raw;
  GFElem<p, m> *error;
  // variables for BM
  GFElem<p, m> de;       // delta
  GFElem<p, m> ga;       // gamma
  GFElem<p, m> *mu;      // mu
  GFElem<p, m> *la;      // lambda
  GFElem<p, m> *tmp_mu;  // mu
  GFElem<p, m> *tmp_la;  // lambda
  // variables for Chien
  GFElem<p, m> *reg;

  //! decode function
  ErrorType decode(ECCWord *msg, ECCWord *decoded, std::set<int> *correctedPos,
                   std::list<int> *ErasureLocation) {
    // ErrorType decode(ECCWord *msg, ECCWord *decoded, std::list<int>*
    // ErasureLocation) {
    bool synError = genSyndrome(msg);
    if (synError) {
      decoded->clone(msg);
      ErasurePolyGen(ErasureLocation);
      BM();
      if (2 * _L + ErasureLocation->size() > symR) {
        //printf("DUE from BM (_L: %i, E_Location->size(): %i, symR: %i)\n",
        //_L, ErasureLocation->size(), symR);
        // print_errata();
        return DUE;
      }

      if (!Chien()) {
        // printf("DUE from Chien\n");
        return DUE;
      }
      ErrorEval();
      hasSDC = false;
      Correction(decoded, correctedPos, ErasureLocation);
      // Correction(decoded);
    } else {
      // generally it might be SDC if we generated all zero syndromes
      // the below code is for the exception cases where errors exists only in
      // parity (categorized as DUE)
      if (msg->isZero())
        return CE;
      else
        return SDC;
    }

    if (decoded->isZero()) {
      // if(_L+symB>10){
      //	msg->print();
      //	for(int i=0; i<_L+symB; i++){
      //		printf("location[%i]: %i\n", i, indexMax-location[i]);
      //		printf("error index: %i\n", error[i].getIndexValue());
      //	}
      //}
      return CE;
    } else {
      // if(hasSDC) return DUE;
      return SDC;
    }
  }
  bool genSyndrome(ECCWord *msg) {
    bool synError = false;
    GFElem<p, m> received[symN];
    for (int i = 0; i < symN; i++) {
      received[i].setValue(msg->getSymbol(m, i));
      // printf("received[%i]: %i\n", i, received[i].getValue());
    }
    for (int i = 0; i < symR; i++) {
      syndrome[i] = GFElem<p, m>(indexMax);  // reinitialize
      for (int j = 0; j < symK + symR; j++) {
        if (j != symK + symR - 1) {
          syndrome[i] = (syndrome[i] + received[symK + symR - 1 - j]) *
                        (GFElem<p, m>(1) ^ (i + 1));
        } else {
          syndrome[i] += received[0];
        }
      }
      if (!syndrome[i].isZero()) {
        synError = true;
      }
    }
    return synError;
  }
  //! partial error polynomial generation using erasures
  void ErasurePolyGen(std::list<int> *ErasureLocation) {
    if (ErasureLocation->size() == 0) {
      for (int i = 0; i < symR + symB; i++) erasure[i] = GFElem<p, m>(indexMax);
      erasure[0] = GFElem<p, m>(0);
    } else {
      bool first = true;
      for (std::list<int>::iterator it = ErasureLocation->begin();
           it != ErasureLocation->end(); it++) {
        if (first) {
          erasure[0] = GFElem<p, m>(0);
          erasure[1] = GFElem<p, m>(*it);
          for (int i = 2; i < symR + symB; i++)
            erasure[i] = GFElem<p, m>(indexMax);
          first = false;
        } else {
          GFElem<p, m> tmp[symR + symB];
          for (int i = 0; i < symR + symB; i++) {
            tmp[i] = GFElem<p, m>(*it) * erasure[i];
          }
          for (int i = 1; i < symR + symB; i++) {
            erasure[i] += tmp[i - 1];
          }
        }
      }
      // for(int i=0; i<symR+symB; i++) printf("erasure[%i]: %i\n", i,
      // erasure[i]);
    }
  }
  //! a modified BM algorithm (inversion-less version)
  void BM() {
    int l = 0;
    ga = GFElem<p, m>(0);
    for (int i = 0; i < symR + symB; i++) {
      mu[i] = GFElem<p, m>(indexMax);
      la[i] = GFElem<p, m>(indexMax);
    }

    for (int i = 0; i < symR + symB; i++) {
      mu[i] = la[i] = erasure[i];
    }

    for (int k = 1; k < symR; k++) {
      if (k > symR - symB) {
        break;
      }
      // update of delta
      de = GFElem<p, m>(indexMax);
      for (int j = 0; j <= k + symB; j++) {
        de += mu[j] * syndrome[k - j + symB - 1];
      }
      // update of mu
      for (int j = 0; j < symR + symB; j++) {
        if (j == 0)
          tmp_mu[j] = ga * mu[j];
        else
          tmp_mu[j] = ga * mu[j] + de * la[j - 1];
      }
      // update of lambda
      if (de.getIndexValue() != indexMax && 2 * l <= k - 1) {
        for (int j = 0; j < symR + symB; j++) tmp_la[j] = mu[j];
      } else {
        for (int j = 0; j < symR + symB; j++) {
          if (j == 0)
            tmp_la[j] = GFElem<p, m>(indexMax);
          else
            tmp_la[j] = la[j - 1];
        }
      }
      // update of length and gammah
      if (de.getIndexValue() != indexMax && 2 * l <= k - 1) {
        l = k - l;
        ga = de;
      }

      for (int j = 0; j < symR + symB; j++) {
        mu[j] = tmp_mu[j];
        la[j] = tmp_la[j];
      }
    }

    // printf("_L: %i\n", l);
    _L = l;
    for (int i = 0; i < symR + symB; i++) {
      errata[i] = mu[i] / mu[0];
      errata_raw[i] = mu[i];
    }
  }

  //! Chien search algorithm for errata location search
  bool Chien() {
    //printf("L+S: %i\n", _L+symB);
    // GFElem<p,m> reg[_L+symB];
    int cnt = 0;
    for (int i = 0; i < indexMax; i++) {
      GFElem<p, m> sum = GFElem<p, m>(indexMax);
      // for(int j=0; j<=_L+symB; j++){
      for (int j = 0; j <= _L + symB; j++) {
        if (i == 0)
          reg[j] = errata[j + 1];
        else
          reg[j] = reg[j] * (GFElem<p, m>(1) ^ (j + 1));
        sum += reg[j];
      }
      //printf("sum: %i\n", sum.getIndexValue());
      if ((i == 0 || i > indexMax - symN) && sum == GFElem<p, m>(0)) {
        location[cnt] = i;
        //printf("locaton[%i] : %i \n", cnt, indexMax-location[cnt]);
        cnt++;
      }
    }
    if (cnt != _L + symB)
      return false;
    else
      return true;
  }

  //! error value evaluation
  void ErrorEval() {
    GFElem<p, m> poly[symR + _L + symB];
    for (int i = 0; i < symR + _L + symB; i++) poly[i] = GFElem<p, m>(indexMax);
    for (int i = 0; i < symR; i++) {
      for (int j = 0; j <= _L + symB; j++) {
        poly[i + j] += syndrome[i] * errata_raw[j];  // errata_raw[j];
      }
    }

    GFElem<p, m> numer[_L + symB];
    GFElem<p, m> denom[_L + symB];
    for (int i = 0; i < _L + symB; i++) {
      numer[i] = GFElem<p, m>(indexMax);
      denom[i] = GFElem<p, m>(indexMax);
      for (int j = 0; j < symR; j++) {
        numer[i] += poly[j] * (GFElem<p, m>(location[i]) ^ (j + 1));
      }
      for (int j = 0; j < symR; j++) {
        if (j % 2 == 1) {
          denom[i] += errata_raw[j] * (GFElem<p, m>(location[i]) ^ (j - 1)) *
                      GFElem<p, m>(location[i]);
        }
      }
      // printf("num: %i den: %i\t", numer[i].getIndexValue(),
      // denom[i].getIndexValue());
      error[i] = numer[i] / denom[i];
      // printf("error [%i]: %2x\n", i, error[i].getPolyValue());
    }
  }

  //! Correction - XORing error values
  void Correction(ECCWord *decoded, std::set<int> *correctedPos,
                  std::list<int> *ErasureLocation) {
    // void Correction(ECCWord* decoded){
    for (int i = 0; i < _L + symB; i++) {
      int symID = (indexMax - location[i]) % indexMax;
      if (symID >= symN) {
        printf("symID: %i\n", symID);
        assert(0);
      }
      int e_index = error[i].getIndexValue() + 1;
      // assuming only inherent faults corrected as "errors"
      // chip fault erasure locations (or parity-related symbols) should be put
      // in advance
      std::list<int>::iterator it =
          std::find(ErasureLocation->begin(), ErasureLocation->end(), symID);
      if (it == ErasureLocation->end()) {
        // printf("symID: %i\t error(index): %i\n", symID, e_index);
        if (e_index != 0 || e_index != 1 || e_index != 2 || e_index != 4)
          hasSDC = true;
      }
      insertCorrectionInfo(symID, e_index);
      decoded->invSymbol(m, symID, e_index);
      if (correctedPos != NULL) {
        correctedPos->insert(symID);
      }
    }
  }
  void init() {
    for (int i = 0; i < symR; i++) {
      syndrome[i] = GFElem<p, m>(indexMax);
      error[i] = GFElem<p, m>(indexMax);
      reg[i] = GFElem<p, m>(indexMax);
      location[i] = -1;
    }
    for (int i = 0; i < symR + symB; i++) {
      errata[i] = GFElem<p, m>(indexMax);
      errata_raw[i] = GFElem<p, m>(indexMax);
      erasure[i] = GFElem<p, m>(indexMax);
      mu[i] = GFElem<p, m>(indexMax);
      la[i] = GFElem<p, m>(indexMax);
      tmp_mu[i] = GFElem<p, m>(indexMax);
      tmp_la[i] = GFElem<p, m>(indexMax);
    }
  }
  void printSyn() {
    printf("syndrome (index): \n");
    for (int i = 0; i < symR; i++) {
      printf("%i\t %2x (%i)\n", i, syndrome[i].getValue(),
             syndrome[i].getIndexValue());
    }
  }
  void print_errata() {
    printf("errata (index): \n");
    for (int i = 0; i < symR + symB; i++) {
      printf("%i\t%i\n", i, errata[i].getIndexValue());
    }
  }
  // dummy definitions
  void encode(Block *data, ECCWord *encoded) {}
  ErrorType decode(ECCWord *msg, ECCWord *decoded,
                   std::set<int> *correctedPos) {}
};

#endif /* __RS_HH__ */
