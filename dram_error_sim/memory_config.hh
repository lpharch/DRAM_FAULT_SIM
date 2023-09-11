/**
 * memory configuration implementation
 * Using mat size, subarray size, number of mats in the subarray, 
 * subbank size, number of rows and columns calculate basic parameters
 */

#ifndef MEMORY_CONFIG_HH
#define MEMORY_CONFIG_HH

// FIT RATE per each component
// BLSA, WLD, CSL, RDEC, CDEC, MC2MEM, BSEL



// configuration part
#define mat_row  1024
#define mat_col  1024
#define num_banks  16

#define subarray_col_size  8192 
// number of columns in subarray per chip
#define subbank_size  (16*1024) 
#define subarrays_per_subbank (subbank_size/mat_row)
// number of rows in subbank per chip
#define total_row  (1 << 17) 
// total number of rows in the memory

// basic parameter calculation part
// Bitline sense amplifier
#define BLSA_per_subarray  subarray_col_size
#define BLSA_per_bank  (BLSA_per_subarray * (total_row/mat_row/4))
// 4 since 2 for combo DRAM, 2 for BLSA cross two mats
#define BLSA_per_chip  (BLSA_per_bank * num_banks)

//Wordline Driver
#define WLD_per_subarray  ((subarray_col_size / mat_col) + 1) 
//For 8 mat subarray, there are 9 WLDs
#define WLD_per_bank  WLD_per_subarray * (total_row/mat_row/2)
// div by 2 for comboDRAM
#define WLD_per_chip  WLD_per_bank * num_banks

// Column select line
#define CSL_per_subbank  1
#define CSL_per_bank  (CSL_per_subbank * (total_row/subbank_size))
#define CSL_per_chip  CSL_per_bank * num_banks

//Row decoder
#define RDEC_per_bank  1
#define RDEC_per_chip  RDEC_per_bank * num_banks

//Column decoder
#define CDEC_per_bank  1
#define CDEC_per_chip  CDEC_per_bank * num_banks

//Bank selector
#define BSEL_per_chip  1

//Memory controller <-> Memory
#define MC2MEM  1



#endif // MEMORY_CONFIG_HH