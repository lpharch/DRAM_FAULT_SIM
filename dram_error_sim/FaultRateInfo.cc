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

#include "FaultRateInfo.hh"
#include "Tester.hh"
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
// global variable, defined in main.cc
extern int module;
bool ignore_external_faults = false;



double getMultiplier(const std::string &faultName)
{
  std::map<std::string, double> multiplierMap = {
    {"bank_control_bank_8diff", numofBanks},
    {"decoder_multi_col", CDEC_per_chip},
    {"decoder_single_col", CDEC_per_chip},
    {"local_wordline", WLD_per_chip},
    {"local_wordline_two_clusters", WLD_per_chip},
    {"lwl_sel", RDEC_per_chip},
    {"lwl_sel2", RDEC_per_chip},
    {"multiple_single_bit_failures_", BITS_per_chip},
    {"mutli_csls_", numofBanks},
    {"not_clustered_single_bank", numofBanks},
    {"not_clustered_single_column", BLSA_per_chip},
    {"row_decoder", RDEC_per_chip},
    {"single_csl_", CSL_per_chip},
    {"single_sense_amp", BLSA_per_chip},
    {"subarray_row_decoder", RDEC_SUBBANK_per_chip},
    {"potential_sense_amp", BLSA_per_chip},
    {"potential_csl_column", CSL_per_chip},
    {"consequtive_rows", WLD_per_chip}};
    
  for (const auto &[key, val] : multiplierMap)
  {
    if (faultName.find(key) != std::string::npos)
    {
      return val;
    }
  }
  return 1.0; // default
}

DefaultFaultRateInfo::DefaultFaultRateInfo(bool HBMSETUP, bool detailed_errormap) : FaultRateInfo()
{
  this->setDetailedError(detailed_errormap);

  if (module == 0)
  {
    addFaultRate("bank_control_bank_8diff-p", 5.082E-04 * numofBanks FIT);
    addFaultRate("bank_control_bank_8diff-t", 0.000E+00 * numofBanks FIT);
    addFaultRate("decoder_multi_col-p", 5.082E-04 * CDEC_per_chip FIT);
    addFaultRate("decoder_multi_col-t", 4.523E-02 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-p", 0.000E+00 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-t", 1.779E-02 * CDEC_per_chip FIT);
    addFaultRate("local_wordline-p", 1.417E-03 * WLD_per_chip FIT);
    addFaultRate("local_wordline-t", 1.668E-04 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-p", 7.147E-05 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-t", 1.147E-05 * WLD_per_chip FIT);
    addFaultRate("lwl_sel-p", 5.082E-04 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel-t", 1.016E-03 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel_column_related-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_column_related-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2-p", 5.082E-04 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2-t", 0.000E+00 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2_potential_col-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2_potential_col-t", 0.000E+00 * 1 FIT);
    addFaultRate("multiple_single_bit_failures_-p", 9.378E+01 * BITS_per_chip FIT);
    addFaultRate("multiple_single_bit_failures_-t", 5.356E+01 * BITS_per_chip FIT);
    addFaultRate("mutli_csls_column_and_bank-p", 8.132E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_column_and_bank-t", 6.099E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-p", 2.541E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-t", 3.558E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-p", 7.115E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-t", 5.082E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-p", 4.574E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-t", 1.016E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-p", 3.558E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-t", 4.066E-03 * numofBanks FIT);
    addFaultRate("not_clustered_multi_bank-p", 2.765E-01 * 1 FIT);
    addFaultRate("not_clustered_multi_bank-t", 2.277E-01 * 1 FIT);
    // addFaultRate("not_clustered_multi_bank_two_independent-p",7.481E-01*1 FIT);	addFaultRate("not_clustered_multi_bank_two_independent-t",2.440E-02*1 FIT);
    addFaultRate("not_clustered_single_bank-p", 1.016E-02 * numofBanks FIT);
    addFaultRate("not_clustered_single_bank-t", 6.607E-03 * numofBanks FIT);
    addFaultRate("not_clustered_single_column-p", 5.816E-09 * BLSA_per_chip FIT);
    addFaultRate("not_clustered_single_column-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("row_decoder-p", 1.220E-02 * RDEC_per_chip FIT);
    addFaultRate("row_decoder-t", 1.372E-02 * RDEC_per_chip FIT);
    addFaultRate("single_csl_bank-p", 1.842E-03 * CSL_per_chip FIT);
    addFaultRate("single_csl_bank-t", 6.353E-04 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-p", 6.353E-04 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-t", 1.906E-04 * CSL_per_chip FIT);
    addFaultRate("single_sense_amp-p", 3.141E-07 * BLSA_per_chip FIT);
    addFaultRate("single_sense_amp-t", 6.786E-08 * BLSA_per_chip FIT);
    addFaultRate("subarray_row_decoder-p", 6.658E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder-t", 1.321E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-p", 2.846E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-t", 1.016E-02 * RDEC_SUBBANK_per_chip FIT);
    // addFaultRate("two_row_overlap-p",2.277E-01*1 FIT);	addFaultRate("two_row_overlap-t",2.440E-02*1 FIT);
    addFaultRate("potential_sense_amp-p", 5.816E-08 * BLSA_per_chip FIT);
    addFaultRate("potential_sense_amp-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("bank_control-p", 0.000E+00 * 1 FIT);
    addFaultRate("bank_control-t", 0.000E+00 * 1 FIT);
    addFaultRate("potential_csl_column-p", 0.000E+00 * CSL_per_chip FIT);
    addFaultRate("potential_csl_column-t", 0.000E+00 * CSL_per_chip FIT);
    addFaultRate("mutli_csls-p", 3.253E-02 * 1 FIT);
    addFaultRate("mutli_csls-t", 0.000E+00 * 1 FIT);
    addFaultRate("consequtive_rows-p", 8.029E-05 * WLD_per_chip FIT);
    addFaultRate("consequtive_rows-t", 0.000E+00 * WLD_per_chip FIT);
  }
  else if (module == 1)
  {
    addFaultRate("bank_control_bank_8diff-p", 1.271E-02 * numofBanks FIT);
    addFaultRate("bank_control_bank_8diff-t", 0.000E+00 * numofBanks FIT);
    addFaultRate("decoder_multi_col-p", 0.000E+00 * CDEC_per_chip FIT);
    addFaultRate("decoder_multi_col-t", 9.656E-03 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-p", 5.082E-04 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-t", 1.448E-01 * CDEC_per_chip FIT);
    addFaultRate("local_wordline-p", 9.088E-05 * WLD_per_chip FIT);
    addFaultRate("local_wordline-t", 3.882E-05 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-p", 8.823E-07 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-t", 2.647E-06 * WLD_per_chip FIT);
    addFaultRate("lwl_sel-p", 3.303E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel-t", 1.530E-01 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel_column_related-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_column_related-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2-p", 1.525E-03 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2-t", 7.623E-03 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2_potential_col-p", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2_potential_col-t", 0.000E+00 * 1 FIT);
    addFaultRate("multiple_single_bit_failures_-p", 1.271E+01 * BITS_per_chip FIT);
    addFaultRate("multiple_single_bit_failures_-t", 9.913E+00 * BITS_per_chip FIT);
    addFaultRate("mutli_csls_column_and_bank-p", 9.656E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_column_and_bank-t", 6.099E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-p", 5.082E-04 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-t", 5.082E-04 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-p", 4.066E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-t", 3.558E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-p", 1.016E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-t", 5.082E-04 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-p", 5.082E-04 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-t", 2.033E-03 * numofBanks FIT);
    addFaultRate("not_clustered_multi_bank-p", 4.879E-02 * 1 FIT);
    addFaultRate("not_clustered_multi_bank-t", 8.132E-02 * 1 FIT);
    // addFaultRate("not_clustered_multi_bank_two_independent-p",3.334E-01*1 FIT);	addFaultRate("not_clustered_multi_bank_two_independent-t",1.626E-02*1 FIT);
    addFaultRate("not_clustered_single_bank-p", 1.016E-02 * numofBanks FIT);
    addFaultRate("not_clustered_single_bank-t", 9.148E-03 * numofBanks FIT);
    addFaultRate("not_clustered_single_column-p", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("not_clustered_single_column-t", 3.877E-09 * BLSA_per_chip FIT);
    addFaultRate("row_decoder-p", 1.016E-03 * RDEC_per_chip FIT);
    addFaultRate("row_decoder-t", 5.082E-03 * RDEC_per_chip FIT);
    addFaultRate("single_csl_bank-p", 8.259E-04 * CSL_per_chip FIT);
    addFaultRate("single_csl_bank-t", 2.916E-02 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-p", 6.353E-05 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-t", 1.938E-02 * CSL_per_chip FIT);
    addFaultRate("single_sense_amp-p", 1.512E-07 * BLSA_per_chip FIT);
    addFaultRate("single_sense_amp-t", 3.017E-06 * BLSA_per_chip FIT);
    addFaultRate("subarray_row_decoder-p", 7.623E-03 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder-t", 1.118E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-p", 3.049E-03 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-t", 6.607E-03 * RDEC_SUBBANK_per_chip FIT);
    // addFaultRate("two_row_overlap-p",0.000E+00*1 FIT);	addFaultRate("two_row_overlap-t",0.000E+00*1 FIT);
    addFaultRate("potential_sense_amp-p", 5.739E-07 * BLSA_per_chip FIT);
    addFaultRate("potential_sense_amp-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("bank_control-p", 1.057E-01 * 1 FIT);
    addFaultRate("bank_control-t", 0.000E+00 * 1 FIT);
    addFaultRate("potential_csl_column-p", 5.082E-03 * CSL_per_chip FIT);
    addFaultRate("potential_csl_column-t", 0.000E+00 * CSL_per_chip FIT);
    addFaultRate("mutli_csls-p", 0.000E+00 * 1 FIT);
    addFaultRate("mutli_csls-t", 0.000E+00 * 1 FIT);
    addFaultRate("consequtive_rows-p", 1.059E-05 * WLD_per_chip FIT);
    addFaultRate("consequtive_rows-t", 0.000E+00 * WLD_per_chip FIT);
  }
  else if (module == 2)
  {
    addFaultRate("bank_control_bank_8diff-p", 2.541E-03 * numofBanks FIT);
    addFaultRate("bank_control_bank_8diff-t", 0.000E+00 * numofBanks FIT);
    addFaultRate("decoder_multi_col-p", 4.574E-03 * CDEC_per_chip FIT);
    addFaultRate("decoder_multi_col-t", 0.000E+00 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-p", 1.016E-03 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-t", 5.082E-04 * CDEC_per_chip FIT);
    addFaultRate("local_wordline-p", 4.173E-04 * WLD_per_chip FIT);
    addFaultRate("local_wordline-t", 6.353E-05 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-p", 3.626E-04 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-t", 2.647E-05 * WLD_per_chip FIT);
    addFaultRate("lwl_sel-p", 1.880E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel-t", 1.042E-01 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel_column_related-p", 1.626E-02 * 1 FIT);
    addFaultRate("lwl_sel_column_related-t", 8.132E-03 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-p", 8.132E-03 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2-p", 2.541E-03 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2-t", 4.015E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2_potential_col-p", 3.253E-02 * 1 FIT);
    addFaultRate("lwl_sel2_potential_col-t", 0.000E+00 * 1 FIT);
    addFaultRate("multiple_single_bit_failures_-p", 1.154E+01 * BITS_per_chip FIT);
    addFaultRate("multiple_single_bit_failures_-t", 1.195E+01 * BITS_per_chip FIT);
    addFaultRate("mutli_csls_column_and_bank-p", 1.931E-02 * numofBanks FIT);
    addFaultRate("mutli_csls_column_and_bank-t", 9.656E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-p", 2.033E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-t", 4.066E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-p", 1.525E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-t", 7.623E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-p", 3.558E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-t", 2.033E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-p", 5.591E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-t", 2.033E-03 * numofBanks FIT);
    addFaultRate("not_clustered_multi_bank-p", 1.464E-01 * 1 FIT);
    addFaultRate("not_clustered_multi_bank-t", 2.358E-01 * 1 FIT);
    // addFaultRate("not_clustered_multi_bank_two_independent-p",6.668E-01*1 FIT);	addFaultRate("not_clustered_multi_bank_two_independent-t",8.945E-02*1 FIT);
    addFaultRate("not_clustered_single_bank-p", 8.132E-03 * numofBanks FIT);
    addFaultRate("not_clustered_single_bank-t", 1.372E-02 * numofBanks FIT);
    addFaultRate("not_clustered_single_column-p", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("not_clustered_single_column-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("row_decoder-p", 1.779E-02 * RDEC_per_chip FIT);
    addFaultRate("row_decoder-t", 1.423E-02 * RDEC_per_chip FIT);
    addFaultRate("single_csl_bank-p", 1.334E-03 * CSL_per_chip FIT);
    addFaultRate("single_csl_bank-t", 4.873E-02 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-p", 9.529E-04 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-t", 2.109E-02 * CSL_per_chip FIT);
    addFaultRate("single_sense_amp-p", 1.124E-07 * BLSA_per_chip FIT);
    addFaultRate("single_sense_amp-t", 5.622E-08 * BLSA_per_chip FIT);
    addFaultRate("subarray_row_decoder-p", 3.558E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder-t", 1.423E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-p", 5.692E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-t", 1.169E-02 * RDEC_SUBBANK_per_chip FIT);
    // addFaultRate("two_row_overlap-p",4.879E-02*1 FIT);	addFaultRate("two_row_overlap-t",0.000E+00*1 FIT);
    addFaultRate("potential_sense_amp-p", 1.881E-07 * BLSA_per_chip FIT);
    addFaultRate("potential_sense_amp-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("bank_control-p", 1.626E-02 * 1 FIT);
    addFaultRate("bank_control-t", 0.000E+00 * 1 FIT);
    addFaultRate("potential_csl_column-p", 4.256E-03 * CSL_per_chip FIT);
    addFaultRate("potential_csl_column-t", 0.000E+00 * CSL_per_chip FIT);
    addFaultRate("mutli_csls-p", 1.382E-01 * 1 FIT);
    addFaultRate("mutli_csls-t", 0.000E+00 * 1 FIT);
    addFaultRate("consequtive_rows-p", 8.029E-05 * WLD_per_chip FIT);
    addFaultRate("consequtive_rows-t", 0.000E+00 * WLD_per_chip FIT);
  }
  else if (module == 3)
  {
    addFaultRate("bank_control_bank_8diff-p", 5.252E-03 * numofBanks FIT);
    addFaultRate("bank_control_bank_8diff-t", 0.000E+00 * numofBanks FIT);
    addFaultRate("decoder_multi_col-p", 1.694E-03 * CDEC_per_chip FIT);
    addFaultRate("decoder_multi_col-t", 1.830E-02 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-p", 5.082E-04 * CDEC_per_chip FIT);
    addFaultRate("decoder_single_col-t", 5.438E-02 * CDEC_per_chip FIT);
    addFaultRate("local_wordline-p", 6.418E-04 * WLD_per_chip FIT);
    addFaultRate("local_wordline-t", 8.970E-05 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-p", 1.450E-04 * WLD_per_chip FIT);
    addFaultRate("local_wordline_two_clusters-t", 1.353E-05 * WLD_per_chip FIT);
    addFaultRate("lwl_sel-p", 1.745E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel-t", 8.606E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel_column_related-p", 5.421E-03 * 1 FIT);
    addFaultRate("lwl_sel_column_related-t", 2.711E-03 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-p", 2.711E-03 * 1 FIT);
    addFaultRate("lwl_sel_random_bit-t", 0.000E+00 * 1 FIT);
    addFaultRate("lwl_sel2-p", 1.525E-03 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2-t", 1.592E-02 * RDEC_per_chip FIT);
    addFaultRate("lwl_sel2_potential_col-p", 1.084E-02 * 1 FIT);
    addFaultRate("lwl_sel2_potential_col-t", 0.000E+00 * 1 FIT);
    addFaultRate("multiple_single_bit_failures_-p", 3.934E+01 * BITS_per_chip FIT);
    addFaultRate("multiple_single_bit_failures_-t", 2.514E+01 * BITS_per_chip FIT);
    addFaultRate("mutli_csls_column_and_bank-p", 1.237E-02 * numofBanks FIT);
    addFaultRate("mutli_csls_column_and_bank-t", 7.285E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-p", 1.694E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bank_row-t", 2.711E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-p", 4.235E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_random_bits-t", 5.421E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-p", 3.049E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_and_bits-t", 1.186E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-p", 3.219E-03 * numofBanks FIT);
    addFaultRate("mutli_csls_row_related-t", 2.711E-03 * numofBanks FIT);
    addFaultRate("not_clustered_multi_bank-p", 1.572E-01 * 1 FIT);
    addFaultRate("not_clustered_multi_bank-t", 1.816E-01 * 1 FIT);
    // addFaultRate("not_clustered_multi_bank_two_independent-p",5.828E-01*1 FIT);	addFaultRate("not_clustered_multi_bank_two_independent-t",4.337E-02*1 FIT);
    addFaultRate("not_clustered_single_bank-p", 9.487E-03 * numofBanks FIT);
    addFaultRate("not_clustered_single_bank-t", 9.826E-03 * numofBanks FIT);
    addFaultRate("not_clustered_single_column-p", 1.939E-09 * BLSA_per_chip FIT);
    addFaultRate("not_clustered_single_column-t", 1.292E-09 * BLSA_per_chip FIT);
    addFaultRate("row_decoder-p", 1.033E-02 * RDEC_per_chip FIT);
    addFaultRate("row_decoder-t", 1.101E-02 * RDEC_per_chip FIT);
    addFaultRate("single_csl_bank-p", 1.334E-03 * CSL_per_chip FIT);
    addFaultRate("single_csl_bank-t", 2.617E-02 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-p", 5.506E-04 * CSL_per_chip FIT);
    addFaultRate("single_csl_column-t", 1.355E-02 * CSL_per_chip FIT);
    addFaultRate("single_sense_amp-p", 1.926E-07 * BLSA_per_chip FIT);
    addFaultRate("single_sense_amp-t", 1.047E-06 * BLSA_per_chip FIT);
    addFaultRate("subarray_row_decoder-p", 3.659E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder-t", 1.288E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-p", 2.948E-02 * RDEC_SUBBANK_per_chip FIT);
    addFaultRate("subarray_row_decoder_two_clusters-t", 9.487E-03 * RDEC_SUBBANK_per_chip FIT);
    // addFaultRate("two_row_overlap-p",9.216E-02*1 FIT);	addFaultRate("two_row_overlap-t",8.132E-03*1 FIT);
    addFaultRate("potential_sense_amp-p", 2.734E-07 * BLSA_per_chip FIT);
    addFaultRate("potential_sense_amp-t", 0.000E+00 * BLSA_per_chip FIT);
    addFaultRate("bank_control-p", 4.066E-02 * 1 FIT);
    addFaultRate("bank_control-t", 0.000E+00 * 1 FIT);
    addFaultRate("potential_csl_column-p", 3.113E-03 * CSL_per_chip FIT);
    addFaultRate("potential_csl_column-t", 0.000E+00 * CSL_per_chip FIT);
    addFaultRate("mutli_csls-p", 5.692E-02 * 1 FIT);
    addFaultRate("mutli_csls-t", 0.000E+00 * 1 FIT);
    addFaultRate("consequtive_rows-p", 5.706E-05 * WLD_per_chip FIT);
    addFaultRate("consequtive_rows-t", 0.000E+00 * WLD_per_chip FIT);
  }
  else if (module == 4)
  {
    std::ifstream inputFile("input_FIT.conf");
    if (!inputFile.is_open())
    {
      std::cerr << "Error: Unable to open input_FIT.conf" << std::endl;
      return;
    }

    std::string line;
    int lineNumber = 0; // for reporting errors
    while (std::getline(inputFile, line))
    {
      lineNumber++;

      std::stringstream ss(line);
      std::string faultName;
      double faultValue;

      if (std::getline(ss, faultName, ',') && (ss >> faultValue))
      {

        // Validate fault name: (for example, we can check if it starts with quotes and ends with quotes)
        if (faultName.front() != '"' || faultName.back() != '"')
        {
          std::cerr << "Error on line " << lineNumber << ": Invalid fault name format." << std::endl;
          continue; // skip this line and move to the next
        }

        // Validate faultValue range, if necessary. (e.g. ensuring it's non-negative)
        if (faultValue < 0)
        {
          std::cerr << "Error on line " << lineNumber << ": Negative fault value detected." << std::endl;
          continue; // skip this line and move to the next
        }

        // Removing quotes from faultName
        faultName = faultName.substr(1, faultName.length() - 2);
        faultValue *= getMultiplier(faultName);

        addFaultRate(faultName, faultValue FIT);
      }
      else
      {
        std::cerr << "Error on line " << lineNumber << ": Malformed input data." << std::endl;
      }
    }

    inputFile.close();
  }

  if (!HBMSETUP)
  {
    if (module == 0)
    {
      if (!ignore_external_faults)
      {
        addFaultRate("multi_rank-p", 9.595E-01 * 1 FIT);
        addFaultRate("multi_rank-t", 1.952E-01 * 1 FIT);
        addFaultRate("multi_rank_random_bits-p", 8.132E-03 * 1 FIT);
        addFaultRate("multi_rank_random_bits-t", 3.253E-02 * 1 FIT);
        // addFaultRate("multi_rank_two_inpendent-p",1.708E-01*1 FIT);	addFaultRate("multi_rank_two_inpendent-t",2.440E-02*1 FIT);
      }
    }
    else if (module == 1)
    {
      if (!ignore_external_faults)
      {
        addFaultRate("multi_rank-p", 8.538E-01 * 1 FIT);
        addFaultRate("multi_rank-t", 1.057E-01 * 1 FIT);
        addFaultRate("multi_rank_random_bits-p", 0.000E+00 * 1 FIT);
        addFaultRate("multi_rank_random_bits-t", 0.000E+00 * 1 FIT);
        // addFaultRate("multi_rank_two_inpendent-p",1.301E-01*1 FIT);	addFaultRate("multi_rank_two_inpendent-t",3.253E-02*1 FIT);
      }
    }
    else if (module == 2)
    {
      if (!ignore_external_faults)
      {
        addFaultRate("multi_rank-p", 1.147E+00 * 1 FIT);
        addFaultRate("multi_rank-t", 2.521E-01 * 1 FIT);
        addFaultRate("multi_rank_random_bits-p", 0.000E+00 * 1 FIT);
        addFaultRate("multi_rank_random_bits-t", 0.000E+00 * 1 FIT);
        // addFaultRate("multi_rank_two_inpendent-p",9.758E-02*1 FIT);	addFaultRate("multi_rank_two_inpendent-t",8.132E-03*1 FIT);
      }
    }
    else if (module == 3)
    {
      if (!ignore_external_faults)
      {
        addFaultRate("multi_rank-p", 9.866E-01 * 1 FIT);
        addFaultRate("multi_rank-t", 1.843E-01 * 1 FIT);
        addFaultRate("multi_rank_random_bits-p", 2.711E-03 * 1 FIT);
        addFaultRate("multi_rank_random_bits-t", 1.084E-02 * 1 FIT);
        // addFaultRate("multi_rank_two_inpendent-p",1.328E-01*1 FIT);	addFaultRate("multi_rank_two_inpendent-t",2.168E-02*1 FIT);
      }
    }
    else if (module == 4)
    {
      if (!ignore_external_faults)
      {
        addFaultRate("multi_rank-p", 9.866E-01 * 1 FIT);
        addFaultRate("multi_rank-t", 1.843E-01 * 1 FIT);
        addFaultRate("multi_rank_random_bits-p", 2.711E-03 * 1 FIT);
        addFaultRate("multi_rank_random_bits-t", 1.084E-02 * 1 FIT);
        // addFaultRate("multi_rank_two_inpendent-p",1.328E-01*1 FIT);	addFaultRate("multi_rank_two_inpendent-t",2.168E-02*1 FIT);
      }
    }
  }
}

// n choose k for sufficiently small integer
double InherentRate::nchoosek(int n, int k)
{
  if (k == 0)
    return 1;
  return (n * nchoosek(n - 1, k - 1)) / k;
}

void InherentRate::setTester(class TesterSystem *tester, class ECC *ecc)
{
  _tester = tester;
  _ecc = ecc;
}

void InherentRate::setIFRate()
{
  initial = false;
  int bitN = _ecc->getBitN();
  // assuming 16GB DIMM Config.hh
  // 16*8*1024*1024*1024 = 137438953472
  // double p1 = (totalWeakCells / GB_16);
  double p1 = _tester->getRatioWC();
  double p2 = _tester->getActiveProbWC();
  double p3 = _tester->getRatioFWC();
  double p4 = _tester->getActiveProbFWC();

  double p_err[Twelv * 2 + 2] = {
      0,
  }; // error actually activated

  pWordWC_ = bitN * (p1 + p3);
  // rough estimation of prob. that a weak cell exists in a block
  // probability pre-calculations
  for (int k1 = 0; k1 < Twelv * 10; k1++)
  {
    for (int k2 = 0; k2 < Twelv * 10; k2++)
    {
      double comb = nchoosek(bitN, k1 + k2) * nchoosek(k1 + k2, k1);

      pWordWC[k1][k2] =
          comb * pow(p1, k1) * pow(p3, k2) * pow(1 - p1 - p3, bitN - k1 - k2);
    }

    for (int j = 0; j <= Twelv; j++)
    {
      if (j > k1)
      {
        pError1[k1][j] = 0;
        pError2[k1][j] = 0;
      }
      else
      {
        pError1[k1][j] = nchoosek(k1, j) * pow(p2, j) * pow(1 - p2, k1 - j);
        pError2[k1][j] = nchoosek(k1, j) * pow(p4, j) * pow(1 - p4, k1 - j);
      }
    }
  }
  // summing up probabilties
  for (int k1 = 0; k1 < Twelv * 10; k1++)
  {
    for (int k2 = 0; k2 < Twelv * 10; k2++)
    {
      for (int j1 = 0; j1 <= Twelv; j1++)
      {
        for (int j2 = 0; j2 <= Twelv; j2++)
        {
          p_err[j1 + j2] += pWordWC[k1][k2] * pError1[k1][j1] * pError2[k2][j2];
        }
      }
    }
  }
  for (int i = 0; i <= Twelv; i++)
  {
    IFRate[i] = (p_err[i] * GB_1) / (bitN / 1.0625) * 16 *
                3600; // for 1GB device, prob of multi-bit errors per access
                      // (64ms) * 16 * 3600

    printf("IFRate[%i]: %.3e %.3e\n", i, IFRate[i], p_err[i]);
  }
  // Double-device (Double-bit+Single-bit) error among 9 chips
  IFRate[DoubleSingle9] = nchoosek(9, 1) * nchoosek(8, 1) * p_err[1] *
                          p_err[2] * pow(1 - p_err[1] - p_err[2], 7) * GB_1 /
                          (bitN / 1.0625) * 16 * 3600;
  // printf("IFRate[%i]: %.3e\n", DoubleSingle9, IFRate[DoubleSingle9]);
  // Double-device (Double-bit+Double-bit) error among 9 chips
  IFRate[DoubleDouble9] = nchoosek(9, 2) * pow(p_err[2], 2) *
                          pow(1 - p_err[2], 7) * GB_1 / (bitN / 1.0625) * 16 *
                          3600;
  // printf("IFRate[%i]: %.3e\n", DoubleDouble9, IFRate[DoubleDouble9]);
  // Double-device (Single-bit+Single-bit) error among 18 chips
  IFRate[SingleSingle18] = nchoosek(18, 2) * pow(p_err[1], 2) *
                           pow(1 - p_err[1], 16) * GB_1 / (bitN / 1.0625) * 16 *
                           3600;
  IFRate[SingleSingle10] = nchoosek(10, 2) * pow(p_err[1], 2) *
                           pow(1 - p_err[1], 8) * GB_1 / (bitN / 1.0625) * 16 *
                           3600;
  IFRate[SingleSingleOn18Symbol] = nchoosek(18, 2) * pow(p_err[1], 2) *
                                   pow(1 - p_err[1], 16) * GB_1 / (bitN / 1.0625) * 16 *
                                   3600;
  IFRate[SingleSingleSingleOn18Symbol] = nchoosek(18, 3) * pow(p_err[1], 3) *
                                         pow(1 - p_err[1], 16) * GB_1 / (bitN / 1.0625) * 16 *
                                         3600;
  // printf("IFRate[%i]: %.3e\n", SingleSingle18, IFRate[SingleSingle18]);
  // Double-device (Double-bit+Double-bit) error among 18 chips
  IFRate[DoubleDouble18] = nchoosek(18, 2) * pow(p_err[2], 2) *
                           pow(1 - p_err[2], 16) * GB_1 / (bitN / 1.0625) * 16 *
                           3600;
  // printf("IFRate[%i]: %.3e\n", DoubleDouble18, IFRate[DoubleDouble18]);

  IFRate[DoubleDouble10] = nchoosek(10, 2) * pow(p_err[2], 2) *
                           pow(1 - p_err[2], 8) * GB_1 / (bitN / 1.0625) * 16 *
                           3600;
  // printf("IFRate[%i]: %.3e\n", DoubleDouble10, IFRate[DoubleDouble10]);
  // Single-device double-bit & Double-device single-bit error among 18 chips
  IFRate[DoubleSingleSingle18] =
      nchoosek(18, 2) * nchoosek(16, 1) * pow(p_err[2], 1) * pow(p_err[1], 2) *
      pow(1 - p_err[1] - p_err[2], 15) * GB_1 / (bitN / 1.0625) * 16 * 3600;
  // printf("IFRate[%i]: %.3e\n", DoubleSingleSingle18,
  // IFRate[DoubleSingleSingle18]);
}
