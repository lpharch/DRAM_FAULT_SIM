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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <random>

#include "Config.hh"
#include "FaultRateInfo.hh"

#include "DomainGroup.hh"
#include "Scrubber.hh"
#include "Tester.hh"
#include "message.hh"

#include "Bamboo.hh"
#include "DUO.hh"
#include "Huawei.hh"
#include "REGB.hh"
#include "XED.hh"
#include "prior.hh"
#include "FlipCorrection.hh"

#include "Config.hh"

void testBamboo(int ID, DomainGroup *dg);
void testFrugal(int ID);
void testAIECC(int ID);
void testDUO(int ID);

// #define BAMBOO
#define AGECC

int BANKSPERBEAT = 1;
int module;

int main(int argc, char **argv)
{
  if (argc < 6)
  {
    printf(
        "Usage for system evaluation  : %s TargetSystemID numTrials RandomSeed "
        "S PermanentRate PermanentAct IntermittentRate IntermittentAct\n",
        argv[0]);
    printf(
        "Usage for scenario evaluation: %s TargetSystemID numTrials RandomSeed "
        "FaultType1 FaultType2 ...\n",
        argv[0]);
    exit(1);
  }
  setup_configs(argv[argc - 1]);
  // get the last argument in variable length argument list
  module = atoi(argv[argc - 2]);
  // random seed
  srand(atoi(argv[3]));
  // srand(time(NULL));

  char filePrefix[256];
  DomainGroup *dg = NULL;
  ECC *ecc = NULL;
  Tester *tester = NULL;
  Scrubber *scrubber = NULL;

  // int DIMMcnt = 100000;
  int DIMMcnt = 2;
  // int DIMMcnt = 2;
  // int DIMMcnt = 1;
  int chipNumber;
  int chipWidth;
  MSGConfig *message_config;

#ifdef BAMBOO
  switch (atoi(argv[1]))
  {
  // 2 rank / x4 chip / 64-/72-bit channel
  case 0: // bit-level
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 8);
    ecc = new ECCNone();
    sprintf(filePrefix, "000.4x16.None.%s", argv[3]);
    break;
  case 1:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new SECDED72b();
    sprintf(filePrefix, "001.4x18.SECDED72b.%s", argv[3]);
    break;
  case 2:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 17, 4, 8);
    ecc = new SPC66bx4();
    sprintf(filePrefix, "002.4x17.SPC66bx4.%s", argv[3]);
    break;
  case 3:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 17, 4, 8);
    ecc = new SPCTPD68bx4();
    sprintf(filePrefix, "003.4x17.SPCTPD68bx4.%s", argv[3]);
    break;
  case 10: // chip-level
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    sprintf(filePrefix, "010.4x18.AMD.%s", argv[3]);
    break;
  case 11:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b();
    sprintf(filePrefix, "011.4x18.QPC.%s", argv[3]);
    break;
  case 20:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 36, 4, 8);
    ecc = new AMDDChipkill144b();
    sprintf(filePrefix, "020.4x36.DAMD.%s", argv[3]);
    break;
  case 21:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 36, 4, 8);
    ecc = new OPC144b();
    sprintf(filePrefix, "021.4x36.OPC.%s", argv[3]);
    break;
  // 2 rank / x8 chip / 72-bit channel
  case 30: // bit-level
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 8, 8);
    ecc = new SECDED72b();
    sprintf(filePrefix, "030.9x8.SECDED72b.%s", argv[3]);
    break;
  // 2 rank / x8 chip / 144-bit channel
  case 40:
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 8, 8);
    ecc = new S8SC144b();
    sprintf(filePrefix, "040.8x18.S8SC.%s", argv[3]);
    break;
  case 41:
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 8, 8);
    ecc = new OPC144b();
    sprintf(filePrefix, "041.8x18.OPC.%s", argv[3]);
    break;
  case 50: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 1);
    sprintf(filePrefix, "050.4x18.QPC41.%s", argv[3]);
    break;
  case 51: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 2);
    sprintf(filePrefix, "051.4x18.QPC42.%s", argv[3]);
    break;
  case 52: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 3);
    sprintf(filePrefix, "052.4x18.QPC43.%s", argv[3]);
    break;
  case 53: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 4);
    sprintf(filePrefix, "053.4x18.QPC44.%s", argv[3]);
    break;
  case 60:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 19, 4, 8);
    ecc = new QPC76b();
    sprintf(filePrefix, "060.19x4.QPC76b.%s", argv[3]);
    break;
  default:
    printf("Invalid ECC ID\n");
    exit(1);
  }
#endif
#ifdef AGECC
  switch (atoi(argv[1]))
  {
  case 0: // None
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 8);
    ecc = new ECCNone();
    sprintf(filePrefix, "000.4x16.None.%s", argv[3]);
    break;
  case 1: // None
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new ECCNone();
    sprintf(filePrefix, "001.4x18.None.%s", argv[3]);
    break;
  case 2: // Zero EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new SECDED72b();
    sprintf(filePrefix, "002.4x18.SECDED72b.%s", argv[3]);
    break;
  case 9: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    sprintf(filePrefix, "009.4x18.AMD.%s", argv[3]);
    break;
  case 10: // Zero EGB + no post-processing
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(false);
    sprintf(filePrefix, "010.4x18.AMD2.%s", argv[3]);
    break;
  case 11: //
    message_config = new MSGConfig(8, 8, 0, 0, 1, 4, 18, 0, EXTRACHIP);
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8, *message_config, false, false);
    ecc = new AMDChipkill72b(false);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "011_simplemodel.4x18.AMD2.%s", argv[3]);
    break;
  case 12: //
    message_config = new MSGConfig(8, 8, 0, 0, 1, 4, 18, 0, EXTRACHIP);
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8, *message_config, false, true);
    ecc = new AMDChipkill72b(false);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "012_newmodel.4x18.AMD2.%s", argv[3]);
    break;
  case 20: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 1);
    sprintf(filePrefix, "020.4x18.QPC41.%s", argv[3]);
    break;
  case 21: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 2);
    sprintf(filePrefix, "021.4x18.QPC42.%s", argv[3]);
    break;
  case 22: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 3);
    sprintf(filePrefix, "022.4x18.QPC43.%s", argv[3]);
    break;
  case 23: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(4, 4);
    sprintf(filePrefix, "023.4x18.QPC44.%s", argv[3]);
    break;
  case 24: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(3, 3);
    sprintf(filePrefix, "024.4x18.QPC33.%s", argv[3]);
    break;
  case 25: // EGB
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72b(2, 2);
    sprintf(filePrefix, "025.4x18.QPC22.%s", argv[3]);
    break;
  case 100: // on-chip ECC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 18);
    ecc = new OnChip64b();
    sprintf(filePrefix, "100.4x16.OnChip.%s", argv[3]);
    break;
  case 110: // on-chip ECC + SEC-DED
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bSECDED();
    sprintf(filePrefix, "110.4x18.OnChip+SECDED.%s", argv[3]);
    break;
  case 111: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    sprintf(filePrefix, "111.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 112: // on-chip ECC + AMD (w/o postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(false);
    sprintf(filePrefix, "112.4x18.OnChip+AMD2.%s", argv[3]);
    break;
  case 113: // on-chip ECC + QPC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bQPC72b(4, 2);
    sprintf(filePrefix, "113.4x18.OnChip+QPC42.%s", argv[3]);
    break;
  case 114: // on-chip ECC + QPC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bQPC72b(4, 3);
    sprintf(filePrefix, "114.4x18.OnChip+QPC43.%s", argv[3]);
    break;
  case 115: // on-chip ECC + QPC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bQPC72b(4, 4);
    sprintf(filePrefix, "115.4x18.OnChip+QPC44.%s", argv[3]);
    break;
  case 116: // on-chip ECC + QPC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bQPC72b(3, 3);
    sprintf(filePrefix, "116.4x18.OnChip+QPC33.%s", argv[3]);
    break;
  case 117: // on-chip ECC + QPC
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bQPC72b(2, 2);
    sprintf(filePrefix, "117.4x18.OnChip+QPC22.%s", argv[3]);
    break;
  case 130:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    sprintf(filePrefix, "130.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 132:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(false); // w/o fault diagnosis
    sprintf(filePrefix, "132.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 140:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    sprintf(filePrefix, "140.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 141:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, false); // w/o retire
    sprintf(filePrefix, "141.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 200: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    sprintf(filePrefix, "200.4x18.AMD.%s", argv[3]);
    break;
  case 201: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "201.4x18.AMD.%s", argv[3]);
    break;
  case 202: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    ecc->setMaxRetiredBlkCount(64);
    sprintf(filePrefix, "202.4x18.AMD.%s", argv[3]);
    break;
  case 203: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "203.4x18.AMD.%s", argv[3]);
    break;
  case 204: // Zero EGB + post-processing
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 8);
    ecc = new AMDChipkill72b(true);
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "204.4x18.AMD.%s", argv[3]);
    break;
  case 205: // Zero EGB + post-processing
            // DomainGroupDDR(DIMMcnt/8, 8, 5, 4, 34)
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 6, 4, 32);
    ecc = new AMDChipkill20b(false);
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "205.4x5.AMD.%s", argv[3]);
    break;
  case 210: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    sprintf(filePrefix, "210.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 211: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "211.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 212: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(64 / 2);
    sprintf(filePrefix, "212.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 213: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(2048 / 2);
    sprintf(filePrefix, "213.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 214: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(4096 / 2);
    // ecc->setMaxRetiredBlkCount(16384/2);
    sprintf(filePrefix, "214.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 215: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(8192 / 2);
    // ecc->setMaxRetiredBlkCount(16384/2);
    sprintf(filePrefix, "215.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 216: // on-chip ECC + AMD (w/ postprocessing)
    dg = new DomainGroupDDR(DIMMcnt / 4, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    // ecc->setMaxRetiredBlkCount(2048);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "216.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 220:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    sprintf(filePrefix, "220.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 221:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "221.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 222:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(64 / 2);
    sprintf(filePrefix, "222.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 223:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(2048 / 2);
    sprintf(filePrefix, "223.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 224:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(16384 / 2);
    sprintf(filePrefix, "224.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 230:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    ecc->setDoRetire(false);
    sprintf(filePrefix, "230.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 231:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "231.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 232:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    ecc->setMaxRetiredBlkCount(64);
    sprintf(filePrefix, "232.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 233:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "233.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 234:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new QPC72bREGB(true, true); // w/ retire
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "234.4x18.QPC_REGB.%s", argv[3]);
    break;
  case 240:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    sprintf(filePrefix, "240.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 241:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "241.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 242:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(64 / 2);
    sprintf(filePrefix, "242.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 243:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(2048 / 2);
    sprintf(filePrefix, "243.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 244:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 18);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(16384 / 2);
    sprintf(filePrefix, "244.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 300:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(1);
    sprintf(filePrefix, "300.4x16.DUO.%s", argv[3]);
    break;
  case 301:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(1);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "301.4x16.DUO.%s", argv[3]);
    break;
  case 302:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(1);
    ecc->setMaxRetiredBlkCount(64);
    sprintf(filePrefix, "302.4x16.DUO.%s", argv[3]);
    break;
  case 303:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(1);
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "303.4x16.DUO.%s", argv[3]);
    break;
  case 304:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(1);
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "304.4x16.DUO.%s", argv[3]);
    break;
  case 310:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(2);
    sprintf(filePrefix, "310.4x16.DUO.%s", argv[3]);
    break;
  case 311:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(2);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "311.4x16.DUO.%s", argv[3]);
    break;
  case 312:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(2);
    ecc->setMaxRetiredBlkCount(64);
    sprintf(filePrefix, "312.4x16.DUO.%s", argv[3]);
    break;
  case 313:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(2);
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "313.4x16.DUO.%s", argv[3]);
    break;
  case 314:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(2);
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "314.4x16.DUO.%s", argv[3]);
    break;
  case 320:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(3);
    sprintf(filePrefix, "320.4x16.DUO.%s", argv[3]);
    break;
  case 321:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(3);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "321.4x16.DUO.%s", argv[3]);
    break;
  case 322:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(3);
    ecc->setMaxRetiredBlkCount(64);
    sprintf(filePrefix, "322.4x16.DUO.%s", argv[3]);
    break;
  case 323:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(3);
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "323.4x16.DUO.%s", argv[3]);
    break;
  case 324:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4(3);
    ecc->setMaxRetiredBlkCount(16384);
    sprintf(filePrefix, "324.4x16.DUO.%s", argv[3]);
    break;
  case 330:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    ecc = new DUO36bx4(6, false, false, 0);
    // ecc->setDoRetire(false);//no retirement
    // ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "330.4x9(BL17).DUO.%s", argv[3]);
    break;
  case 331:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    // ecc = new DUO36bx4(3);
    ecc = new DUO36bx4(6, false, true, 128 * 1024);
    ecc->setMaxRetiredBlkCount(128 * 1024);
    sprintf(filePrefix, "331.4x9(BL17).DUO.%s", argv[3]);
    break;
  case 332:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    // ecc = new DUO36bx4(3);
    ecc = new DUO36bx4(6, false, true, 512 * 1024);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "332.4x9(BL17).DUO.%s", argv[3]);
    break;
  case 333:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    // ecc = new DUO36bx4(3);
    ecc = new DUO36bx4_meta(6, false, true, 512 * 1024, 1);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "333.4x9(BL17).DUO.meta1.%s", argv[3]);
    break;
  case 334:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    // ecc = new DUO36bx4(3);
    ecc = new DUO36bx4_meta(6, false, true, 512 * 1024, 2);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "334.4x9(BL17).DUO.meta2.%s", argv[3]);
    break;
  case 335:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    // ecc = new DUO36bx4(3);
    ecc = new DUO36bx4_meta(6, false, true, 512 * 1024, 3);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "335.4x9(BL17).DUO.meta3.%s", argv[3]);
    break;
  case 340:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 18);
    ecc = new XED_SDDC_NC(true); // w/ fault diagnosis
    sprintf(filePrefix, "340.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 341:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 18);
    ecc = new XED_SDDC_NC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "341.4x9.XED_SDDC.%s", argv[3]);
    break;
  case 350:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34(10, false, false, 0);
    sprintf(filePrefix, "350.4x5(BL34).DUO.%s", argv[3]);
    break;
  case 351:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34(10, false, true, 128 * 1024);
    sprintf(filePrefix, "351.4x5(BL34).DUO.%s", argv[3]);
    break;
  case 352:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34(10, false, true, 512 * 1024);
    sprintf(filePrefix, "352.4x5(BL34).DUO.%s", argv[3]);
    break;
  case 353:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34_meta(10, false, false, 512 * 1024, 1);
    sprintf(filePrefix, "353.4x5(BL34).DUO.meta1.%s", argv[3]);
    break;
  case 354:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34_meta(10, false, true, 512 * 1024, 2);
    sprintf(filePrefix, "354.4x5(BL34).DUO.meta2.%s", argv[3]);
    break;
  case 355:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 34);
    ecc = new DUO20bx4_34_meta(10, false, true, 512 * 1024, 3);
    sprintf(filePrefix, "355.4x5(BL34).DUO.meta3.%s", argv[3]);
    break;
  case 360:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 33);
    ecc = new DUO20bx4_33(10, false, false, 0);
    sprintf(filePrefix, "360.4x5(BL33).DUO.%s", argv[3]);
    break;
  case 361:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 33);
    ecc = new DUO20bx4_33(10, false, true, 128 * 1024);
    sprintf(filePrefix, "361.4x5(BL33).DUO.%s", argv[3]);
    break;
  case 362:
    dg = new DomainGroupDDR(DIMMcnt / 8, 8, 5, 4, 33);
    ecc = new DUO20bx4_33(10, false, true, 512 * 1024);
    sprintf(filePrefix, "362.4x5(BL33).DUO.%s", argv[3]);
    break;

  case 900:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 8, 9);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    sprintf(filePrefix, "900.8x9.XED_SDDC.%s", argv[3]);
    break;
  case 901:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 8, 9);
    ecc = new XED_SDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(2048 / 2);
    sprintf(filePrefix, "901.8x9.XED_SDDC.%s", argv[3]);
    break;
  case 910:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    sprintf(filePrefix, "910.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 911:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new XED_DDDC(true); // w/ fault diagnosis
    ecc->setMaxRetiredBlkCount(2048 / 2);
    sprintf(filePrefix, "911.4x18.XED_DDDC.%s", argv[3]);
    break;
  case 920:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 8, 9);
    ecc = new DUO72bx8(2, false, false, 0);
    sprintf(filePrefix, "920.8x9.DUO.%s", argv[3]);
    break;
  case 921:
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 8, 9);
    ecc = new DUO72bx8(2, false, true, 2048);
    sprintf(filePrefix, "921.8x9.DUO.%s", argv[3]);
    break;
  // case 930:
  //    dg = new DomainGroupDDR(DIMMcnt/2, 2, 16, 4, 9);
  //    ecc = new DUO64bx4(1);
  //    sprintf(filePrefix, "930.4x16.DUO.%s", argv[3]);
  //    break;
  case 940:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 9);
    ecc = new DUO72bx4(2, false, false, 0);
    sprintf(filePrefix, "940.4x18.DUO.%s", argv[3]);
    break;
  case 941:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 9);
    ecc = new DUO72bx4(2, false, true, 128 * 1024);
    sprintf(filePrefix, "941.4x18.DUO.%s", argv[3]);
    break;
  case 942:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 9);
    ecc = new DUO72bx4(2, false, false, 512 * 1024);
    sprintf(filePrefix, "942.4x18.DUO.%s", argv[3]);
    break;
  case 950:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    sprintf(filePrefix, "950.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 951:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip72bAMD(true);
    ecc->setMaxRetiredBlkCount(2048);
    sprintf(filePrefix, "951.4x18.OnChip+AMD.%s", argv[3]);
    break;
  case 960:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip36bSDDC(true);
    sprintf(filePrefix, "960.4x10.OnChip+SDDC.%s", argv[3]);
    break;
  case 970:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 16, 4, 9);
    ecc = new DUO64bx4_(2, false, false, 0); // 12.5% on-chip redundancy
    sprintf(filePrefix, "970.4x18.DUO.125.%s", argv[3]);
    break;
  case 971:
    dg = new DomainGroupDDR(1, 1, 16, 4, 9);
    ecc = new DUO64bx4_(4, false, false, 0); // 12.5% on-chip redundancy
    sprintf(filePrefix, "971.4x18.DUO.125.%s", argv[3]);
    break;
  case 980:
    dg = new DomainGroupDDR(DIMMcnt / 2, 1, 18, 4, 18);
    ecc = new OnChip72bBamboo();
    sprintf(filePrefix, "980.huawei.4x10.OnChip+Bamboo.%s", argv[3]);
    break;
  // Huawei
  case 1000:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bSDDC(true);
    sprintf(filePrefix, "1000.huawei.4x10.OnChip+SDDC.%s", argv[3]);
    break;
  case 1010:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(0); // ino meta data
    sprintf(filePrefix, "1010.huawei.4x10.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1012:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(2); // two-byte meta data
    sprintf(filePrefix, "1012.huawei.4x10.OnChip+Bamboo+2B.%s", argv[3]);
    break;
  case 1013:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(3); // three-byte meta data
    sprintf(filePrefix, "1013.huawei.4x10.OnChip+Bamboo+3B.%s", argv[3]);
    break;
  case 1014:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(4); // four-byte meta data
    sprintf(filePrefix, "1014.huawei.4x10.OnChip+Bamboo+4B.%s", argv[3]);
    break;
  case 1015:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(5); // five-byte meta data
    sprintf(filePrefix, "1015.huawei.4x10.OnChip+Bamboo+5B.%s", argv[3]);
    break;
  case 1016:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 18);
    ecc = new OnChip40bBamboo(6); // six-byte meta data
    sprintf(filePrefix, "1016.huawei.4x10.OnChip+Bamboo+6B.%s", argv[3]);
    break;
  case 1020:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 17);
    ecc = new OnChip40bDUO(0); // DUO with 2-byte meta data
    sprintf(filePrefix, "1020.huawei.4x10.OnChipDUO.0B.%s", argv[3]);
    break;
  case 1022:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 17);
    ecc = new OnChip40bDUO(2); // DUO with 2-byte meta data
    sprintf(filePrefix, "1022.huawei.4x10.OnChipDUO.2B.%s", argv[3]);
    break;
  case 1023:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 17);
    ecc = new OnChip40bDUO(3); // DUO with 3-byte meta data
    sprintf(filePrefix, "1023.huawei.4x10.OnChipDUO.3B.%s", argv[3]);
    break;
  case 1024:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 10, 4, 17);
    ecc = new OnChip40bDUO(4); // DUO with 4-byte meta data
    sprintf(filePrefix, "1024.huawei.4x10.OnChipDUO.4B.%s", argv[3]);
    break;
  case 1099: // 9x4 SDDC baseline, which should run no inherent faults
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 9, 4, 16);
    ecc = new SDDCBamboo36b();
    sprintf(filePrefix, "1099.huawei.4x9.SDDCBaseline.%s", argv[3]);
    break;
  case 1100:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bXEDx8(true, 0);
    sprintf(filePrefix, "1100.huawei.8x5.OnChip+XED.%s", argv[3]);
    break;
  case 1103:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bXEDx8(true, 3);
    sprintf(filePrefix, "1103.huawei.8x5.OnChip+XED.%s", argv[3]);
    break;
  case 1110:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bBamboox8(0); // ino meta data
    sprintf(filePrefix, "1110.huawei.8x5.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1111:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bBamboox8(1); // ino meta data
    sprintf(filePrefix, "1111.huawei.8x5.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1112:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bBamboox8(2); // ino meta data
    sprintf(filePrefix, "1112.huawei.8x5.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1113:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bBamboox8(3); // ino meta data
    sprintf(filePrefix, "1113.huawei.8x5.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1120:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bDUOx8(0); // DUO with 2-byte meta data
    sprintf(filePrefix, "1120.huawei.8x5.OnChipDUOx8.0B.%s", argv[3]);
    break;
  case 1122:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bDUOx8(2); // DUO with 2-byte meta data
    sprintf(filePrefix, "1122.huawei.8x5.OnChipDUOx8.2B.%s", argv[3]);
    break;
  case 1123:
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 5, 8, 17);
    ecc = new OnChip40bDUOx8(3); // DUO with 2-byte meta data
    sprintf(filePrefix, "1123.huawei.8x5.OnChipDUOx8.3B.%s", argv[3]);
    break;
  case 1200:
    dg = new DomainGroupDDR(DIMMcnt / 4, 4, 8, 8, 17);
    ecc = new OnChip4xSEC(136);
    sprintf(filePrefix, "1200.huawei.4x16.OnChipSEC.%s", argv[3]);
    break;
  case 1201:
    dg = new DomainGroupDDR(2, 4, 8, 8, 9);
    ecc = new OnChip8xSECDED(136);
    sprintf(filePrefix, "1201.huawei.8x8.OnChipSECDED.%s", argv[3]);
    break;
  case 1210:
    dg = new DomainGroupDDR(1, 1, 4, 8, 17);
    ecc = new OnChip2xSEC16b(272);
    sprintf(filePrefix, "1210.huawei.16x2.OnChipSEC272.%s", argv[3]);
    break;
  case 1220:
    dg = new DomainGroupDDR(1, 1, 4, 8, 17);
    ecc = new OnChip1xSEC16b(512);
    sprintf(filePrefix, "1220.huawei.16x2.OnChipSEC512.%s", argv[3]);
    break;
  case 1230:
    dg = new DomainGroupDDR(4, 4, 4, 8, 17);
    ecc = new OnChipBCHTriple(true);
    sprintf(filePrefix, "1230.huawei.4x8.OnChipBCH.Triple.%s", argv[3]);
    break;
  case 1231:
    dg = new DomainGroupDDR(4, 4, 4, 8, 18);
    ecc = new OnChipBCHHexa(true);
    sprintf(filePrefix, "1231.huawei.4x8.OnChipBCH.Hexa.%s", argv[3]);
    break;
  case 1299: // no ecc baseline
    dg = new DomainGroupDDR(1, 1, 2, 16, 16);
    ecc = new OnChipNone();
    sprintf(filePrefix, "1299.huawei.16x2.None.%s", argv[3]);
    break;
  case 1300: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChipSym16bBB72b(true);
    sprintf(filePrefix, "1300.huawei.18x4.OnChip+Bamboo(16bit Sym).%s",
            argv[3]);
    break;
  case 1310: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8);
    ecc = new OnChipSym16bBB72b(false);
    sprintf(filePrefix, "1310.huawei.18x4.Bamboo(16bit Sym).%s", argv[3]);
    break;
  case 1320: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(true, 0);
    sprintf(filePrefix, "1320.huawei.18x4.OnChip+Bamboo.%s", argv[3]);
    break;
  case 1321: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(true, 1);
    sprintf(filePrefix, "1321.huawei.18x4.OnChip+Bamboo+1Bmeta.%s", argv[3]);
    break;
  case 1322: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(true, 2);
    sprintf(filePrefix, "1322.huawei.18x4.OnChip+Bamboo+2Bmeta.%s", argv[3]);
    break;
  case 1323: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(true, 3);
    sprintf(filePrefix, "1323.huawei.18x4.OnChip+Bamboo+3Bmeta.%s", argv[3]);
    break;
  case 1330: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(false, 0);
    sprintf(filePrefix, "1330.huawei.18x4.Bamboo.%s", argv[3]);
    break;
  case 1331: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(false, 1);
    sprintf(filePrefix, "1331.huawei.18x4.Bamboo+1Bmeta.%s", argv[3]);
    break;
  case 1332: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(false, 2);
    sprintf(filePrefix, "1332.huawei.18x4.Bamboo+2Bmeta.%s", argv[3]);
    break;
  case 1333: //
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 18);
    ecc = new OnChip8bBB72b(false, 3);
    sprintf(filePrefix, "1333.huawei.18x4.Bamboo+3Bmeta.%s", argv[3]);
    break;
  case 2900: // DUO - 9chip baseline for rank level PIM
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 17);
    ecc = new DUO36bx4(6, false, true, 512 * 1024);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    // ecc->setDoRetire(false);//no retirement
    // ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "2900.4x9(BL17).DUO.rankPIM.%s", argv[3]);
    break;
  case 2910: // Bamboo - 10chip baseline for rank level PIM
    // On rank + IECC DDR5
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 18);
    ecc = new OnChip40bBamboo(0); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "2910.4x10.OnChip+Bamboo.rankPIM.%s", argv[3]);
    break;
  case 2911: // Bamboo - 8chip naive bank level HBM-PIM(144,128) like code without rank ece
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 8, 4, 20);
    ecc = new Onchip144_128(); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "2911.4x8.Naive_OnChip.bankPIM.%s", argv[3]);
    break;
  case 3000: // DUO- 9chip bank level PIM with 8bit CRC
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 9, 4, 19);
    ecc = new FlipCRC_DUO(6, false, true, 512 * 1024);
    ecc->setMaxRetiredBlkCount(512 * 1024);
    // ecc->setDoRetire(false);//no retirement
    // ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "3000.4x9(BL17).CRC+DUO.bankPIM.%s", argv[3]);
    break;
  case 3010: // DUO - 10chip bamboo with 8bit CRC
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 2 + 2);
    ecc = new FlipCRC_Bamboo(0, 8); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "3010.huawei.4x10.8bCRC+Bamboo.bankPIM.%s", argv[3]);
    break;
  case 3011: //  DUO - 10chip bamboo with 16bit CRC
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 2 + 4);
    ecc = new FlipCRC_Bamboo(0, 16); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "3011.huawei.4x10.16bCRC+Bamboo.bankPIM.%s", argv[3]);
    break;
  case 4000: // Bamboo - 10chip baseline for rank level PIM
    // On rank + IECC DDR5
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 + 2);
    ecc = new OnChip40bBamboo(0); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4000.4x10.OnChip+Bamboo.rankPIM.%s", argv[3]);
    break;
  case 4001: // Bamboo - 10chip baseline for rank level PIM
    // On rank DDR5
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 + 2);
    ecc = new OnChip40bBamboo(0, false, false); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4001.4x10.Bamboo.rankPIM.%s", argv[3]);
    break;
  case 4010: // CRC 8 128 overfetch
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 2 + 2);
    ecc = new CRCECC128(8); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4010.4x8.CRC8_128overfetch.%s", argv[3]);
    break;
  case 4011: // CRC 16 128 overfetch
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 2 + 4);
    ecc = new CRCECC128(16); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4011.4x8.CRC16_128overfetch.%s", argv[3]);
    break;
  case 4012: // CRC 8 256 overfetch
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 4 + 2);
    ecc = new CRCECC256(8); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4012.4x8.CRC8_256overfetch.%s", argv[3]);
    break;
  case 4013: // CRC 16 256 overfetch
    dg = new DomainGroupDDR(DIMMcnt / 2, 4, 10, 4, 16 * 4 + 4);
    ecc = new CRCECC256(16); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    sprintf(filePrefix, "4013.4x8.CRC16_256overfetch.%s", argv[3]);
    break;
  case 4019: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, false);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4020_simplemodel.HBM3.2RS8.%s", argv[3]);
    break;
  case 4020: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4020_newmodel.HBM3.2RS8.%s", argv[3]);
    break;
  case 4021: // HBM real case, 256bit + 32bit ECC + AIECC 16bit
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_aiecc(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4021_newmodel.HBM3.2RS8_AIECC.%s", argv[3]);
    break;
  case 4022: // HBM real case, 256bit + 32bit ECC + AIECC 16bit
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, false);
    dg->setHBM(true);
    ecc = new LargeRS_aiecc(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4021_simplemodel.HBM3.2RS8_AIECC.%s", argv[3]);
    break;

  case 4023: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, false);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4023_simplemodel.HBM3.RS8.%s", argv[3]);
    break;
  case 4024: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4023_newmodel.HBM3.RS8.%s", argv[3]);
    break;
  case 4025: // HBM real case, 256bit + 32bit ECC + AIECC 16bit
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_aiecc(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4025_newmodel.HBM3.RS8_AIECC.%s", argv[3]);
    break;
  case 4026: // HBM real case, 256bit + 32bit ECC + AIECC 16bit
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, false);
    dg->setHBM(true);
    ecc = new LargeRS_aiecc(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4025_simplemodel.HBM3.RS8_AIECC.%s", argv[3]);
    break;

  case 4027: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, false);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(2); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4027_simplemodel.HBM3.RS16.%s", argv[3]);
    break;
  case 4028: // HBM real case, 256bit + 32bit ECC
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_HBM(2); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4027_newmodel.HBM3.RS16.%s", argv[3]);
    break;
  case 4029: // HBM real case, 256bit + 32bit ECC + AIECC 16bit
    chipNumber = 1;
    chipWidth = 36;
    message_config = new MSGConfig(8, 8, 0, 0, 1, chipWidth, chipNumber);
    dg = new DomainGroupDDR(1, 16, chipNumber, chipWidth, 8, *message_config, true, true);
    dg->setHBM(true);
    ecc = new LargeRS_aiecc(2); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4029_newmodel.HBM3.RS16_AIECC.%s", argv[3]);
    break;

  case 4030: // DDR5 IECC
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(16, 16, 0, 8, 2, chipWidth - 1, chipNumber, 1, EXTRAPIN);

    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16 * 2, *message_config);
    ecc = new OnChip4xSEC_FLEX(0); // ino meta data
    // ecc->setMaxRetiredBlkCount(512*1024);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4030.4x8.DDR5_IECC.%s", argv[3]);
    break;
  case 4031: // Bamboo - 10chip baseline for rank level PIM
    // On rank + IECC DDR5
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(16, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16, *message_config);
    ecc = new OnChip40bBamboo_FLEX(0); // ino meta data
    // ecc->setMaxRetiredBlkCount(512*1024);
    sprintf(filePrefix, "4031_newmodel.4x10.OnChip+Bamboo.rankPIM.%s", argv[3]);
    break;
  case 4032: // Bamboo - 10chip baseline for rank level PIM
    // On rank + IECC DDR5
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(16, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16, *message_config, false, false);
    ecc = new OnChip40bBamboo_FLEX(0); // ino meta data
    // ecc->setMaxRetiredBlkCount(512*1024);
    sprintf(filePrefix, "4031_simplemodel.4x10.OnChip+Bamboo.rankPIM.%s", argv[3]);
    break;
  case 4040: // DDR5 DEC
    chipNumber = 10;
    chipWidth = 10;
    message_config = new MSGConfig(16, 16, 0, 14, 1, chipWidth - 2, chipNumber, 2, EXTRAPIN);
    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16, *message_config);
    ecc = new OnChip4xSEC_FLEX(2); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4040.4x8.DDR5_IECC_DEC.%s", argv[3]);
    break;
  case 4041: // DDR5 TEC
    chipNumber = 10;
    chipWidth = 12;
    message_config = new MSGConfig(16, 16, 0, 10, 1, chipWidth - 4, chipNumber, 4, EXTRAPIN);
    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16, *message_config);
    ecc = new OnChip4xSEC_FLEX(3); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4041.4x8.DDR5_IECC_TEC.%s", argv[3]);
    break;
  case 4042: // DDR5 QEC
    chipNumber = 10;
    chipWidth = 12;
    message_config = new MSGConfig(16, 16, 0, 14, 1, chipWidth - 4, chipNumber, 4, EXTRAPIN);
    dg = new DomainGroupDDR(1, 1, chipNumber, chipWidth, 16, *message_config);
    ecc = new OnChip4xSEC_FLEX(4); // ino meta data
    ecc->setMaxRetiredBlkCount(512 * 1024);
    BANKSPERBEAT = 1;
    sprintf(filePrefix, "4042.4x8.DDR5_IECC_QEC.%s", argv[3]);
    break;

  case 5001: //
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(2, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new AMDChipkill_FLEX(false, true);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5001_newmodel.4x10DDR5.AMD2.%s", argv[3]);
    break;
  case 5002: //
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(2, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, false);
    ecc = new AMDChipkill_FLEX(false, true);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5001_simplemodel.4x10DDR5.AMD2.%s", argv[3]);
    break;
  case 5003: //
    chipNumber = 9;
    chipWidth = 5;
    message_config = new MSGConfig(4, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new AMDChipkill_FLEX(false, true, 2);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5003_newmodel.4x9DDR5.AMD2.%s", argv[3]);
    break;
  case 5004: //
    chipNumber = 9;
    chipWidth = 5;
    message_config = new MSGConfig(4, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, false);
    ecc = new AMDChipkill_FLEX(false, true, 2);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5003_simplemodel.4x9DDR5.AMD2.%s", argv[3]);
    break;
  case 5005: //
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(2, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new AMDChipkill_FLEX(false, false);
    sprintf(filePrefix, "5005_newmodel.4x10DDR5_noecc.AMD2.%s", argv[3]);
    break;
  case 5006: //
    chipNumber = 10;
    chipWidth = 5;
    message_config = new MSGConfig(2, 16, 0, 0, 1, 4, chipNumber, 1, EXTRACHIP);
    dg = new DomainGroupDDR(1, 2, chipNumber, chipWidth, 16, *message_config, false, false);
    ecc = new AMDChipkill_FLEX(false, false);
    sprintf(filePrefix, "5005_simplemodel.4x10DDR5_noecc.AMD2.%s", argv[3]);
    break;
  case 5011: // DDR4 chipkill, but half correctable
    message_config = new MSGConfig(4, 8, 0, 0, 1, 4, 18, 0, EXTRACHIP);
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8, *message_config, false, false);
    ecc = new AMDChipkill_FLEX(false, false, 1);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5011_simplemodel.4x18.AMD2_half.%s", argv[3]);
    break;
  case 5012: //
    message_config = new MSGConfig(4, 8, 0, 0, 1, 4, 18, 0, EXTRACHIP);
    dg = new DomainGroupDDR(DIMMcnt / 2, 2, 18, 4, 8, *message_config, false, true);
    ecc = new AMDChipkill_FLEX(false, false, 1);
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "5012_newmodel.4x18.AMD2_half.%s", argv[3]);
    break;

  case 6000: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 17;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth - 1, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_SEC_SEC.%s", argv[3]);
    break;
  case 6001: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 17;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth - 1, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_RS1.%s", argv[3]);
    break;
  case 6002: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 18;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(2); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_RS2.%s", argv[3]);
    break;

  case 6003: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 17;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(0); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_SEC_SEC.%s", argv[3]);
    break;
  case 6004: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 18;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(1); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_DEC.%s", argv[3]);
    break;
  case 6005: // LPDDR5 SEC-DED + Link CRC
    chipNumber = 2;
    chipWidth = 18;
    message_config = new MSGConfig(16, 16, 0, 0, 1, chipWidth, chipNumber, 1, EXTRAPIN);
    dg = new DomainGroupDDR(DIMMcnt / 2, 16, chipNumber, chipWidth, 16, *message_config, false, true);
    ecc = new LPDDR5_ONCHIP(2); // ino meta data
    ecc->setMaxRetiredBlkCount(0);
    sprintf(filePrefix, "6000_newmodel.LPDDR5_RS.%s", argv[3]);
    break;
    /*
        case 4021: // RS 32 256 overfetch with banksplit
          chipNumber = 8;
          chipWidth = 4;
          message_config = new MSGConfig(4,16,8,4,4,chipWidth,chipNumber);
          dg = new DomainGroupDDR(DIMMcnt / 2, 4, chipNumber, chipWidth, 16*4+8, *message_config);
          ecc = new LargeRS(16);  // ino meta data
          ecc->setMaxRetiredBlkCount(512*1024);
          BANKSPERBEAT = 18;
          sprintf(filePrefix, "4021.4x8.RS16_256overfetchbanksplit.%s", argv[3]);
          break;
        case 4030: // RS 32 256 overfetch with HBM setup
          chipNumber = 1;
          chipWidth = 72;
          message_config = new MSGConfig(4,4,0,0,1,chipWidth,chipNumber);
          // dg arg[4] = messageconfig (arg[1]*arg[4]+arg[2])
          dg = new DomainGroupDDR(DIMMcnt / 2, 4, chipNumber, chipWidth, 4, *message_config);
          ecc = new LargeRS(16);  // ino meta data
          ecc->setMaxRetiredBlkCount(512*1024);
          sprintf(filePrefix, "4030.4x8.RS16_256overfetchHBM.%s", argv[3]);
          break;
    */

  default:
    printf("Invalid ECC ID\n");
    exit(1);
  }
#endif /* AGECC */
  if (module == 0)
  {
    strcat(filePrefix, "ModuleA");
  }
  else if (module == 1)
  {
    strcat(filePrefix, "ModuleB");
  }
  else if (module == 2)
  {
    strcat(filePrefix, "ModuleC");
  }
  else if (module == 3)
  {
    strcat(filePrefix, "ModuleALL");
  }
  if (strcmp(argv[4], "S") == 0)
  {
    tester = new TesterSystem();
    scrubber = new PeriodicScrubber(8);

    string faults[argc - 5];
    for (int i = 5; i < argc; i++)
    {
      faults[i - 5] = string(argv[i]);
    }
    tester->test(dg, ecc, scrubber, atol(argv[2]), filePrefix, argc - 5,
                 faults);
    delete tester;
    delete scrubber;
  }
  else
  {
    tester = new TesterScenario();
    scrubber = new NoScrubber();

    string faults[argc - 4];
    for (int i = 4; i < argc; i++)
    {
      faults[i - 4] = string(argv[i]);
    }
    tester->test(dg, ecc, scrubber, atol(argv[2]), filePrefix, argc - 4,
                 faults);
    delete tester;
    delete scrubber;
  }
  delete ecc;
  delete dg;
  delete message_config;
  return 0;
}
