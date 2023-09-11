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
 * @file: gf.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * GF declaration
 */

#ifndef __GF_CC__
#define __GF_CC__

#include "gf.hh"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include "message.hh"
#include "util.hh"

using namespace std;

//------------------------------------------------------------------------------
template <int p, int m>
GF<p, m>::GF() {
  assert(p == 2);  // supports GF with characteristic of 2 only

  maxIndex = (1 << m) - 1;
  primitivePoly = pickPrimitivePoly();

  index2polyTable[0] = POLY(0x1u);
  for (INDEX i = 1; i < maxIndex; i++) {
    index2polyTable[i] = index2polyTable[i - 1] << 1;
    if (index2polyTable[i] > maxIndex) {  // overflown
      index2polyTable[i] ^= primitivePoly;
    }
  }
  index2polyTable[maxIndex] = 0;
  for (INDEX i = 0; i <= maxIndex; i++) {
    poly2indexTable[index2polyTable[i]] = i;
  }

  // print();
}

template <int p, int m>
POLY GF<p, m>::pickPrimitivePoly() {
  // From page 30 of "Code design for dependable systems" / Eiji Fujiwara /
  // Wiley
  if (m == 1) {
    return POLY(0x3u);  // x + 1 = 0  -> {11} -> 0x3
  } else if (m == 2) {
    return POLY(0x7u);  // x^2 + x + 1 = 0  -> {111} -> 0x7
  } else if (m == 3) {
    return POLY(0xBu);  // x^3 + x + 1 = 0  -> {1011} -> 0xb
  } else if (m == 4) {
    return POLY(0x13u);  // x^4 + x + 1 = 0  -> {1_0011} -> 0x13
  } else if (m == 5) {
    return POLY(0x25u);  // x^5 + x^2 + 1 = 0  -> {10_0101} -> 0x25
  } else if (m == 6) {
    return POLY(0x43u);  // x^6 + x + 1 = 0  -> {100_0011} -> 0x43
  } else if (m == 7) {
    return POLY(0x83u);  // x^7 + x + 1 = 0  -> {1000_0011} -> 0x83
  } else if (m == 8) {
    return POLY(0x11Du);  // x^8 + x^4 + x^3 + x^2 + 1 -> {1 0001 1101} -> 0x11d
  } else if (m == 9) {
    return POLY(0x211u);  // x^9 + x^4 + 1 = 0  -> {10_0001_0001} -> 0x211
  } else if (m == 10) {
    return POLY(0x409u);  // x^10 + x^3 + 1 = 0  -> {100_0000_1001} -> 0x409
  } else if (m == 16) {
    return POLY(0x1100Bu);  // x^16 + x^12 + x^3 + x + 1 = 0  ->
                            // {1_0001_0000_0000_1011} -> 0x1100B
  } else {
    assert(0 && "Unsupported m size");
    return POLY(0x0u);
  }
}

template <int p, int m>
POLY GF<p, m>::genMinimalPoly(int alphaPower) {
  assert((alphaPower % 2) != 0);

  /*
  set<int> conjugates;
  int rootCount = 0;

  // find conjugate roots
  for (int i=0; i<m; i++) {
      int alphaIndex = (alphaPower<<i) % ((1<<m) - 1);    // alpha^((1<<m)-1) is
  1
      set<int>::iterator it = conjugates.find(alphaIndex);
      if (it!=conjugates.end()) {   // already in the list
          break;
      } else {
          printf("%d, ", alphaIndex);
          conjugates.insert(alphaIndex);
          rootCount++;
      }
  }
  printf("\n");

  printf("%dx^%d\n", 1, rootCount);
  POLY coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
      coeff ^= index2poly(*it);
  }
  printf("%dx^%d\n", coeff, rootCount-1);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
      for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
          if (*it > *it2) {
              int newIndex = ((*it) + (*it2)) % maxIndex;
              coeff ^= index2poly(newIndex);
          }
      }
  }
  printf("%dx^%d\n", coeff, rootCount-2);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
      for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
          for (set<int>::iterator it3 = conjugates.begin(); it3 !=
  conjugates.end(); ++it3) {
              if ((*it > *it2) && (*it2 > *it3)) {
                  int newIndex = ((*it) + (*it2) + *(it3)) % maxIndex;
                  coeff ^= index2poly(newIndex);
              }
          }
      }
  }
  printf("%dx^%d\n", coeff, rootCount-3);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
      for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
          for (set<int>::iterator it3 = conjugates.begin(); it3 !=
  conjugates.end(); ++it3) {
          for (set<int>::iterator it4 = conjugates.begin(); it4 !=
  conjugates.end(); ++it4) {
              if ((*it > *it2) && (*it2 > *it3) && (*it3 > *it4)) {
                  int newIndex = ((*it) + (*it2) + *(it3) + *(it4)) % maxIndex;
                  coeff ^= index2poly(newIndex);
              }
          }
          }
      }
  }
  printf("%dx^%d\n", coeff, rootCount-4);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
  for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
  for (set<int>::iterator it3 = conjugates.begin(); it3 != conjugates.end();
  ++it3) {
  for (set<int>::iterator it4 = conjugates.begin(); it4 != conjugates.end();
  ++it4) {
  for (set<int>::iterator it5 = conjugates.begin(); it5 != conjugates.end();
  ++it5) {
      if ((*it > *it2) && (*it2 > *it3) && (*it3 > *it4) && (*it4 > *it5)) {
          int newIndex = ((*it) + (*it2) + *(it3) + *(it4) + *(it5)) % maxIndex;
          coeff ^= index2poly(newIndex);
      }
  }
  }
  }
  }
  }
  printf("%dx^%d\n", coeff, rootCount-5);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
  for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
  for (set<int>::iterator it3 = conjugates.begin(); it3 != conjugates.end();
  ++it3) {
  for (set<int>::iterator it4 = conjugates.begin(); it4 != conjugates.end();
  ++it4) {
  for (set<int>::iterator it5 = conjugates.begin(); it5 != conjugates.end();
  ++it5) {
  for (set<int>::iterator it6 = conjugates.begin(); it6 != conjugates.end();
  ++it6) {
      if ((*it > *it2) && (*it2 > *it3) && (*it3 > *it4) && (*it4 > *it5) &&
  (*it5 > *it6)) {
          int newIndex = ((*it) + (*it2) + *(it3) + *(it4) + *(it5) + *(it6)) %
  maxIndex;
          coeff ^= index2poly(newIndex);
      }
  }
  }
  }
  }
  }
  }
  printf("%dx^%d\n", coeff, rootCount-6);

  coeff = 0;
  for (set<int>::iterator it = conjugates.begin(); it != conjugates.end(); ++it)
  {
  for (set<int>::iterator it2 = conjugates.begin(); it2 != conjugates.end();
  ++it2) {
  for (set<int>::iterator it3 = conjugates.begin(); it3 != conjugates.end();
  ++it3) {
  for (set<int>::iterator it4 = conjugates.begin(); it4 != conjugates.end();
  ++it4) {
  for (set<int>::iterator it5 = conjugates.begin(); it5 != conjugates.end();
  ++it5) {
  for (set<int>::iterator it6 = conjugates.begin(); it6 != conjugates.end();
  ++it6) {
  for (set<int>::iterator it7 = conjugates.begin(); it7 != conjugates.end();
  ++it7) {
      if ((*it > *it2) && (*it2 > *it3) && (*it3 > *it4) && (*it4 > *it5) &&
  (*it5 > *it6) && (*it6 > *it7)) {
          int newIndex = ((*it) + (*it2) + *(it3) + *(it4) + *(it5) + *(it6) +
  *(it7)) % maxIndex;
          coeff ^= index2poly(newIndex);
      }
  }
  }
  }
  }
  }
  }
  }
  printf("%dx^%d\n", coeff, rootCount-7);
  */

  if (m == 3) {
    if (alphaPower == 1) {
      return POLY(0xBu);  // x^3 + x + 1
    } else if (alphaPower == 3) {
      return POLY(0xDu);  // x^3 + x^2 + 1
    }
  } else if (m == 4) {
    if (alphaPower == 1) {
      return POLY(0x13u);  // x^4 + x + 1
    } else if (alphaPower == 3) {
      return POLY(0x1Fu);  // x^4 + x^3 + x^2 + x + 1
    } else if (alphaPower == 5) {
      return POLY(0x7u);  // x^2 + x + 1
    } else if (alphaPower == 7) {
      return POLY(0x19u);  // x^4 + x^3 + 1
    }
  } else if (m == 5) {
    if (alphaPower == 1) {
      return POLY(0x25u);  // x^5 + x^2 + 1
    } else if (alphaPower == 3) {
      // root: a^3, a^6, a^12, a^24, a^17
      return POLY(0x3Du);  // x^5 + x^4 + x^3 + x^2 + 1
    } else if (alphaPower == 5) {
      // root: a^5, a^10, a^20, a^9, a^18
      return POLY(0x37u);  // x^5 + x^4 + x^2 + x + 1
    } else if (alphaPower == 7) {
      // root: a^7, a^14, a^28, a^25, a^19
      return POLY(0x2Fu);  // x^5 + x^3 + x^2 + x + 1
    } else if (alphaPower == 9) {
      // root: a^9, a^18, a^5, a^10, a^20
      return POLY(0x37u);  // x^5 + x^4 + x^2 + x + 1
    }
  } else if (m == 6) {
    if (alphaPower == 1) {
      return POLY(0x43u);  // x^6 + x + 1
    } else if (alphaPower == 3) {
      // root: a^3, a^6, a^12, a^24, a^48, a^33
      return POLY(0x57u);  // x^6 + x^4 + x^2 + x + 1
    } else if (alphaPower == 5) {
      // root: a^5, a^10, a^20, a^40, a^17, a^34
      return POLY(0x67u);  // x^6 + x^5 + x^2 + x + 1
    } else if (alphaPower == 7) {
      // root: a^7, a^14, a^28, a^56, a^49, a^35
      return POLY(0x49u);  // x^6 + x^3 + 1
    } else if (alphaPower == 9) {
      // root: a^9, a^18, a^36
      return POLY(0xDu);  // x^3 + x^2 + 1
    }
  } else if (m == 7) {
    if (alphaPower == 1) {
      return POLY(0x83u);  // x^7 + x + 1
    } else if (alphaPower == 3) {
      // root: a^3, a^6, a^12, a^24, a^48, a^96, a^65
      return POLY(0xABu);  // x^7 + x^5 + x^3 + x + 1
    } else if (alphaPower == 5) {
      // root: a^5, a^10, a^20, a^40, a^80, a^33, a^66
      return POLY(0x8Fu);  // x^7 + x^3 + x^2 + x + 1
    } else if (alphaPower == 7) {
      // root: a^7, a^14, a^28, a^56, a^112, a^97, a^67
      return POLY(0xFDu);  // x^7 + x^6 + x^5 + x^4 + x^3 + x^2 + 1
    } else if (alphaPower == 9) {
      // root: a^9, a^18, a^36, a^72, a^17, a^34, a^68
      return POLY(0xB9u);  // x^7 + x^5 + x^4 + x^3 + 1
    }
  } else if (m == 8) {
    if (alphaPower == 1) {
      return POLY(0x11Du);  // x^8 + x^4 + x^3 + x^2 + 1
    } else if (alphaPower == 3) {
      // root: a^3, a^6, a^12, a^24, a^48, a^96, a^192, a^129
      return POLY(0x177u);  // x^8 + x^6 + x^5 + x^4 + x^2 + x + 1
    } else if (alphaPower == 5) {
      // root: a^5, a^10, a^20, a^40, a^80, a^160, a^65, a^130
      return POLY(0x1F3u);  // x^8 + x^7 + x^6 + x^5 + x^4 + x + 1
    } else if (alphaPower == 7) {
      // root: a^7, a^14, a^28, a^56, a^112, a^224, a^193, a^131
      return POLY(0x169u);  // x^8 + x^6 + x^5 + x^3 + 1
    } else if (alphaPower == 9) {
      // root: a^9, a^18, a^36, a^72, a^144, a^33, a^66, a^132
      return POLY(0x1BDu);  // x^8 + x^7 + x^5 + x^4 + x^3 + x^2 + 1
    }
  } else if (m == 16) {
    if (alphaPower == 1) {
      return POLY(0x1100Bu);  // x^16 + x^12 + x^3 + x + 1
    } else {
      assert(0);
    }
  }
}

template <int p, int m>
void GF<p, m>::print() const {
  printf("p = %d, m = %d: p(x) = ", p, m);
  printPoly(primitivePoly, m);
  printf("\n\npower | poly\n");
  for (INDEX i = 0; i <= maxIndex; i++) {
    if (isZeroIndex(i)) {
      printf("-INF  | ");
    } else {
      printf("%4d  | ", i);
    }
    printf("%3x  | ", index2polyTable[i]);
    printPoly(index2polyTable[i], m - 1);
    printf("\n");
  }
  printf("\n");
}

//------------------------------------------------------------------------------
template <int p, int m>
GF<p, m> GFElem<p, m>::gf;

template <int p, int m>
GFElem<p, m> GFElem<p, m>::ZERO = GFElem<p, m>(gf.getZeroIndex());

//------------------------------------------------------------------------------
template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator=(const GFElem& rhs) {
  if (this == &rhs) return *this;
  indexValue = rhs.indexValue;
  return *this;
}

template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator+=(const GFElem& rhs) {
  indexValue =
      gf.poly2index(gf.index2poly(indexValue) ^ gf.index2poly(rhs.indexValue));
  return *this;
}

template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator-=(const GFElem& rhs) {
  *this += rhs;
  return *this;
}

template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator*=(const GFElem& rhs) {
  if (isZero() || rhs.isZero())[[likely]] {
    indexValue = gf.getZeroIndex();
  } else [[unlikely]] {
    indexValue += rhs.indexValue;
    indexValue = indexValue < ((1 << m) - 1) ? indexValue : indexValue % ((1 << m) - 1);
  }
  return *this;
}

template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator/=(const GFElem& rhs) {
  if (isZero() || rhs.isZero()) {
    indexValue = gf.getZeroIndex();
  } else {
    indexValue = indexValue - rhs.indexValue + ((1 << m) - 1);
    indexValue = indexValue < ((1 << m) - 1) ? indexValue : indexValue % ((1 << m) - 1);
  }
  return *this;
}

template <int p, int m>
GFElem<p, m>& GFElem<p, m>::operator^=(const int rhs) {
  if (!isZero()) {
    indexValue *= rhs;
    indexValue = indexValue < ((1 << m) - 1) ? indexValue : indexValue % ((1 << m) - 1);
  }
  return *this;
}

template <int p, int m>
bool GFElem<p, m>::operator==(const GFElem& rhs) const {
  return (indexValue == rhs.indexValue);
}

template <int p, int m>
bool GFElem<p, m>::operator!=(const GFElem& rhs) const {
  return (indexValue != rhs.indexValue);
}

template <int p, int m>
GFElem<p, m> GFElem<p, m>::operator+(const GFElem& rhs) {
  GFElem<p, m> result(indexValue);
  result += rhs;
  return result;
}

template <int p, int m>
GFElem<p, m> GFElem<p, m>::operator-(const GFElem& rhs) {
  GFElem<p, m> result(indexValue);
  result -= rhs;
  return result;
}

template <int p, int m>
GFElem<p, m> GFElem<p, m>::operator*(const GFElem& rhs) {
  GFElem<p, m> result(indexValue);
  result *= rhs;
  return result;
}

template <int p, int m>
GFElem<p, m> GFElem<p, m>::operator/(const GFElem& rhs) {
  GFElem<p, m> result(indexValue);
  result /= rhs;
  return result;
}

template <int p, int m>
GFElem<p, m> GFElem<p, m>::operator^(const int rhs) {
  GFElem<p, m> result(indexValue);
  result ^= rhs;
  return result;
}

template <int p,int m>
bool GFElem<p,m>::isZero() const { 
  return indexValue == gf.getZeroIndex(); 
}

//------------------------------------------------------------------------------
template <int p, int m>
GFPoly<p, m>::GFPoly(int _degree) {
  degree = _degree;
  coeffArr = new GFElem<p, m>[degree + 1];
  for (int i = 0; i <= degree; i++) {
    coeffArr[i] = GFElem<p, m>::ZERO;
  }
}

template <int p, int m>
GFPoly<p, m>::GFPoly(Block* msg) {
  degree = (msg->getBitN() + (m - 1)) / m;
  coeffArr = new GFElem<p, m>[degree + 1];
  for (int i = degree; i >= 0; i--) {
    coeffArr[i].setValue(msg->getSymbol(m, i));
  }
}

template <int p, int m>
GFPoly<p, m>::~GFPoly() {
  delete[] coeffArr;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator=(const GFPoly& rhs) {
  if (rhs.degree > degree) {  // allocate new array and deallocate existing
    delete[] coeffArr;
    coeffArr = new GFElem<p, m>[rhs.degree + 1];
  }
  degree = rhs.degree;
  for (int i = degree; i >= 0; i--) {
    coeffArr[i] = rhs.coeffArr[i];
  }
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator+=(const GFPoly& rhs) {
  int maxDegree = (degree > rhs.degree) ? degree : rhs.degree;
  if (maxDegree > degree) {
    GFElem<p, m>* newCoeffArr = new GFElem<p, m>[maxDegree + 1];
    for (int i = degree; i >= 0; i--) {
      newCoeffArr[i] = coeffArr[i];
    }
    delete[] coeffArr;
    coeffArr = newCoeffArr;
  }

  for (int i = maxDegree; i >= 0; i--) {
    if (i <= rhs.degree) coeffArr[i] += rhs.coeffArr[i];
  }
  degree = maxDegree;
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator-=(const GFPoly& rhs) {
  return (*this += rhs);
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator*=(const GFPoly<p, m>& rhs) {
  int maxDegree =
      (degree > (degree + rhs.degree)) ? degree : (degree + rhs.degree);
  GFElem<p, m>* newCoeffArr = new GFElem<p, m>[maxDegree + 1];
  for (int i = degree; i >= 0; i--) {
    for (int j = rhs.degree; j >= 0; j--) {
      newCoeffArr[i + j] += (coeffArr[i] * rhs.coeffArr[j]);
    }
  }
  delete[] coeffArr;
  coeffArr = newCoeffArr;
  degree = maxDegree;
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator*=(const GFElem<p, m>& rhs) {
  for (int i = degree; i >= 0; i--) {
    coeffArr[i] *= rhs;
  }
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator/=(const GFPoly<p, m>& rhs) {
  int quotientDegree = degree - rhs.degree;
  int remainderDegree = rhs.degree - 1;

  GFElem<p, m>* quotientArr = new GFElem<p, m>[quotientDegree + 1];
  GFElem<p, m>* remainderArr = new GFElem<p, m>[remainderDegree + 1];

  for (int i = degree; i >= 0; i++) {
    remainderArr[i] = coeffArr[i];
  }
  for (int i = degree; i >= rhs.degree; i--) {
    quotientArr[i - rhs.degree] = remainderArr[i] / rhs.coeffArr[rhs.degree];
    for (int j = 0; j <= rhs.degree; j++) {
      remainderArr[i - j] -= remainderArr[i] * rhs.coeffArr[i - j];
    }
  }

  delete[] coeffArr;
  delete[] remainderArr;
  coeffArr = quotientArr;
  degree = quotientDegree;
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator/=(const GFElem<p, m>& rhs) {
  for (int i = degree; i >= 0; i--) {
    coeffArr[i] /= rhs;
  }
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator%=(const GFPoly<p, m>& rhs) {
  int quotientDegree = degree - rhs.degree;
  int remainderDegree = rhs.degree - 1;

  GFElem<p, m>* quotientArr = new GFElem<p, m>[quotientDegree + 1];
  GFElem<p, m>* remainderArr = new GFElem<p, m>[remainderDegree + 1];

  for (int i = degree; i >= 0; i++) {
    remainderArr[i] = coeffArr[i];
  }
  for (int i = degree; i >= rhs.degree; i--) {
    quotientArr[i - rhs.degree] = remainderArr[i] / rhs.coeffArr[rhs.degree];
    for (int j = 0; j <= rhs.degree; j++) {
      remainderArr[i - j] -= remainderArr[i] * rhs.coeffArr[i - j];
    }
  }

  delete[] coeffArr;
  delete[] quotientArr;
  coeffArr = remainderArr;
  degree = remainderDegree;
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator^=(const int rhs) {
  for (int i = degree; i >= 0; i--) {
    coeffArr[i] ^= rhs;
  }
  return *this;
}

template <int p, int m>
GFPoly<p, m>& GFPoly<p, m>::operator<<=(const int rhs) {
  int maxDegree = degree + rhs;
  GFElem<p, m>* newCoeffArr = new GFElem<p, m>[maxDegree + 1];
  for (int i = degree; i >= 0; i--) {
    newCoeffArr[i + rhs] = coeffArr[i];
  }
  for (int i = rhs - 1; i >= 0; i--) {
    newCoeffArr[i] = GFElem<p, m>::ZERO;
  }
  delete[] coeffArr;
  coeffArr = newCoeffArr;
  degree = maxDegree;
  return *this;
}

template <int p, int m>
bool GFPoly<p, m>::operator==(const GFPoly& rhs) const {
  if (degree != rhs.degree) return false;
  for (int i = 0; i <= degree; i++) {
    if (coeffArr[i] != rhs.coeffArr[i]) {
      return false;
    }
  }
  return true;
}

template <int p, int m>
bool GFPoly<p, m>::operator!=(const GFPoly& rhs) const {
  if (degree != rhs.degree) return true;
  for (int i = 0; i <= degree; i++) {
    if (coeffArr[i] != rhs.coeffArr[i]) {
      return true;
    }
  }
  return false;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator+(const GFPoly& rhs) {
  int maxDegree = (degree > rhs.degree) ? degree : rhs.degree;
  GFPoly<p, m> result(maxDegree);
  result = *this;
  result += rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator-(const GFPoly& rhs) {
  int maxDegree = (degree > rhs.degree) ? degree : rhs.degree;
  GFPoly<p, m> result(maxDegree);
  result = *this;
  result -= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator*(const GFPoly<p, m>& rhs) {
  GFPoly<p, m> result(degree + rhs.degree);
  result = *this;
  result *= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator*(const GFElem<p, m>& rhs) {
  GFPoly<p, m> result(degree);
  result = *this;
  result *= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator/(const GFPoly<p, m>& rhs) {
  GFPoly<p, m> result(degree - rhs.degree);
  result = *this;
  result /= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator/(const GFElem<p, m>& rhs) {
  GFPoly<p, m> result(degree);
  result = *this;
  result /= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator%(const GFPoly& rhs) {
  GFPoly<p, m> result(rhs.degree - 1);
  result = *this;
  result %= rhs;
  return result;
}

template <int p, int m>
GFPoly<p, m> GFPoly<p, m>::operator<<(const int rhs) {
  GFPoly<p, m> result(degree);
  result = *this;
  result <<= rhs;
  return result;
}

template <int p, int m>
void GFPoly<p, m>::print() const {
  for (int i = degree; i >= 0; i--) {
    if (coeffArr[i].isZero()) {
      if (i != 0) {
        printf("        +");
      }
    } else {
      if (i != 0) {
        printf("a^%d x^%d +", coeffArr[i].getIndexValue(), i);
      } else {
        printf("a^%d", coeffArr[i].getIndexValue());
      }
    }
  }
  printf("\n");
}

#endif /* __GF_CC__ */
