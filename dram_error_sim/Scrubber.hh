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
 * @file: Scrubber.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * scrubbing
 */

#ifndef __SCRUBBER_HH__
#define __SCRUBBER_HH__

#include "DomainGroup.hh"

/** @class Scrubber
 * \brief Parent scrubber class
 */
class Scrubber {
 public:
  Scrubber() {}

 public:
  virtual void scrub(DomainGroup *dg, double hrs) = 0;
};

/** @class NoScrubber
 * \brief No scrubber class
 */
class NoScrubber : public Scrubber {
 public:
  NoScrubber() {}

  void scrub(DomainGroup *dg, double hrs) { return; }
};

/** @class PeriodicScrubber
 * \brief Periodic scrubber class
 */
class PeriodicScrubber : public Scrubber {
 public:
  PeriodicScrubber(double _period) : period(_period), prevScrubSection(0) {}
  //!< scrubbing at domain group "dg" after "hrs"
  void scrub(DomainGroup *dg, double hrs) {
    int curScrubSection = (int)hrs / period;
    if (curScrubSection != prevScrubSection) {
      dg->scrub();
      prevScrubSection = curScrubSection;
    }
  }

 protected:
  double period;         //!< scrubbing period
  int prevScrubSection;  //!< previously scrubbed section
};

#endif /* __SCRUBBER_HH__ */
