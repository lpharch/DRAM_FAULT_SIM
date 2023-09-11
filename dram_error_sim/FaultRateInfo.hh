/*
Copyright 2017, The University of Texas at Austin
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
 * @file: FaultRateInfo.hh
 * @author: Jungrae Kim <dale40@gmail.com> and Seong-Lyong Gong
 * Fault Rate Information
 */

#ifndef __FAULT_RATE_INFO_HH__
#define __FAULT_RATE_INFO_HH__

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include <string>
#include <memory>

#include "Config.hh"
#include "common.hh"
#include "util.hh"

#define FIXED_FITRATE

#define MODULEC
// default DIMM of 16GB
//#define GB_16 137438953472
#define GB_1 8589934592
//#define BLK_SIZE 512
//#define BLK_GB_16 BLK_SIZE / GB_16

typedef enum {
  None = 0,
  Single = 1,
  Double = 2,
  Triple = 3,
  Quad = 4,
  Penta = 5,
  Hexa = 6,
  Septa = 7,
  Octa = 8,
  Nona = 9,
  Deca = 10,
  Elev = 11,
  Twelv = 12,
  DoubleSingle9 = 13,
  DoubleDouble9 = 14,
  SingleSingle18 = 15,
  SingleSingle10 = 16,
  DoubleDouble18 = 17,
  DoubleDouble10 = 18,
  DoubleSingleSingle18 = 19,
  SingleSingleOn18Symbol,
  SingleSingleSingleOn18Symbol,

  InherrentPatterns
} InherentErrorPattern;  //

/**
 * Inherent Rate class
 * @brief Manage inherent fault rates. The behaviors of inherent faults follow a
 * simple model appeared in a workshop paper - Seong-Lyong Gong, Jungrae Kim,
 * Mattan Erez, "DRAM Scaling Error Evaluation Model Using Various Retention
 * Time," DSN 2017
 */
class InherentRate {
 public:
  InherentRate()
      : IFRate{0,     0 FIT, 0 FIT, 0 FIT, 0 FIT,
               0 FIT, 0 FIT, 0 FIT, 0 FIT, 0 FIT} {
    isPFmode = false;
    initial = true;
  };
  double getRate(InherentErrorPattern ePattern) {
    // updateIFRate();
    assert(ePattern <
           InherrentPatterns);  // currently we have four additional patterns
    return IFRate[ePattern];
  }
  double nchoosek(int n, int k);
  void setIFRate();       //!< calculate and set inherent fault rates
  void updateIFRate(){};  //!< update inherent fault rates
  void setTester(class TesterSystem *tester, class ECC *ecc);
  bool IsPFmode() { return isPFmode; };
  void setPFmode() { isPFmode = true; };
  void setEP(InherentErrorPattern errorPattern) {
    _errorPattern = errorPattern;
  };  //!< set error pattern
  InherentErrorPattern getEP() { return _errorPattern; };

  double pWordWC_;  //!< ratio of weak cells;
  double
      pWordWC[Twelv * 10]
             [Twelv * 10];  //!< joint distribution of x FW cells and y RW cells
  double pError1[Twelv * 10]
                [Twelv + 1];  //!< given x weak cells, distribution of k1 errors
  double pError2[Twelv * 10]
                [Twelv + 1];  //!< given y weak cells, distribution of k2 errors
  bool initial;

 protected:
  double IFRate[InherrentPatterns];                   //!< Inherent Fault Rate in terms of FIT
  class TesterSystem *_tester;         //!< tester class
  class ECC *_ecc;                     //!< ECC class
  bool isPFmode;                       //!< old variable used for debugging
  InherentErrorPattern _errorPattern;  //!< error pattern of interest
};

/**@addtogroup Fault_Management
 * @{
 */

/** @class FaultRateInfo
 * \brief A class that manages overall fault rate information
 * @details Default operational fault rates are defined at DefaultFaultRateInfo.
 * Default inherent faults' behaviors are considered in InherentRate.
*/
class FaultRateInfo {
 public:
  FaultRateInfo() { totalRate = .0; }

 protected:
  void setDetailedError(bool detailed) { detailed_error = detailed; }
 private:
  bool detailed_error = true;
  std::string convertToSimpleError(std::string name) {
    if (name == "inherent"){
      return name;
    }
    std::string prefix = name.substr(0, name.length() - 2);
    std::string suffix = name.substr(name.length() - 2);

    if (prefix == "bank_control_bank_8diff"||
        prefix == "bank_control_manybanks" ||
        prefix == "bank_control_two_banks_not8diff"||
        prefix == "bank_control") {
        prefix = "mbank";
    } else if (prefix == "single_sense_amp" ||
        prefix == "potential_sense_amp") {
        prefix = "scol";
    } else if (prefix == "decoder_multi_col") {
        prefix = "sbank";
    } else if (prefix == "decoder_single_col") {
        prefix = "scol";
    } else if (prefix == "single_csl_bank") {
        prefix = "sbank";
    } else if (prefix == "single_csl_column" || prefix == "potential_csl_column") {
        prefix = "scol";
    } else if (prefix == "multi_rank") {
        prefix = "mrank";
    } else if (prefix == "multi_rank_random_bits") {
        prefix = "mrank";
    } else if (prefix == "multi_socket" ||
                prefix == "multi_socket_could_justone" ||
                prefix == "multi_socket_true_socket") {
        prefix = "mrank";    
    } else if (prefix == "not_clustered_multi_bank"){
        prefix = "mbank";  
    } else if ( prefix == "not_clustered_single_bank" ||
                prefix == "mutli_csls_column_and_bank" ||
                prefix == "mutli_csls_random_bank_row" ||
                prefix == "mutli_csls_random_bits" ||
                prefix == "mutli_csls") {
        prefix = "sbank";
    }else if (prefix == "not_clustered_single_column") {
        prefix = "scol";
    } else if (prefix == "lwl_sel" || prefix == "lwl_sel2" ||
                prefix == "mutli_csls_row_and_bits" ||
                prefix == "mutli_csls_row_related" ||
                prefix == "row_decoder" ||
                prefix == "lwl_sel_column_related"||
                prefix == "lwl_sel2_potential_col") {
        prefix = "sbank";
    } else if (prefix.find("multiple_single_bit_failures") != std::string::npos){
      prefix = "sbit";
    } else if (prefix == "local_wordline" || prefix == "consequtive_rows"){
      prefix = "srow";
    } else if (prefix == "local_wordline_two_clusters"||
      prefix == "subarray_row_decoder_two_clusters"||
      prefix == "lwl_sel_random_bit"){
      prefix = "sbank";
    } else if (prefix == "subarray_row_decoder"){
      prefix = "sbank";
    }
    else{
      assert(0);
    }

    return prefix + suffix;
  }

 public:
  //! get total fault rate
  double getTotalRate() { 
    auto last = rateInfo.rbegin();
#ifdef FIXED_FITRATE
    //For fixed fault rate    
    if ((*last).first == "inherent") 
      return 9.38e-8 + (*last).second;
    else
      return 9.38e-8; 
#else
    return totalRate;
#endif
    //return totalRate;
  }

  //! add fault rate to the list
  void addFaultRate(std::string name, double rate) {
    if (!detailed_error){
      name = convertToSimpleError(name);
    }
    rateInfo.push_back(
      std::make_pair(name, rate)
      );

    totalRate += rate;
  }
  //! remove fault rate from the list
  void removeFaultRate(std::string name, double rate) {
    rateInfo.remove(std::make_pair(name, rate));
    totalRate -= rate;
  }

  void removeLastRate() {
    auto last = rateInfo.rbegin();
    

    totalRate -= (*last).second;
    // free the memory last which is create by make_pair
    rateInfo.pop_back();
    if (rateInfo.size() == 0) printf("empty!!\n");
  }
  //! a simplified calculation of overlap probability between faults
  double overlap_prob(std::string name) {
    double overlap_prob_;
    // double BLK_GB_16 = (double) BLK_SIZE / GB_16;
    double pWord = iRate->pWordWC_;
    if (name == "Sbit") {
      // prob that single bit overlapped with a cacheline in 16GB
      overlap_prob_ = 1 - pow(1-pWord, 1);//
      // std::cout<<"mask: SBIT";
    } else if (name == "Sword") {
      overlap_prob_ = 1 - pow(1 - pWord, 1);  //
      // std::cout<<"mask: SWORD";
    } else if (name == "Scol") {
      overlap_prob_ = 1 - pow(1-pWord, 128*1024);// 128k rows
      // std::cout<<"mask: SCOL";
    } else if (name == "Srow" || name == "Lwordline") {
      overlap_prob_ = 1 - pow(1 - pWord, 128);  // from one row, 128 columns
      // std::cout<<"mask: SROW";
    } else if (name == "Sbank") {
      overlap_prob_ =
          1 - pow(1 - pWord, 128 * 128 * 1024);  // 
      // std::cout<<"mask: SBANK";
    } else if (name == "Mbank") {
      overlap_prob_ =
          1 -
          pow(1 - pWord, 16 * 128 *  128 * 1024);  //
      // std::cout<<"mask: MBANK";
    } else if (name == "Mrank") {
      if (pWord < 1e-40)
        overlap_prob_ = 0;
      else
        overlap_prob_ = 1;
      // std::cout<<"mask: MRANK";
    } else if (name == "Channel") {
      if (pWord < 1e-40)
        overlap_prob_ = 0;
      else
        overlap_prob_ = 1;
      // std::cout<<"mask: CHANNEL";
    } else if (name == "BLSA") {
      overlap_prob_ =
          1 - pow(1 - pWord, 2 * 1024);
    } else if (name == "Bank_pattern"){
      overlap_prob_ =
          1 - pow(1 - pWord, 2 * 128 * 1024);
    } else if (name == "CDEC"){
      overlap_prob_ = 1 - pow(1 - pWord, 2 * 16 * 1024);
    } else if (name == "CSL"){
      overlap_prob_ = 1 - pow(1 - pWord, 16 * 1024);
    } else if (name == "Multi_module"){
      if (pWord < 1e-40)
        overlap_prob_ = 0;
      else
        overlap_prob_ = 1;

    } else if (name == "RDEC"){
      overlap_prob_ = 1 - pow(1 - pWord, 128 * 128);
    } else if (name =="SWD"){
      overlap_prob_ = 1 - pow(1 - pWord, 2 * 1024);
    } else if (name == "Dist_bit"){
      overlap_prob_ = 1 - pow(1 - pWord, 10);
    } else if (name == "CMUX"){
      overlap_prob_ = 1 - pow(1 - pWord, 2 * 1024);
    } else if (name == "MWL"){
      overlap_prob_ = 1 - pow(1 - pWord, 128 * 128);
    }
    else {
      assert(0);
      return 1;
    }

    return overlap_prob_;
  }

  void printFaults() {
    for (auto it = rateInfo.cbegin(); it != rateInfo.cend(); it++) {
      std::cout << (*it).first << " @ FIT rate " << (*it).second << std::endl;
    }
  }
  // std::string pickRandomType() {
  //! pick a random fault type from the list, proportionally to its rate
  const std::pair<std::string, double> *pickRandomType() {
    double draw = (double)rand() / RAND_MAX;
    double sum = .0;
    double total_sum = .0;
    for (auto it = rateInfo.cbegin(); it != rateInfo.cend(); it++) {
      total_sum += (*it).second;
    }
    for (auto it = rateInfo.cbegin(); it != rateInfo.cend(); it++) {
      sum += (*it).second;
      if ((sum / totalRate) >= draw * (total_sum / totalRate)) {
        // return (*it).first;
        return &(*it);
      }
    }

    assert(0);
  }
  InherentRate *iRate = new InherentRate();
  void setTester(class TesterSystem *tester, ECC *ecc) {
    iRate->setTester(tester, ecc);
  };  //_tester = tester;};
 protected:
  std::list<std::pair<std::string, double>> rateInfo;
  // make smart pointer version rateInfo
  // std::list<std::shared_ptr<std::pair<std::string, double>>> rateInfo;
  double totalRate;
  class TesterSystem *_tester;
  class ECC *_ecc;
public:
  ~FaultRateInfo() { 
    delete iRate; 
    rateInfo.clear();
  }

};
/* @} */

/**
 * @class DefaultFaultRateInfo
 * @brief Tianchi DRAM fault dataset
 * https://tianchi.aliyun.com/dataset/132973
 * SRDS 2022, An In-Depth Correlative Study Between DRAM Errors and Server Failures in Production Data Centers
 */
class DefaultFaultRateInfo : public FaultRateInfo {
 public:
  ~DefaultFaultRateInfo() { 
    delete iRate; 
    rateInfo.clear();
  }
  DefaultFaultRateInfo(bool HBMSETUP, bool detailed_errormap);
};
#endif /* __FAULT_RATE_INFO_HH__ */
