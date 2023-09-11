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
 * @file: Tester.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * DUO declaration. Each class represent one configuration to be tested.
 */
#ifndef __TESTER_HH__
#define __TESTER_HH__

#include <stdio.h>
#include <list>

#include "ECC.hh"
#include "Fault.hh"
#include "VECC.hh"
#include "codec.hh"
#include "common.hh"

#define MAX_YEAR 6

class FaultDomain;
class DomainGroup;
class Scrubber;

/*! \addtogroup Tester
 * \brief Classes related to testing \details Via class TesterSystem, we can run
 * system level reliability simulation (e.g. what is the probability that we
 * would meet at least one DUE/SDC within 5 years for a given system). Via class
 * TesterScenario, we can run simulations for random error patterns from
 * specific faults (e.g. what is SDC probability of SEC-DED against double-bit
 * errors induced from two single bit faults)
 * @{
*/

/** @class Tester
 * \brief General tester class
 */
class Tester {
  /*! @} */
 public:
  Tester() {}
  //! test
  /*! \param dg DomainGroup pointer
          \param ecc ECC pointer
          \param scrubber Scrubber pointer
          \param runCnt Number of random simulations
          \param filePrefix Prefix for output file
          \param faultCount Number of input fault types
          \param faults Faults pointer. Pointing to string array, each element
     of which contains a fault type.
  */
  virtual void test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber, long runCnt,
                    char *filePrefix, int faultCount, std::string *faults) = 0;
};

/** @class TesterSystem
 * \brief Tester class for system level reliability
 */
class TesterSystem : public Tester {
 public:
  TesterSystem() {}

  void test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber, long runCnt,
            char *filePrefix, int faultCount,
            std::string *faults);  //!< test run

  void updateElapsedTime(double hr) { elapsedTime = hr; }
  double getElapsedTime() { return elapsedTime; }
  //! Get activation probability of weak cells; First parameter faults[0] in
  //! Teseter.cc
  double getActiveProbWC() { return actProbWeakCells; }
  void setActiveProbWC(double _rate) { actProbWeakCells = _rate; }

  //! Get ratio of weak cells; Second parameter faults[1] in Teseter.cc
  double getRatioWC() { return ratioWeakCells; }
  void setRatioWC(double _p) { ratioWeakCells = _p; }

  //! Get activation probability of frequently weak cells; Third parameter
  //! faults[2] in Teseter.cc
  double getActiveProbFWC() { return actProbFWCs; }
  void setActiveProbFWC(double _rate) { actProbFWCs = _rate; }

  //! Get ratio of frequently weak cells; Fourth parameter faults[3] in
  //! Teseter.cc
  double getRatioFWC() { return ratioFrequentWeakCells; }
  void setRatioFWC(double _p) { ratioFrequentWeakCells = _p; }

 protected:
  void reset();
  void printSummary(FILE *fd, long runNum);
  double advance(
      double faultRate);    //!< advance random time according to fault rates
  double elapsedTime;       //!< time elapsed so far
  double ratioWeakCells;    //!< ratio of weak cells to whole cells
  double actProbWeakCells;  //!< activation probability faulty cells
  double ratioFrequentWeakCells;  //!< ratio of weak cells to whole cells
  double actProbFWCs;             //!< activation probability faulty cells

 protected:
  // CE + DUE + SDC can be bigger than expected error count?
  long BadCnt = 0;
  long RetireCntYear[MAX_YEAR] = {0};
  long DUECntYear[MAX_YEAR] = {0};
  long SDCCntYear[MAX_YEAR] = {0};
#ifdef DUE_BREAKDOWN
  long DUE_ParityYear[MAX_YEAR] = {0};
  long DUE_NoErasureYear[MAX_YEAR] = {0};
  long SDC_ErasureYear[MAX_YEAR] = {0};
#endif
};

/** @class TesterScenario
 * \brief Tester class for error pattern scenarios
 */
class TesterScenario : public Tester {
 public:
  TesterScenario() {}

  void test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber, long runCnt,
            char *filePrefix, int faultCount, std::string *faults);

 protected:
  void reset();
  void printSummary(FILE *fd, long runNum);

 protected:
  long errorCnt[SDC + 1];
};

#endif /* __TESTER_HH__ */
