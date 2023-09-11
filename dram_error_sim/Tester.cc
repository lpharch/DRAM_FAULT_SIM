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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "Config.hh"
#include "DomainGroup.hh"
#include "FaultDomain.hh"
#include "Scrubber.hh"
#include "Tester.hh"
#include "codec.hh"
#include "rs.hh"

volatile bool killflag = false;

//------------------------------------------------------------------------------
char errorName[][10] = {"NE ", "CE ", "DUE ", "SDC"

};

//------------------------------------------------------------------------------
void TesterSystem::reset()
{
  for (int i = 0; i < MAX_YEAR; i++)
  {
    RetireCntYear[i] = 0l;
    DUECntYear[i] = 0l;
    SDCCntYear[i] = 0l;
  }
}

//------------------------------------------------------------------------------
void TesterSystem::printSummary(FILE *fd, long runNum)
{
  fprintf(fd, "After %ld runs\n", runNum);
  fprintf(fd, "Retire\n");
  for (int yr = 1; yr < MAX_YEAR; yr++)
  {
    fprintf(fd, "%.11f\n", (double)RetireCntYear[yr] / runNum);
  }
  fprintf(fd, "DUE\n");
  for (int yr = 1; yr < MAX_YEAR; yr++)
  {
    fprintf(fd, "%.11f\n", (double)DUECntYear[yr] / runNum);
  }
  fprintf(fd, "SDC\n");
  for (int yr = 1; yr < MAX_YEAR; yr++)
  {
    fprintf(fd, "%.11f\n", (double)SDCCntYear[yr] / runNum);
  }

  fflush(fd);
}

//------------------------------------------------------------------------------
double TesterSystem::advance(double faultRate)
{
  double r = (double)rand();
  long long randmax = RAND_MAX;
  double result =
      -log(1.0f - (double)r / (randmax + 1)) / faultRate;
  // printf("- %f\n", result);
  return result;
}
void sigterm_handler(int sig)
{
  printf("Received SIGTERM signal\n");
  killflag = true;
}
//------------------------------------------------------------------------------
void TesterSystem::test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber,
                        long runCnt, char *filePrefix, int faultCount,
                        std::string *faults)
{
  // assert(faultCount<=1);  // either no or 1 inherent fault
  dg->setTester(this, ecc);
  Fault *inherentFault = NULL;
  // create log file
  std::string nameBuffer = std::string(filePrefix) + ".S";
  if (faultCount == 2)
  {
    nameBuffer = nameBuffer + "." + faults[0];
    inherentFault = Fault::genRandomFault(faults[0], NULL);
    // dg->setInherentFault(inherentFault);
    dg->setInherentFault(inherentFault, ecc, true);
  }
  else if (faultCount == 6)
  {
    // faults[0] is initial high prob weak cells
    // faults[1] is high-prob weak cell activation probability (e.g. 0.1)
    // faults[2] is initial high prob freq. weak cells
    // faults[3] is high-prob freq. weak cell activation probability (e.g. 0.1)
    nameBuffer = nameBuffer + "." + faults[0] + "." + faults[1] + "." +
                 faults[2] + "." + faults[3];
    inherentFault = Fault::genRandomFault(faults[0], NULL);
    setRatioFWC(inherentFault->getCellFaultRate());
    setActiveProbFWC(std::stod(faults[1]));
    setRatioWC(std::stod(faults[2]));
    setActiveProbWC(std::stod(faults[3]));
    dg->setInherentFault(inherentFault, ecc, true);

    printf("parameter review:\n");
    printf("1) ratio of freq. weak cells:   \t %.3e\n", getRatioFWC());
    printf("2) activ. prob. of freq. weak cells:\t %.3e\n", getActiveProbFWC());
    printf("3) ratio of weak cells:   \t %.3e\n", getRatioWC());
    printf("4) activ. prob. of weak cells:\t %.3e\n", getActiveProbWC());
  }
  else
  {
    // accept only two cases?
    assert(0);
  }
  signal(SIGTERM, sigterm_handler);

  FILE *fd = fopen(nameBuffer.c_str(), "w");
  assert(fd != NULL);
  dg->pickRandomFD()->faultRateInfo->printFaults();

  // reset statistics
  reset();
  long runNum = 0;
  // for runCnt times
  for (runNum = 0; runNum < runCnt; runNum++)
  {
    if ((runNum == 100) || ((runNum != 0) && (runNum % 1000000 == 0)))
    {
      printSummary(fd, runNum);
      dg->printFaultStats(stdout, DUECntYear[MAX_YEAR - 1], SDCCntYear[MAX_YEAR - 1], MAX_YEAR);
    }

    if (runNum % 10000000 == 0)
    {
      // if (runNum%1000000==0) {
      printf("Processing %ldth iteration\n", runNum);
    }

    if (inherentFault != NULL)
    {
      dg->resetInherentFault(inherentFault, ecc);
      dg->setInitialRetiredBlkCount(ecc, getRatioFWC());
    }

    // GONG: setting chip failure in advance
    //		dg->setSingleChipFault();
    //		dg->updateInherentFault(ecc);

    double hr = 0.;
    int CEcounter = 0;
    int errorCounter = 0;

    bool hr_datagen = false;
    while (true)
    {
      if (killflag)
      {
        break;
      }
      // 1. Advance
      double prevHr = hr;
      double delta = advance(dg->getFaultRate());
      hr += delta;
      if (dg->getFaultRate() > 2.7801302490616151e-07)
      {
        int a = 10;
      }

      // 2. Pick random Fault domain
      FaultDomain *fd = dg->pickRandomFD();

      if (hr > (MAX_YEAR - 1) * 24 * 365)
      {
        // if (hr > 5856) {
        // fd->printOperationalFaults();
        // fd->printVisualFaults();
        break;
      }
      errorCounter++;
      updateElapsedTime(hr);
      if (errorCounter > 100000)
      {
        break;
      }

      // fd->printTransientFaults();
      //  2. scrub soft errors
      scrubber->scrub(dg, hr);

      // 4. generate an error and decode it
      ErrorType result = fd->genSystemRandomFaultAndTest(ecc);

      // GONG: update inherent fault
      dg->updateInherentFault(ecc);

      // 5. process result
      // default : PF retirement

      if (fd->getRetiredBlkCount() >= 25 * 1024 && (result != CE))
      {
        printf("-------------RETIRE: hours %lf (%lfyrs),i retiredBlkCount: \
        %lld maxRetiredBlkCount: %lld\n",
               hr, hr / (24 * 365),
               fd->getRetiredBlkCount(), ecc->getMaxRetiredBlkCount());
        for (int i = 0; i < MAX_YEAR; i++)
        {
          if (hr < i * 24 * 365)
          {
            RetireCntYear[i]++;
          }
        }
        break;
      }
      else if (result == DUE)
      {
        // printf("===DUE: hours %lf (%lfyrs), isPFmode() %d  ", hr,
        // hr/(24*365), fd->faultRateInfo->iRate->IsPFmode());
        // printf("tick %d \n",runNum);
        printf("%lf DUE-", hr);
        fd->printOperationalFaults();
        // fd->printVisualFaults();
        // printf("\n");
        CEcounter = 0;
        for (int i = 0; i < MAX_YEAR; i++)
        {
          if (hr < i * 24 * 365)
          {
            DUECntYear[i]++;
            fd->setFaultStats(DUE, i);
          }
        }
        break;
      }
      else if (result == SDC)
      {
        // printf("***SDC: hours %lf (%lfyrs), isPFmode() %d  ", hr,
        // hr/(24*365), fd->faultRateInfo->iRate->IsPFmode());
        // printf("tick %d \n",runNum);
        printf("%lf SDC-", hr);
        fd->printOperationalFaults();
        // fd->printVisualFaults();

        CEcounter = 0;
        for (int i = 0; i < MAX_YEAR; i++)
        {
          if (hr < i * 24 * 365)
          {
            SDCCntYear[i]++;
            fd->setFaultStats(SDC, i);
          }
        }
        break;
      }
    }

    if (killflag)
    {
      printSummary(fd, runNum);
      dg->printFaultStats(stdout, DUECntYear[MAX_YEAR - 1], SDCCntYear[MAX_YEAR - 1], MAX_YEAR);
      break;
    }

    dg->clear();
    ecc->clear();
  }

  dg->printFaultStatsAll(stdout, DUECntYear, SDCCntYear, MAX_YEAR);
  // dg->printFaultStats(stdout,DUECntYear[MAX_YEAR-1],SDCCntYear[MAX_YEAR-1],MAX_YEAR);

  printSummary(fd, runNum);
  // ecc->printHistogram();
  fclose(fd);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void TesterScenario::reset()
{
  for (int i = 0; i <= SDC; i++)
  {
    errorCnt[i] = 0l;
  }
}
//------------------------------------------------------------------------------
void TesterScenario::printSummary(FILE *fd, long runNum)
{
  fprintf(fd, "After %ld runs\n", runNum);
  for (int i = 0; i <= SDC; i++)
  {
    fprintf(fd, "%s\t%.10f\n", errorName[i], (double)errorCnt[i] / runNum);
  }
  fflush(fd);
}
//------------------------------------------------------------------------------
void TesterScenario::test(DomainGroup *dg, ECC *ecc, Scrubber *scrubber,
                          long runCnt, char *filePrefix, int faultCount,
                          std::string *faults)
{
  assert(faultCount != 0);
  // create log file
  std::string nameBuffer = std::string(filePrefix) + "." + faults[0];
  for (int i = 1; i < faultCount; i++)
  {
    nameBuffer = nameBuffer + "." + faults[i];
  }
  FILE *fd = fopen(nameBuffer.c_str(), "w");
  assert(fd != NULL);

  // reset statistics
  reset();

  // for runCnt times
  for (long runNum = 0; runNum < runCnt; runNum++)
  {
    if ((runNum == 100) || ((runNum != 0) && (runNum % 10000000 == 0)))
    {
      printSummary(fd, runNum);
    }
    if (runNum % 1000000 == 0)
    {
      printf("Processing %ldth iteration\n", runNum);
    }

    // ErrorType result = dg->getFD()->genScenarioRandomFaultAndTest(ecc,
    // faultCount, faults);
    ErrorType result = dg->getFD()->genScenarioRandomFaultAndTest(
        ecc, faultCount - 2, faults, false);
    if (result == SDC)
    {
      dg->getFD()->setFaultStats(SDC, 0);
    }
    else if (result == DUE)
    {
      dg->getFD()->setFaultStats(DUE, 0);
    }
    // dg->getFD()->printVisualFaults();
    errorCnt[result]++;
  }

  dg->printFaultStats(stdout, errorCnt[DUE], errorCnt[SDC], 1);

  printSummary(fd, runCnt);
  ecc->printHistogram();
  fclose(fd);
}

//------------------------------------------------------------------------------
