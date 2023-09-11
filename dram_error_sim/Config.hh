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
 * @file: Config.hh
 * @author: Jungrae Kim <dale40@gmail.com>
 * @brief miscellaneous definitions related to configuration.
 */
#pragma once

#ifndef __CONFIG_HH__
#define __CONFIG_HH__
#include <math.h>

#define FIT *(0.000000001)
#define AutogenMASK 2

#if AutogenMASK == 0
// Jaguar
// 2GB DDR-2 DIMM -> 128MB (1Gb) x4 DDR-2 chip
// # of bank 8
// row addr A[13:0]
// column address {A11, A[9:0]}
// 2 rank
//#define DEFAULT_MASK     0x0000000000000000ULL
#define DEFAULT_MASK 0xFFFFFFFFE0000000ULL

#define SBIT_MASK (0x0000000000000000ULL | DEFAULT_MASK)
#define SWORD_MASK (0x0000000000000000ULL | DEFAULT_MASK)
#define SCOL_MASK (0x00000000000007FFULL | DEFAULT_MASK)
#define SROW_MASK (0x000000000FFFC000ULL | DEFAULT_MASK)
#define SBANK_MASK (SCOL_MASK | SROW_MASK | DEFAULT_MASK)
#define MBANK_MASK (0x000000000FFFFFFFULL | DEFAULT_MASK)
#define MRANK_MASK (0x000000001FFFFFFFULL | DEFAULT_MASK)
#define CHANNEL_MASK (0xFFFFFFFFFFFFFFFFULL)
#define SAME_BANKGROUP_MASK MBANK_MASK
#define SAME_BANKIDX_MASK SBANK_MASK
#elif AutogenMASK == 1
// R-EGB
// 16GB DDR-4 DIMM -> 1GB (8Gb) x4 DDR-4 chip
// # of bank 16
// row addr A[15:0]
// column address {A11, A[9:0]}
// 2 rank
//#define DEFAULT_MASK     0x0000000000000000ULL
#define DEFAULT_MASK 0xFFFFFFFF00000000ULL

#define SBIT_MASK       (0x0000000000000000ULL | DEFAULT_MASK)
#define SWORD_MASK      (0x0000000000000000ULL | DEFAULT_MASK)
#define SCOL_MASK       (0x00000000000007FFULL | DEFAULT_MASK)
#define SROW_MASK       (0x000000007FFF8000ULL | DEFAULT_MASK)
#define SBANK_MASK      (SCOL_MASK | SROW_MASK | DEFAULT_MASK)
#define MBANK_MASK      (0x000000007FFFFFFFULL | DEFAULT_MASK)
#define MRANK_MASK      (0x00000000FFFFFFFFULL | DEFAULT_MASK)
#define CHANNEL_MASK    (0xFFFFFFFFFFFFFFFFULL)
#define SAME_BANKGROUP_MASK MBANK_MASK
#define SAME_BANKIDX_MASK SBANK_MASK
#elif AutogenMASK == 2

//[Rank][Bank][Bankgroup][Row][Column]
void setup_configs(char* type_char);
extern unsigned long long DEFAULT_MASK;
extern  unsigned long long SBIT_MASK ;
extern  unsigned long long SWORD_MASK;
extern  unsigned long long SCOL_MASK;
extern  unsigned long long SROW_MASK ;
extern  unsigned long long SBANK_MASK;
extern unsigned long long SBANK_MASK_DEGRADE;
extern  unsigned long long MBANK_MASK ;
extern  unsigned long long SAME_BANKGROUP_MASK;
extern  unsigned long long SAME_BANKIDX_MASK ;
extern  unsigned long long MRANK_MASK ;
extern  unsigned long long CHANNEL_MASK;
extern unsigned long long BLSA_MASK ;
extern unsigned long long BANK_PATTERN_MASK;
extern unsigned long long CDEC_MASK;
extern unsigned long long CSL_MASK;
extern unsigned long long RDEC_MASK;
extern unsigned long long SWD_MASK;
extern unsigned long long LWL_MASK;
extern unsigned long long combo_mask;

extern int subarray_address_bits;
extern int row_address_bits;
extern int column_address_bits;

extern int mat_row;
extern int mat_col;
extern int numofBanks;

extern int subarray_col_size;
extern int subbank_size;
extern int subarrays_per_subbank;
extern int total_row;

// basic parameter calculation part
// Bitline sense amplifier
extern int BLSA_per_subarray;
extern int BLSA_per_bank;
extern int BLSA_per_chip;

// Wordline Driver
extern int WLD_per_subarray;
extern int WLD_per_bank;
extern int WLD_per_chip;

// Column select line
extern int CSL_per_subbank;
extern int CSL_per_bank;
extern int CSL_per_chip;

// Row decoder
extern int RDEC_per_bank;
extern int RDEC_per_chip;
extern int RDEC_SUBBANK_per_chip;

// Column decoder
extern int CDEC_per_bank;
extern int CDEC_per_chip;

// Bank selector
extern int BSEL_per_chip;

// Memory controller <-> Memory
extern int MC2MEM;

extern int BITS_per_chip;

#endif

/**
        * @mainpage Fault-Error Simulator
        *
        * \author Jungrae Kim, Seong-Lyong Gong, Mattan Erez
        * \date September 2017
        *
        * \note Print version available by request.
  * \note This research was, in part, funded by Huawei Technologies and the U.S.
Government with partial support from the Department of Energy under Awards
DE-SC0008671 and DE-SC0008111.
        * The views and conclusions contained in this document are those of the
authors and should not be interpreted as representing the official policies,
either expressed or implied, of the U.S. Government.
        *
        *
        * \brief Fault-Error Simulator is a DRAM reliability evaluation
framework
        * that enables users to test the expected reliability of various error
correcting codes (ECC) and supporting resiliency schemes.
        * The purpose of this document is to describe the simulator and
hierarchical inheritance between its classes.
        * The simulator currently contains conventional ECC schemes and various
new ECC schemes proposed by the LPH group at UT Austin and their related work.
        * \section overview Overview
Fault-Error Simulator (FES) is a DRAM reliability evaluation framework to
estimate the reliability of various error correcting codes (ECC) schemes.
Due to the randomness of DRAM faults and errors, FES runs two-stage Monte Carlo
simulations; briefly, DRAM operational faults are randomly injected at the first
stage and random errors (bit-flip) are generated according to the injected
faults.

FES has two type of simulations, system evaluation and scenario evaluation. For
system evaluation, FES randomly injects DRAM faults based on fault rates
published in a well-known DRAM fault analysis <a
href="https://dl.acm.org/citation.cfm?id=2389100">paper</a>.  Scenario
evaluation is for specific fault patterns; fault types defined in Fault.hh and
Fault.cc can be used as command line arguments (@ref howto)  and these input
faults are constantly injected over iterations.

Target schemes are defined by ECC class, and memory specification such as system
size, rank organization and chip-width are flexibly defined by DomainGroup
class. For detailed review, please see \ref CC.

For DRAM scaling errors, FES utilize the error evaluation model (see
FaultRateInfo.hh) introduced in a recent workshop <a
href="http://ieeexplore.ieee.org/abstract/document/8023728">paper</a>. Briefly,
FES assumes two type of weak DRAM cells exist; **permanent** error cells
generate VRT errors frequently while **intermittent** error cells generate
errors intermittently and infrequently. So, each error cell type needs two
parameters, ratio and activation probability. Ratio represents the probability
that a DRAM cell has permanent (or intermittent) single bit fault, and
activation probability represents the probability that a single bit fault
generates an error during a given time period (e.g. refresh interval). Ratio
1e-6 and act. prob. 1e-7 for 2GB DRAM, for example, implies that about
1e-6*2GB=17k cells have faults and these faulty cells generate errors every 10
million refresh periods on average. How to set up these parameter in command
lines is introduced in \ref howto).


        * \section howto How to build and run
1. Makefile contains single line command for build and will output an executable
"faulterrorsim"

                $make

2. This executable requires multiple parameters as below:

                $./faulterrorsim
                Usage for system evaluation  : ./faulterrorsim TargetSystemID
numTrials RandomSeed S PermanentRate PermanentAct IntermittentRate
IntermittentAct
                Usage for scenario evaluation: ./faulterrorsim TargetSystemID
numTrials RandomSeed FaultType1 FaultType2 ...

        - TargetSystemID: Index of target system; the combination of DomainGroup
and a specific ECC scheme (these indices are declared in main.cc)
        - numTrials: Number of random trials for MonteCarlo simulations.
        - Randomseed: Any string input as the random seed for simulations (the
same seeds will generate the same results - beneficial for debugging)
        - FaultType: FaultType indicates which fault type will be added. For
example, FaultType 'b' implies bit error and FaultType 'p' implies pin error in
a memory transfer block. If FaultType 'S' is inputted, this implies system
evaluation (DUE/SDC probability for a specific system over time).
        - System evaluation with scaling errors requires four additional
parameters (described in \ref overview):
                - PermanentRatio is the ratio of permanent single bit fault
                - PermanentAct is the activation probability of permanent single
bit fault
                - IntermittentRatio is the ratio of intermittent single bit
fault
                - IntermittentAct is the activation probability of intermittent
single bit fault

3. For system evaluation, for example, run as below

                $./faulterrorsim 1230 1000000 example S i6 1e-1 1e-5 1e-7

        - ECCID 1230 denotes DUO (BCH TEC)
        - A million trials
        - System evaluation 'S' requires four additional parameters
        - i6 for PermanentRate denotes 1e-6 (defined in Fault.cc)

4. For scenario evaluation, for example, run as below

                $./faulterrorsim 1230 1000000 example b b

        - ECCID 1230 denotes DUO (BCH TEC)
        - A million trials
        - Multiple fault types can be injected together
        - 'b b' denotes double bit fault pattern
        * \section result Interpretation of results
Result output file has file prefix and it can be easily modified by users
(main.cc). The above scenario evaluation example will outputs
*1230.huawei.4x8.OnchipBCH.Triple.example.b.b* as below. The result breakdown is
printed out at the beginning (after 100 trials) and at the end (after 1 millon
trials) of running. This result shows that DUO BCH TEC has corrected all errors
from double bit fault scenario over one million trials.
\note *NE* denotes Non-Error. **CE** denotes Corrected Error. **DUE** denotes
Detected Uncorrectable Error. **SDC** denotes Silent Data Corruption, which
include miscorrection and undetected errors.

- 1230.huawei.4x8.OnchipBCH.Triple.example.b.b

                After 100 runs
                NE 	0.0000000000
                CE 	1.0000000000
                DUE 0.0000000000
                SDC	0.0000000000
                After 1000000 runs
                NE 	0.0000000000
                CE 	1.0000000000
                DUE 0.0000000000
                SDC	0.0000000000

The following is the result when running a longer simulation for quad bit fault
scenario. About 99% of trials are DUE and there are some SDCs since TEC cannot
correct quad-bit errors. Note that a better precision can be achieved by a
longer simulation and results are printed out every 10 million trials (can be
modified at Tester.cc).
- 1230.huawei.4x8.OnchipBCH.Triple.example.b.b..

                After 100 runs
                NE 	0.0000000000
                CE 	0.0100000000
                DUE 0.9900000000
                SDC	0.0000000000
                After 10000000 runs
                NE 	0.0000000000
                CE 	0.0109416000
                DUE 0.9889894000
                SDC	0.0000690000
                After 20000000 runs
                NE 	0.0000000000
                CE 	0.0109768500
                DUE 0.9889538500
                SDC	0.0000693000
                ...

\note System evaluation outputs the similar results in a vector format of yearly
interval; first element denotes the probability that the system meet the event
(DUE or SDC) in one year and second element denotes the same probability in two
years.

        * \section example Use examples
\subsection add_ECC Adding a new ECC
Estimating the reliability of proposed ECC schemes and comparing with those of
existing ECC schemes are the main use of FES.
The simplest way to add a new ECC scheme is to create new child class of ECC
class. The below snippet is the class definition for DUO BCH-TEC (Huawei.hh).
The details of scheme can be described in function decodeInternal using codec.

        class OnChipBCHTriple : public ECC {
                public:
                    OnChipBCHTriple(bool _parityOn);
                    ErrorType decodeInternal(FaultDomain *fd, CacheLine
&errorBlk);
                protected:
                    Codec *codec;
                        bool parityOn;
        };

Within constructor, for example, a specific codec can be instantiated and
included in a specific ECC class. The below snippet shows that a TEC BCH code
for 512-bit data is added into OnChipBCHTriple class (Huawei.cc).

        OnChipBCHTriple::OnChipBCHTriple(bool _parityOn) : ECC(LINEAR) {
                codec = new BCH<10>("TEC BCH", 544, 30, 3);
                ...
        }

Combined with DomainGroup, ECC scheme can be evaluated as a target system. The
below snippet shows how to register a target system in main.cc. The domain group
consists of 4 memory domain (FaultDomain) and each fault domain consists of 4
DRAM ranks. Assuming DDR5, each rank consists of 4 x8 chips and extra memory
transfer beat is considered for DUO (burst length 17). For better result
classification, a file prefix can be used.

        case 1230:
                dg = new DomainGroupDDR(4, 4, 4, 8, 17);
                ecc = new OnChipBCHTriple(true);
                sprintf(filePrefix, "1230.huawei.4x8.OnChipBCH.Triple.%s",
argv[3]);
                break;

\subsection add_codec Using a new codec
Popular error correcting codes (such as Reed-Solomon codes and
Bose-Chaudhuri-Hocquenghem codes) are already included in the source codes. A
specific codes can be chosen by setting parameters. For example, BCH have a
template parameter *m*, which denotes the order of Galois Field in use.

        template <int m>
        class BCH : public Codec {
                public:
                        BCH(const char *name, int _bitN, int _bitR, int _bitT)
                        : Codec(name, bitN, bitR) {
                                ...
                        }
                ...
        }

OnChipBCHTriple is DUO concept BCH codes at rank-level, and hence a cacheline
includes 32-bit redundancy. For codeword length \f$544 < 2^{10}-1\f$, m=10 is
large enough to use (see <a
href="https://en.wikipedia.org/wiki/BCH_code">Wiki</a> and the below snippet).
The number of redundant bits needed to correct \f$t=3\f$ is \f$m\cdot t = 30 \f$
and the remaining two bits can be used as parity for detecting miscorrection.

        OnChipBCHTriple::OnChipBCHTriple(bool _parityOn) : ECC(LINEAR) {
                codec = new BCH<10>("HEC BCH", 544, 30, 3);
                ...
        }

\subsection add_layout Changing data layout
When adding a new ECC scheme or trying further modification, users MUST be very
careful about data layout, how to rearrange CacheLine data at ECC class into
ECCWord at Codec class. In FES, layout is an attribute of ECC class and uses a
C-style enumerated type, ECCLayout (block.hh). OnChipBCHTriple uses **LINEAR**
layout that is a straightforward data rearrangement. For example, Bamboo72b uses
**PIN** layout, where 8-bit data from the same pin fetched together for symbol
organization of longer RS codes (see <a
href="http://ieeexplore.ieee.org/document/7056025/">BambooECC</a>). All needed
layouts are defined by ECCWord::extract (in message.cc), but users may have to
add a new layout for correct data rearrangement if a new ECC scheme is added.

        void ECCWord::extract(Block* data, ECCLayout layout, int pos, int
channelWidth) {
                if (layout==LINEAR) {
        memcpy(bitArr, &(data->bitArr[channelWidth*pos]),
sizeof(bool)*channelWidth);
        } else if (layout==PIN) {
        for (int i=0; i<channelWidth; i++) {
            bitArr[i*8+0] = data->bitArr[channelWidth*0+i];
            bitArr[i*8+1] = data->bitArr[channelWidth*1+i];
            bitArr[i*8+2] = data->bitArr[channelWidth*2+i];
            bitArr[i*8+3] = data->bitArr[channelWidth*3+i];
            bitArr[i*8+4] = data->bitArr[channelWidth*4+i];
            bitArr[i*8+5] = data->bitArr[channelWidth*5+i];
            bitArr[i*8+6] = data->bitArr[channelWidth*6+i];
            bitArr[i*8+7] = data->bitArr[channelWidth*7+i];
        }
                                ...


* @section CC Core classes overview
        *
        *
        *
         \tableofcontents
For now, the documentation is organized around the following core classes. These
can also be accessed through the "Core classes" tab on the HTML docs.
- \ref Error_Correction
- \ref Data_Block
- \ref Fault_Management
- \ref Tester
//  */

#endif /* __CONFIG_HH__ */
