# DRAM fault-error simulation #
**Modified date: 08.01.2017**

## Copyright ##
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


## Overview ##
Fault-Error Simulator is a DRAM reliability evaluation framework to estimate the reliability of various error correcting codes (ECC).
.

## How to build and run ##
1. Makefile contains single line command for build and will output an executable "faulterrorsim"

		$make 

2. This executable requires multiple parameters as below:

		$./faulterrorsim
		Usage for system evaluation  : ./faulterrorsim ECCID runCnt RandomSeed S PermanentRate PermanentAct IntermittentRate IntermittentAct
		Usage for scenario evaluation: ./faulterrorsim ECCID runCnt RandomSeed FaultType1 FaultType2 ... 

	- ECCID: Index representing the domain group with a specific DRAM configuration and a specific ECC scheme (this indices are declared in main.c)	
	- runCnt: Number of random iterations to be executed for MonteCarlo simulations.
	- Randomseed: Any string input as the random seed for simulations (the same seeds will generate the same results - beneficial for debugging)
	- FaultType: FaultType indicates which fault type will be added. For example, FaultType 'b' implies bit error and FaultType 'p' implies pin error in a memory transfer block. If FaultType 'S' is inputted, this implies system evaluation (DUE/SDC probability for a specific system over time).
	- PermanentRate is the rate (or ratio) that a DRAM cell has permanent single bit fault (frequently weak) 
	- PermanentAct is the probability that permanent single bit faults generating errors
	- IntermittentRate is the rate (or ratio) that a DRAM cell has intermittent single bit fault (rarely weak) 
	- IntermittentAct is the probability that latent single bit faults generating VRT errors

3. For system evaluation, for example, run as below
				
		$./faulterrorsim 1230 1000000 test S i6 1e-1 1e-5 1e-7
	
	- ECCID 1230 denotes DUO (BCH TEC) 
	- A million iterations
	- System evaluation 'S' requires four additional parameters
	- i6 for PermanentRate denotes 1e-6 (defined in Fault.cc)

4. For scenario evaluation, for example, run as below 
				
		$./faulterrorsim 1230 1000000 test b b

	- ECCID 1230 denotes DUO (BCH TEC)
	- A million iterations
	- Multiple fault types can be injected together
	- 'b b' denotes double bit fault pattern



