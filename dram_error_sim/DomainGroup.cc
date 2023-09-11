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
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string>

#include "DomainGroup.hh"
#include "FaultDomain.hh"
char ErrorENUMNAME[][16] = {"SBIT ", "SWORD ", "SCOL ", "SROW ", "LOCALWORDLINE",
"SBANK ", "MBANK ", "MRANK ", "CHANNEL ", "BLSA", "BANKPATTERN", "CDEC", "CSL", "MMODULE", 
"RDEC", "SWD", "DISTBIT","MWL" ,"INHERENT1 ",
"INHERENT2 ","INHERENT3 ","INHERENT4 ","INHERENT5 ","INHERENT6 "};


FaultDomain *DomainGroup::pickRandomFD() {
  // ErrorType result;
  // int posFD = rand() % FDList.size();
  // auto it = FDList.begin();
  // for (int i=0; i<posFD; i++) {
  //    ++it;
  //}
  // return *it;
  double totalRate = getFaultRate();

  double draw = (double)rand() / RAND_MAX;
  double sum = .0;
  for (auto it = FDList.begin(); it != FDList.end(); it++) {
    sum += (*it)->getFaultRate();
    if ((sum / totalRate) >= draw) {
      return *it;
    }
  }
  assert(0);
}

double DomainGroup::getFaultRate() {
  if (FDList.size() == 0)
    return 0;
  else {
    double rate = 0;
    for (auto it = FDList.begin(); it != FDList.end(); ++it) {
      rate += (*it)->getFaultRate();
    }
    return rate;
  }
}

void DomainGroup::printFaultStats(FILE *fd, long DUECntYear, long SDCCntYear, int year){
    float DUEresult[ERRORENUM] = {0,};
    float SDCresult[ERRORENUM] = {0,};

    for (auto it = FDList.begin(); it!= FDList.end(); it++){
      float* tmp = (*it)->getFaultStats(DUE,year);
      for (int i=0;i<ERRORENUM;i++){
        DUEresult[i] += tmp[i];
      }
      tmp = (*it)->getFaultStats(SDC,year);
      for (int i=0;i<ERRORENUM;i++){
        SDCresult[i] += tmp[i];
      }
    }

    if(SDCCntYear!= 0){
    fprintf(fd,"Percent of error on SDC at %d\n",year);
      for(int i = 0;i < ERRORENUM; i++){
        fprintf(fd,"%s %f\n",ErrorENUMNAME[i], ((float)SDCresult[i])/SDCCntYear);
      }
    }

    if(DUECntYear!= 0){
    fprintf(fd,"\n\nPercent of error on DUE at %d\n",year);
      for(int i = 0;i < ERRORENUM; i++){
        fprintf(fd,"%s %f\n",ErrorENUMNAME[i], ((float)DUEresult[i])/DUECntYear);
      }
    }
    fprintf(fd,"\n\nPercent of error for entire Errors at %d\n",year);
    if(SDCCntYear+DUECntYear!= 0){
      for(int i = 0;i < ERRORENUM; i++){
        fprintf(fd,"%s %f\n",ErrorENUMNAME[i], 
        ((float)(SDCresult[i]+DUEresult[i]))/(SDCCntYear+DUECntYear));
      }
    }
    fflush(fd);
}


void DomainGroup::printFaultStatsAll(FILE *fd, long *DUECntYears, long *SDCCntYears,int MAXYEAR){
    float **DUEresult;
    float **SDCresult;
    std::string yearlist;
    std::string outputstring;

    DUEresult = new float*[MAXYEAR];
    SDCresult = new float*[MAXYEAR];
    for (int year =0;year<MAXYEAR;year++){
      SDCresult[year] = new float[ERRORENUM];
      DUEresult[year] = new float[ERRORENUM];
      for (int i =0;i<ERRORENUM;i++){
        SDCresult[year][i]=0;
        DUEresult[year][i]=0;
      }
    }
    

    for (int year = 1;year<MAXYEAR;year++){
      yearlist.append(std::to_string(year)).append(", ");
      for (auto it = FDList.begin(); it!= FDList.end(); it++){
        float* tmp = (*it)->getFaultStats(DUE,year);
        for (int i=0;i<ERRORENUM;i++){
          DUEresult[year-1][i] += tmp[i];
        }
        tmp = (*it)->getFaultStats(SDC,year);
        for (int i=0;i<ERRORENUM;i++){
          SDCresult[year][i] += tmp[i];
        }
      }
    }
    yearlist.append("\n");
    
    outputstring.append("Percent of error on SDC at, ").append(yearlist);
    for(int i = 0;i < ERRORENUM; i++){
      outputstring.append(ErrorENUMNAME[i]).append(" ");
      for(int year = 0;year<MAXYEAR;year++){
        if (SDCCntYears[year] != 0)
          outputstring.append(std::to_string((float)SDCresult[year][i]/SDCCntYears[year])).append(" ");
        else
          outputstring.append("0 ");
      }
      outputstring.append("\n");
    }
    outputstring.append("\n\n");
    
    outputstring.append("Percent of error on DUE at, ").append(yearlist);
    for(int i = 0;i < ERRORENUM; i++){
      outputstring.append(ErrorENUMNAME[i]).append(" ");
      for(int year = 0;year<MAXYEAR;year++){
        if (DUECntYears[year] != 0)
          outputstring.append(std::to_string((float)DUEresult[year][i]/DUECntYears[year])).append(" ");
        else
          outputstring.append("0 ");
      }
      outputstring.append("\n");
    }
    outputstring.append("\n\n");

    outputstring.append("Percent of error for entire Errors at, ").append(yearlist);
    for(int i = 0;i < ERRORENUM; i++){
      outputstring.append(ErrorENUMNAME[i]).append(" ");
      for(int year = 0;year<MAXYEAR;year++){
        if (DUECntYears[year] + SDCCntYears[year] != 0)
          outputstring.append(std::to_string((float)(DUEresult[year][i] + SDCresult[year][i])/(SDCCntYears[year]+DUECntYears[year]))).append(" ");
        else
          outputstring.append(std::to_string((float) 0)).append(" ");
      }
      outputstring.append("\n");
    }
    outputstring.append("\n\n");   

    fprintf(fd,"%s",outputstring.c_str());

    for(int i=0;i<MAXYEAR;i++){
      delete [] SDCresult[i];
      delete [] DUEresult[i];
    }
    delete[] SDCresult;
    delete[] DUEresult;
    fflush(fd);
}