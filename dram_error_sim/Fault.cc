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
 * @file: Fault.cc
 * @author: Jungrae Kim <dale40@gmail.com>
 * Fault implementation
 */

#include <stdio.h>
#include <string.h>
#include <iostream>

#include "Config.hh"
#include "Fault.hh"
#include "common.hh"

//----------------------------------------------------------
std::default_random_engine randomGenerator;

//----------------------------------------------------------
Fault::~Fault(){
  detailed_faults.clear();
  delete[] bank_list;

}

Fault::Fault(std::string _name)
    : name(_name),
      fd(NULL),
      mask(SBIT_MASK),
      effective_mask(0),
      isInherent(true),
      isTransient(PERMANENT),
      numDQ(1),
      isSingleBeat(SINGLE_BEAT),
      isMultiRow(SINGLE_ROW),
      isMultiColumn(SINGLE_COLUMN),
      isChannel(NO_CHANNEL),
      affectedBlkCount(0),
      numInherentFaults(0),
      numBanks_perBeat(1),
      isMultipleBanks_perBeat(false) {}

Fault::Fault(FaultDomain *_fd, std::string _name, ADDR _mask, bool _isInherent,
             bool _isTransient, int _numDQ, bool _isSingleBeat,
             bool _isMultiRow, bool _isMultiColumn, bool _isChannel,
             unsigned long long _affectedBlkCount, int _banksperBeat)
    : fd(_fd),
      name(_name),
      mask(_mask),
      effective_mask(0),
      isInherent(_isInherent),
      isTransient(_isTransient),
      numDQ(_numDQ),
      isSingleBeat(_isSingleBeat),
      isMultiRow(_isMultiRow),
      isMultiColumn(_isMultiColumn),
      isChannel(_isChannel),
      affectedBlkCount(_affectedBlkCount),
      numInherentFaults(0) {

  numBanks_perBeat = _banksperBeat;
  if (_banksperBeat > 1)
    isMultipleBanks_perBeat = true;
  else
    isMultipleBanks_perBeat = false;
  bank_list = new int[_banksperBeat];
  for (int i=0;i<_banksperBeat;i++){
    bank_list[i] = i;
  }
  if(_mask == MBANK_MASK){
    if (_banksperBeat > 1){
      numBank_errors = rand()%(_banksperBeat-1) + 2;
    }
    else{
      numBank_errors = 1; 
    }
  }else if(_mask == MRANK_MASK){
    numBank_errors = _banksperBeat;
  }else{
    numBank_errors = 1;
  }

  if (isSingleBeat) {
    // start = end, count = 1
    beatStart = rand() % fd->getBeatHeight();
    beatEnd = beatStart;
    beatCount = 1;
  } else {
    beatStart = 0;
    beatEnd = fd->getBeatHeight() - 1;
    beatCount = fd->getBeatHeight();
  }
  update_pinpos();
  addr = RAND_MAX * ((ADDR)rand()) + rand();  // 64-bit
  cellFaultRate = .0;
}


void Fault::update_pinpos(bool group,int group_size){
  chipPos = rand() % fd->getChannelWidth() / fd->getChipWidth();
  if (numDQ ==2){
    int pin = rand() % (fd->getChipWidth()-1) + chipPos * fd->getChipWidth();    
    if (! group){
      pinPos[0] = pin;
      pinPos[1] = pin+1;
    } else {
      pinPos[0] = pin - (pin%2);
      pinPos[1] = pinPos[0] + 1;
    }
    return;
  }
  if (group && numDQ != fd->getChipWidth()){
    int ngroups = fd->getChipWidth()/group_size;
    // make shuffle list from 0 to ngroups-1
    int *shuffle_list = new int[ngroups];
    for (int i=0;i<ngroups;i++){
      shuffle_list[i] = i;
    }
    std::shuffle(shuffle_list,shuffle_list+ngroups,randomGenerator);
    for (int i=0;i<ngroups;i++){
      int group_start = shuffle_list[i]*group_size;
      for (int j=0;j<group_size;j++){
        pinPos[i*group_size+j] = group_start + j + chipPos * fd->getChipWidth();
      }
    }
    return;
  }

  for (int i = 0; i < std::max(numDQ,fd->getChipWidth()); i++) {
    if (numDQ == fd->getChipWidth()) {
      pinPos[i] = chipPos * fd->getChipWidth() + i;
    } else {
      bool isConflict;
      int pin;
      do {
        pin = rand() % fd->getChipWidth() + chipPos * fd->getChipWidth();
        isConflict = false;
        for (int j = 0; j < i; j++) {
          if (pinPos[j] == pin) {
            isConflict = true;
          }
        }
      } while (isConflict);
      pinPos[i] = pin;
    }
  }
}

Fault *Fault::genRandomFault(std::string type, FaultDomain *fd) {
  if (type == "b") {
    return new SingleBitFault(fd, false);
  } else if (type == "w") {
    return new SingleWordFault(fd, false, fd->getChipWidth());
  } else if (type == "w2") {
    return new SingleWordFault(fd, false, 2);
  } else if (type == "p") {
    return new SinglePinFault(fd, false);
  } else if (type == "c") {
    return new SingleChipFault(fd, false, fd->getChipWidth());
  } else if (type == "r") {
    return new ChannelFault(fd, false);
  } else if (type == "i1") {
    return new InherentFault(fd, 1e-1);
  } else if (type == "i2") {
    return new InherentFault(fd, 1e-2);
  } else if (type == "i3") {
    return new InherentFault(fd, 1e-3);
  } else if (type == "i4") {
    return new InherentFault(fd, 1e-4);
  } else if (type == "i5") {
    return new InherentFault(fd, 1e-5);
  } else if (type == "i6") {
    return new InherentFault(fd, 1e-6);
  } else if (type == "i7") {
    return new InherentFault(fd, 1e-7);
  } else if (type == "i8") {
    return new InherentFault(fd, 1e-8);
  } else if (type == "i9") {
    return new InherentFault(fd, 1e-9);
  } else if (type == "i10") {
    return new InherentFault(fd, 1e-10);
  } else if (type == "i11") {
    return new InherentFault(fd, 1e-11);
  } else if (type == "i3-1") {
    return new InherentFault2(fd, 1e-3, 1);
  } else if (type == "i4-1") {
    return new InherentFault2(fd, 1e-4, 1);
  } else if (type == "i5-1") {
    return new InherentFault2(fd, 1e-5, 1);
  } else if (type == "i6-1") {
    return new InherentFault2(fd, 1e-6, 1);
  } else if (type == "i7-1") {
    return new InherentFault2(fd, 1e-7, 1);
  } else if (type == "i8-1") {
    return new InherentFault2(fd, 1e-8, 1);
  } else if (type == "i9-1") {
    return new InherentFault2(fd, 1e-9, 1);
  } else if (type == "i10-1") {
    return new InherentFault2(fd, 1e-10, 1);
  } else if (type == "i3-2") {
    return new InherentFault2(fd, 1e-3, 2);
  } else if (type == "i4-2") {
    return new InherentFault2(fd, 1e-4, 2);
  } else if (type == "i5-2") {
    return new InherentFault2(fd, 1e-5, 2);
  } else if (type == "i6-2") {
    return new InherentFault2(fd, 1e-6, 2);
  } else if (type == "i7-2") {
    return new InherentFault2(fd, 1e-7, 2);
  } else if (type == "i8-2") {
    return new InherentFault2(fd, 1e-8, 2);
  } else if (type == "i9-2") {
    return new InherentFault2(fd, 1e-9, 2);
  } else if (type == "i10-2") {
    return new InherentFault2(fd, 1e-10, 2);
  } else if (type == "sbit-t") {
    return new SingleBitFault(fd, true);
  } else if (type == "sbit-p") {
    return new SingleBitFault(fd, false);
  } else if (type == "sword-1p-t") {
    return new SingleWordFault(fd, true, 1);
  } else if (type == "sword-1p-p") {
    return new SingleWordFault(fd, false, 1);
  } else if (type == "sword-3p-t") {
    return new SingleWordFault(fd, true, 3);
  } else if (type == "sword-3p-p") {
    return new SingleWordFault(fd, false, 3);
  } else if (type == "sword-4p-t") {
    return new SingleWordFault(fd, true, 4);
  } else if (type == "sword-4p-p") {
    return new SingleWordFault(fd, false, 4);
  } else if (type == "sword-np-t") {
    return new SingleWordFault(fd, true, fd->getChipWidth());
  } else if (type == "sword-np-p") {
    return new SingleWordFault(fd, false, fd->getChipWidth());
  } else if (type == "scol-1p-t") {
    return new SingleColumnFault(fd, true, 1);
  } else if (type == "scol-1p-p") {
    return new SingleColumnFault(fd, false, 1);
  } else if (type == "scol-2p-t") {
    return new SingleColumnFault(fd, true, 2);
  } else if (type == "scol-2p-p") {
    return new SingleColumnFault(fd, false, 2);
  } else if (type == "scol-3p-t") {
    return new SingleColumnFault(fd, true, 3);
  } else if (type == "scol-3p-p") {
    return new SingleColumnFault(fd, false, 3);
  } else if (type == "scol-4p-t") {
    return new SingleColumnFault(fd, true, 4);
  } else if (type == "scol-4p-p") {
    return new SingleColumnFault(fd, false, 4);
  } else if (type == "scol-np-t") {
    return new SingleColumnFault(fd, true, fd->getChipWidth());
  } else if (type == "scol-np-p") {
    return new SingleColumnFault(fd, false, fd->getChipWidth());
  } else if (type == "srow-1p-t") {
    return new SingleRowFault(fd, true, 1);
  } else if (type == "srow-1p-p") {
    return new SingleRowFault(fd, false, 1);
  } else if (type == "srow-2p-t") {
    return new SingleRowFault(fd, true, 2);
  } else if (type == "srow-2p-p") {
    return new SingleRowFault(fd, false, 2);
  } else if (type == "srow-3p-t") {
    return new SingleRowFault(fd, true, 3);
  } else if (type == "srow-3p-p") {
    return new SingleRowFault(fd, false, 3);
  } else if (type == "srow-4p-t") {
    return new SingleRowFault(fd, true, 4);
  } else if (type == "srow-4p-p") {
    return new SingleRowFault(fd, false, 4);
  } else if (type == "srow-np-t") {
    return new SingleRowFault(fd, true, fd->getChipWidth());
  } else if (type == "srow-np-p") {
    return new SingleRowFault(fd, false, fd->getChipWidth());
  } else if (type == "sbank-1p-t") {
    return new SingleBankFault(fd, true, 1);
  } else if (type == "sbank-1p-p") {
    return new SingleBankFault(fd, false, 1);
  } else if (type == "sbank-2p-t") {
    return new SingleBankFault(fd, true, 2);
  } else if (type == "sbank-2p-p") {
    return new SingleBankFault(fd, false, 2);
  } else if (type == "sbank-3p-t") {
    return new SingleBankFault(fd, true, 3);
  } else if (type == "sbank-3p-p") {
    return new SingleBankFault(fd, false, 3);
  } else if (type == "sbank-4p-t") {
    return new SingleBankFault(fd, true, 4);
  } else if (type == "sbank-4p-p") {
    return new SingleBankFault(fd, false, 4);
  } else if (type == "sbank-np-t") {
    return new SingleBankFault(fd, true, fd->getChipWidth());
  } else if (type == "sbank-np-p") {
    return new SingleBankFault(fd, false, fd->getChipWidth());
  } else if (type == "mbank-1p-t") {
    return new MultiBankFault(fd, true, 1);
  } else if (type == "mbank-1p-p") {
    return new MultiBankFault(fd, false, 1);
  } else if (type == "mbank-2p-t") {
    return new MultiBankFault(fd, true, 2);
  } else if (type == "mbank-2p-p") {
    return new MultiBankFault(fd, false, 2);
  } else if (type == "mbank-3p-t") {
    return new MultiBankFault(fd, true, 3);
  } else if (type == "mbank-3p-p") {
    return new MultiBankFault(fd, false, 3);
  } else if (type == "mbank-4p-t") {
    return new MultiBankFault(fd, true, 4);
  } else if (type == "mbank-4p-p") {
    return new MultiBankFault(fd, false, 4);
  } else if (type == "mbank-np-t") {
    return new MultiBankFault(fd, true, fd->getChipWidth());
  } else if (type == "mbank-np-p") {
    return new MultiBankFault(fd, false, fd->getChipWidth());
  } else if (type == "mrank-1p-t") {
    return new MultiRankFault(fd, true, 1);
  } else if (type == "mrank-1p-p") {
    return new MultiRankFault(fd, false, 1);
  } else if (type == "mrank-2p-t") {
    return new MultiRankFault(fd, true, 2);
  } else if (type == "mrank-2p-p") {
    return new MultiRankFault(fd, false, 2);
  } else if (type == "mrank-3p-t") {
    return new MultiRankFault(fd, true, 3);
  } else if (type == "mrank-3p-p") {
    return new MultiRankFault(fd, false, 3);
  } else if (type == "mrank-4p-t") {
    return new MultiRankFault(fd, true, 4);
  } else if (type == "mrank-4p-p") {
    return new MultiRankFault(fd, false, 4);
  } else if (type == "mrank-np-t") {
    return new MultiRankFault(fd, true, fd->getChipWidth());
  } else if (type == "mrank-np-p") {
    return new MultiRankFault(fd, false, fd->getChipWidth());
  } else if (type == "bank_control_bank_8diff-t" || type == "bank_control-t") {
    return new BankPatternFault(fd, true,0);
    //return new BankPatternFault(fd, false,0);
  } else if (type == "bank_control_bank_8diff-p" || type == "bank_control-p") {
    return new BankPatternFault(fd, false,0);
  } else if (type == "bank_control_independent_bank-t") {
    assert(0); // Not implemented for independant faults.
    return new BankPatternFault(fd, true,1);
  } else if (type == "bank_control_independent_bank-p") {
    assert(0); // Not implemented for independant faults.
    return new BankPatternFault(fd, false,1);
  } else if (type == "bank_control_manybanks-t") {
    return new BankPatternFault(fd, true, 2);
    //return new BankPatternFault(fd, false, 2);
  } else if (type == "bank_control_manybanks-p") {
    return new BankPatternFault(fd, false,2);
  } else if (type == "bank_control_two_banks_not8diff-t") {
    return new BankPatternFault(fd, true,3);
    //return new BankPatternFault(fd, false,3);
  } else if (type == "bank_control_two_banks_not8diff-p") {
    return new BankPatternFault(fd, false,3);
  } else if (type == "decoder_multi_col-t") {
    return new CDECFault(fd, true,0);
  } else if (type == "decoder_multi_col-p") {
    return new CDECFault(fd, false,0);
  } else if (type == "decoder_single_col-t") {
    return new CDECFault(fd, true,1);
  } else if (type == "decoder_single_col-p") {
    return new CDECFault(fd, false,1);
  } else if (type == "local_wordline-t" || type == "consequtive_rows-t") {
    return new LocalWordlineFault(fd, true, 1);
    //return new LocalWordlineFault(fd, false, 2);
  } else if (type == "local_wordline-p" || type == "consequtive_rows-p") {
    return new LocalWordlineFault(fd, false,1);
  } else if (type == "local_wordline_two_clusters-t") {
    return new LocalWordlineFault(fd, true,2);
   // return new LocalWordlineFault(fd, false,1);

  } else if (type == "local_wordline_two_clusters-p") {
    return new LocalWordlineFault(fd, false,2);
  } else if (type == "lwl_sel-t") {
    //return new RDECFault(fd, false,0);
    return new RDECFault(fd, true,0);
  } else if (type == "lwl_sel-p") {
    return new RDECFault(fd, false,0);
  } else if (type == "lwl_sel_column_related-t") {
    return new SingleBankFault(fd, true,fd->getChipWidth());
  } else if (type == "lwl_sel_column_related-p") {
    return new SingleBankFault(fd, false,fd->getChipWidth());
  } else if (type == "lwl_sel_random_bit-t") {
    return new SingleWordFault(fd, true,fd->getChipWidth());
  } else if (type == "lwl_sel_random_bit-p") {
    return new SingleWordFault(fd, true,fd->getChipWidth());
  } else if (type == "lwl_sel2-t") {
    //return new RDECFault(fd, false,0);
    return new RDECFault(fd, true,0);
  } else if (type == "lwl_sel2-p") {
    return new RDECFault(fd, false,0);
  } else if (type == "lwl_sel2_potential_col-t") {
    return new SingleBankFault(fd, true,fd->getChipWidth());
  } else if (type == "lwl_sel2_potential_col-p") {
    return new SingleBankFault(fd, false,fd->getChipWidth());
  }
  if (type == "multi_rank-t") {
    return new MultiRankFault(fd, true,4);
  } else if (type == "multi_rank-p") {
    return new MultiRankFault(fd, false,4);
  }
  
  if (type == "multi_rank_random_bits-t") {
    return new DistBitFault(fd, true, 0);
  } else if (type == "multi_rank_random_bits-p") {
    return new DistBitFault(fd, false, 0);
  }
  
  if (type == "multi_rank_two_inpendent-t") {
    assert(0);
    //return new multi_rank_two_inpendentFault(fd, true);
  } else if (type == "multi_rank_two_inpendent-p") {
    assert(0); 
    //return new multi_rank_two_inpendentFault(fd, false);
  }
  
  if (type == "multi_socket-t") {
    return new MultiRankFault(fd, true, 4);
  } else if (type == "multi_socket-p") {
    return new MultiRankFault(fd, false, 4);
  }
  
  if (type == "multi_socket_could_justone-t") {
    return new MultiModuleFault(fd, true,0);
  } else if (type == "multi_socket_could_justone-p") {
    return new MultiModuleFault(fd, false,0);
  }
  
  if (type == "multi_socket_true_socket-t") {
    return new MultiRankFault(fd, true, 4);
  } else if (type == "multi_socket_true_socket-p") {
    return new MultiRankFault(fd, false, 4);
  }
  
  if (type == "multi_socket_two_independent-t") {
    assert(0);
    //return new multi_socket_two_independentFault(fd, true);
  } else if (type == "multi_socket_two_independent-p") {
    assert(0);
    //return new multi_socket_two_independentFault(fd, false);
  }
  
  if (type == "multiple_single_bit_failures_-t") {
    return new SingleBitFault(fd, true);
  } else if (type == "multiple_single_bit_failures_-p") {
    return new SingleBitFault(fd, false);
  }
  
  if (type == "mutli_csls_column_and_bank-t") {
    //return new CSLFault(fd, false, 0);
    return new CSLFault(fd, true, 0);
  } else if (type == "mutli_csls_column_and_bank-p") {
    return new CSLFault(fd, false, 0);
  }
  
  if (type == "mutli_csls_random_bank_row-t") {
    return new CSLFault(fd, false,0);
    //return new CSLFault(fd, true,0);
  } else if (type == "mutli_csls_random_bank_row-p") {
    return new CSLFault(fd, false,0);
  }
  
  if (type == "mutli_csls_random_bits-t") {
    //return new CSLFault(fd, false,0);
    return new CSLFault(fd, true,0);
  } else if (type == "mutli_csls_random_bits-p") {
    return new CSLFault(fd, false,0);
  }
  
  if (type == "mutli_csls_row_and_bits-t") {
    //return new CSLFault(fd, false,0);
    return new CSLFault(fd, true,0);
  } else if (type == "mutli_csls_row_and_bits-p") {
    return new CSLFault(fd, false,0);
  }
  
  if (type == "mutli_csls_row_related-t") {
    //return new CSLFault(fd, false,0);
    return new CSLFault(fd, true,0);
  } else if (type == "mutli_csls_row_related-p") {
    return new CSLFault(fd, false,0);
  }

  if (type == "mutli_csls-t") {
    //return new CSLFault(fd, false,0);
    return new CSLFault(fd, true,0);
  } else if (type == "mutli_csls-p") {
    return new CSLFault(fd, false,0);
  }
  
  if (type == "not_clustered_multi_bank-t") {
    return new MultiBankFault(fd, true,fd->getChipWidth());
  } else if (type == "not_clustered_multi_bank-p") {
    return new MultiBankFault(fd, false,fd->getChipWidth());
  }
  
  if (type == "not_clustered_multi_bank_two_independent-t") {
    assert(0);
    //return new not_clustered_multi_bank_two_independentFault(fd, true);
  } else if (type == "not_clustered_multi_bank_two_independent-p") {
    assert(0);
    //return new not_clustered_multi_bank_two_independentFault(fd, false);
  }
  
  if (type == "not_clustered_single_bank-t") {
    return new SingleBankFault(fd, true,fd->getChipWidth());
  } else if (type == "not_clustered_single_bank-p") {
    return new SingleBankFault(fd, false,fd->getChipWidth());
  }
  
  if (type == "not_clustered_single_column-t") {
    return new SingleColumnFault(fd, true,fd->getChipWidth());
  } else if (type == "not_clustered_single_column-p") {
    return new SingleColumnFault(fd, false,fd->getChipWidth());
  }
  
  if (type == "row_decoder-t") {
    //return new RDECFault(fd, false,1);
    return new RDECFault(fd, true,1);
  } else if (type == "row_decoder-p") {
    return new RDECFault(fd, false,1);
  }
  
  if (type == "single_csl_bank-t") {
    //return new CSLFault(fd, false,1);
    return new CSLFault(fd, true,1);
  } else if (type == "single_csl_bank-p") {
    return new CSLFault(fd, false,1);
  }
  
  if (type == "single_csl_column-t"|| type == "potential_csl_column-t") {
    //return new CSLFault(fd, false,2);
    return new CSLFault(fd, true,2);
  } else if (type == "single_csl_column-p"|| type == "potential_csl_column-p") {
    return new CSLFault(fd, false,2);
  }
  
  if (type == "single_sense_amp-t" || type == "potential_sense_amp-t") {
    //return new BLSAFault(fd, false);
    return new BLSAFault(fd, true);
  } else if (type == "single_sense_amp-p" || type == "potential_sense_amp-p") {
    return new BLSAFault(fd, false);
  }
  
  if (type == "subarray_row_decoder-t") {
    //return new SWDFault(fd, false,0);
    return new SWDFault(fd, true,0);
  } else if (type == "subarray_row_decoder-p") {
    return new SWDFault(fd, false,0);
  }
  
  if (type == "subarray_row_decoder_two_clusters-t") {
    return new SWDFault(fd, false,1);
    //return new SWDFault(fd, true,1);
  } else if (type == "subarray_row_decoder_two_clusters-p") {
    return new SWDFault(fd, false,1);
  }
  
  if (type == "two_row_overlap-t") {
    assert(0);
    //return new two_row_overlapFault(fd, true);
  } else if (type == "two_row_overlap-p") {
    assert(0);
    //return new two_row_overlapFault(fd, false);
  }

  double r = (double)rand()/RAND_MAX;

  if (type =="scol-t"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.8580) return new SingleColumnFault(fd, true,1);
      else return new SingleColumnFault(fd, true,pins);
    }else {
      if (r<0.8580) return new SingleColumnFault(fd, true,1);
      else if (r<0.8580+0.0330) return new SingleColumnFault(fd, true,2);
      else if (r<0.8580+0.0330+0.0080) return new SingleColumnFault(fd, true,3);
      else return new SingleColumnFault(fd, true,4);
    }
  } else if (type =="scol-p"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.8580) return new SingleColumnFault(fd, false,1);
      else return new SingleColumnFault(fd, false,pins);
    }else {
      if (r<0.8580) return new SingleColumnFault(fd, false,1);
      else if (r<0.8580+0.0330) return new SingleColumnFault(fd, false,2);
      else if (r<0.8580+0.0330+0.0080) return new SingleColumnFault(fd, false,3);
      else return new SingleColumnFault(fd, false,4);
    }
  } else if (type =="srow-t"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.3110) return new SingleRowFault(fd, true,1);
      else return new SingleRowFault(fd, true,pins);
    }else {
      if (r<0.3110) return new SingleRowFault(fd, true,1);
      else if (r<0.3110+0.6680) return new SingleRowFault(fd, true,2);
      else if (r<0.3110+0.6680+0.0140) return new SingleRowFault(fd, true,3);
      else return new SingleRowFault(fd, true,4);
    }
  } else if (type =="srow-p"){
    if (fd->HBM_setup){ 
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.3110) return new SingleRowFault(fd, false,1);
      else return new SingleRowFault(fd, false,pins);
    } else {
      if (r<0.3110) return new SingleRowFault(fd, false,1);
      else if (r<0.3110+0.6680) return new SingleRowFault(fd, false,2);
      else if (r<0.3110+0.6680+0.0140) return new SingleRowFault(fd, false,3);
      else return new SingleRowFault(fd, false,4);
    }
  } else if (type =="sbank-t"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.5550) return new SingleBankFault(fd, true,4);
      else return new SingleBankFault(fd, true,pins);
    }else {
      if (r<0.5550) return new SingleBankFault(fd, true,4);
      else if (r<0.5550+0.2300) return new SingleBankFault(fd, true,4);
      else if (r<0.5550+0.2300+0.0380) return new SingleBankFault(fd, true,4);
      else return new SingleBankFault(fd, true,4);
    }
  } else if (type =="sbank-p"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.5550) return new SingleBankFault(fd, false,4);
      else return new SingleBankFault(fd, false,pins);
    } else {
      if (r<0.5550) return new SingleBankFault(fd, false,4);
      else if (r<0.5550+0.2300) return new SingleBankFault(fd, false,4);
      else if (r<0.5550+0.2300+0.0380) return new SingleBankFault(fd, false,4);
      else return new SingleBankFault(fd, false,4);
    }
  } else if (type =="mbank-t"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.1750) return new MultiBankFault(fd, true,1);
      else return new MultiBankFault(fd, true,pins);
    }else {
      if (r<0.1750) return new MultiBankFault(fd, true,4);
      else if (r<0.1750+0.3330) return new MultiBankFault(fd, true,4);
      else if (r<0.1750+0.3330+0.0350) return new MultiBankFault(fd, true,4);
      else return new MultiBankFault(fd, true,4);
    }
  } else if (type =="mbank-p"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.1750) return new MultiBankFault(fd, false,1);
      else return new MultiBankFault(fd, false,pins);
      }else {
      if (r<0.1750) return new MultiBankFault(fd, false,4);
      else if (r<0.1750+0.3330) return new MultiBankFault(fd, false,4);
      else if (r<0.1750+0.3330+0.0350) return new MultiBankFault(fd, false,4);
      else return new MultiBankFault(fd, false,4);
    }
  } else if (type =="mrank-t"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.0750) return new MultiRankFault(fd, true,1);
      else return new MultiRankFault(fd, true,pins);
    }else {
      if (r<0.0750) return new MultiRankFault(fd, true,4);
      else if (r<0.0750+0.0710) return new MultiRankFault(fd, true,4);
      else if (r<0.0750+0.0710+0.0180) return new MultiRankFault(fd, true,4);
      else return new MultiRankFault(fd, true,4);
    }
  } else if (type =="mrank-p"){
    if (fd->HBM_setup){
      int pins = (rand() % fd->getChannelWidth()-1) + 2;
      if (r<0.0750) return new MultiRankFault(fd, false,1);
      else return new MultiRankFault(fd, false,pins);
    }else {
      if (r<0.0750) return new MultiRankFault(fd, false,4);
      else if (r<0.0750+0.0710) return new MultiRankFault(fd, false,4);
      else if (r<0.0750+0.0710+0.0180) return new MultiRankFault(fd, false,4);
      else return new MultiRankFault(fd, false,4);
    }
  }

  assert(0);
  return NULL;
  
}

//----------------------------------------------------------
