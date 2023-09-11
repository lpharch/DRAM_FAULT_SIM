

#include "Config.hh"
#include <cstring>
#include <iostream>
#if AutogenMASK == 2

//[Rank][Bank][Bankgroup][Row [Cross_subarray][Within_subarray]][Column]

bool isComboDRAM;
char DRAMTYPE[16];
int size_per_DRAMchip; // Size in Gbit
int Burstlen; // Burstlength
int byN; // x4 chip
int BankperBankGroup;
int Bankgroup;
int numofBanks; // number of banks per chip
int numofRanks;
float row_buffer_size; // in KB per banks

int row_address;
int crosssubarray_address;
int combo_bit;
int subarray_address;

int subarray_per_subbank_address;

int column_address;

unsigned long long combo_mask;

int chips_per_rank;
int total_size_perrank;
int totalbits;

unsigned long long DEFAULT_MASK;
unsigned long long SBIT_MASK;
unsigned long long SWORD_MASK;
unsigned long long MBANK_MASK;
unsigned long long SAME_BANKGROUP_MASK;
unsigned long long SAME_BANKIDX_MASK;
unsigned long long SBANK_MASK;

unsigned long long SCOL_MASK;
unsigned long long SROW_MASK;
unsigned long long SBANK_MASK_DEGRADE;
unsigned long long MRANK_MASK;
unsigned long long CHANNEL_MASK;

unsigned long long BLSA_MASK;

unsigned long long BANK_PATTERN_MASK;

unsigned long long CDEC_MASK;

unsigned long long CSL_MASK;

unsigned long long RDEC_MASK;
unsigned long long LWL_MASK;

unsigned long long SWD_MASK;

int column_address_bits;
int row_address_bits;
int subarray_address_bits;

int mat_row;
int mat_col;
int subarray_col_size;
int subbank_size;
int subarrays_per_subbank;
int total_row;

// basic parameter calculation part
// Bitline sense amplifier
int BLSA_per_subarray;
int BLSA_per_bank;
int BLSA_per_chip;

//Wordline Driver
int WLD_per_subarray;
int WLD_per_bank;
int WLD_per_chip;

// Column select line
int CSL_per_subbank;
int CSL_per_bank;
int CSL_per_chip;

// Row decoder
int RDEC_per_bank;
int RDEC_per_chip;
int RDEC_SUBBANK_per_chip;

int BITS_per_chip;



// Column decoder
int CDEC_per_bank;
int CDEC_per_chip;

// Bank selector
int BSEL_per_chip = 1;

// Memory controller <-> Memory
int MC2MEM = 1;


/*
bool isComboDRAM = true;
char DRAMTYPE[16] = "HBM3";
int size_per_DRAMchip = 16; 
int Burstlen = 8; // Burstlength
int byN = 4; // x4 chip
int BankperBankGroup = 4;
int Bankgroup = 4;
int numofBanks = BankperBankGroup * Bankgroup; // number of banks per chip
int numofRanks = 2;
float row_buffer_size = 1; // in KB per banks

int row_address = log2((long) size_per_DRAMchip * (1024*1024*1024)/numofBanks/(row_buffer_size*1024*8)); 
int crosssubarray_address = isComboDRAM ? row_address - (int)log2(mat_row) -1 : row_address - (int)log2(mat_row);
int combo_bit = isComboDRAM ? 1 : 0;
const int subarray_address = (int)log2(mat_row);

// currently set as 16 ==> 4bits
int subarray_per_subbank_address = (int)log2(subarrays_per_subbank);

// bits to represent row address
int column_address = (int)log2(row_buffer_size*8*1024/Burstlen);

unsigned long long combo_mask = isComboDRAM ? (0x1ULL) << (crosssubarray_address + subarray_address + column_address) : 0;

// bits to represent column address
int chips_per_rank = 64*8/ Burstlen / byN; 
// Based on cacheline size.
int total_size_perrank = chips_per_rank * size_per_DRAMchip; // Size in Gbit
int totalbits = log2(numofRanks) + ceil(log2(numofBanks)) + row_address + column_address;



unsigned long long DEFAULT_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-totalbits)) << (totalbits);
unsigned long long SBIT_MASK = (0x0000000000000000ULL | DEFAULT_MASK);
unsigned long long SWORD_MASK =(0x0000000000000000ULL | DEFAULT_MASK);
unsigned long long MBANK_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(numofBanks)))) | DEFAULT_MASK);
// MBANK_MASK : Any bank failure will be ovelaped with this
unsigned long long SAME_BANKGROUP_MASK = MBANK_MASK ^ ((0xFFFFFFFFFFFFFFFFULL >> (64 - (int)ceil(log2(Bankgroup)))<<(column_address+row_address)));
// SAME_BANKGROUP_MASK: When bank group number is the same, it will be overlapped.
unsigned long long SAME_BANKIDX_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(Bankgroup)))) | DEFAULT_MASK);
// SAME_BANKIDX_MASK: When bank index is the same, it will be overlapped.
unsigned long long SBANK_MASK = SAME_BANKGROUP_MASK & SAME_BANKIDX_MASK;

unsigned long long SCOL_MASK = (~(0xFFFFFFFFFFFFFFFFULL >> (64-column_address)) | DEFAULT_MASK) & SBANK_MASK;
unsigned long long SROW_MASK = (~(0xFFFFFFFFFFFFFFFFULL >> (64-(row_address)) << column_address) | DEFAULT_MASK) & SBANK_MASK;
unsigned long long SBANK_MASK_DEGRADE = 1;
unsigned long long MRANK_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(numofBanks))+log2(numofRanks))) | DEFAULT_MASK);
unsigned long long CHANNEL_MASK = (0xFFFFFFFFFFFFFFFFULL);

// Two subarrays, one column
unsigned long long BLSA_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(1 +subarray_address)) <<column_address | DEFAULT_MASK) & SBANK_MASK;

// Bank_pattern
// Have to attach more bit during execution. It only gives the frame of the bank pattern.
// Select 1-4 bits from bank, select 0-1 bit from column if want to have 2 stride rows.
// Use MBANK_MASK if want global bank errors.
unsigned long long BANK_PATTERN_MASK = SCOL_MASK | 0x7;

// CDEC, column decoder
// Have to attach more bit during execution. It only gives the frame of the column decoder.
// Select 1 bit from column for col_decoder_bank. Don't need to choose for column.
// 0x7 for 8 burst length (128 granularity from 1024 columns)
unsigned long long CDEC_MASK = 0x7 | combo_mask | (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(subarray_per_subbank_address + subarray_address)) <<column_address | DEFAULT_MASK) ;

// CSL, column select line
// Select 1 bit from column for csl_bank. Don't need to choose for column.
// 0x7 for 8 burst length (128 granularity from 1024 columns)
unsigned long long CSL_MASK = 0x7 | (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(subarray_per_subbank_address + subarray_address)) <<column_address | DEFAULT_MASK) ;

// RDEC, row decoder
// Select 1-N bits from external subarray address. 
unsigned long long RDEC_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address + subarray_address)) | DEFAULT_MASK) | combo_mask;
unsigned long long LWL_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address)) | DEFAULT_MASK) | combo_mask;

// SWD 
// select combo_mask bit based on some probability.
unsigned long long SWD_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address + subarray_address)) | DEFAULT_MASK);


const int column_address_bits = column_address;
const int row_address_bits = row_address;
const int subarray_address_bits = subarray_address;
*/

void setup_configs(char* type_char){
    isComboDRAM = true;
    mat_row = 1024;
    mat_col = 1024;
    subbank_size = 16 * 1024;
    subarrays_per_subbank = subbank_size / mat_row;

    strcpy(DRAMTYPE, type_char);
    if (strcmp(DRAMTYPE, "HBM3") == 0){
        isComboDRAM = false;
        size_per_DRAMchip = 32;
        Burstlen = 8;
        byN = 64;
        BankperBankGroup = 4;
        Bankgroup = 16;
        numofBanks = BankperBankGroup * Bankgroup;
        numofRanks = 1;
        row_buffer_size = 1;
        subarray_col_size = row_buffer_size*1024*8;
    } else if(strcmp(DRAMTYPE,"LPDDR5_SPLIT")==0){
        isComboDRAM = false;
        size_per_DRAMchip = 32;
        Burstlen = 16;
        byN = 16;
        BankperBankGroup = 4;
        Bankgroup = 4;
        numofBanks = BankperBankGroup * Bankgroup;
        numofRanks = 1;
        row_buffer_size = 4;
        subarray_col_size = row_buffer_size*1024*8;
    } else if(strcmp(DRAMTYPE,"LPDDR5_MERGE")==0){
        isComboDRAM = false;
        size_per_DRAMchip = 32;
        Burstlen = 16;
        byN = 16;
        BankperBankGroup = 4;
        Bankgroup = 4;
        numofBanks = BankperBankGroup * Bankgroup;
        numofRanks = 1;
        row_buffer_size = 4; //actually 2, but we use 4 to make it easier to calculate
        subarray_col_size = row_buffer_size*1024*8;
    } else if (strcmp(DRAMTYPE,"DDR5")==0){
        isComboDRAM = false;
        size_per_DRAMchip = 32;
        Burstlen = 16;
        byN = 4;
        BankperBankGroup = 4;
        Bankgroup = 8;
        numofBanks = BankperBankGroup * Bankgroup;
        numofRanks = 1;
        row_buffer_size = 1;
        subarray_col_size = row_buffer_size*1024*8; 
    } else{
        isComboDRAM = true;
        size_per_DRAMchip = 16;
        Burstlen = 8;
        byN = 4;
        BankperBankGroup = 4;
        Bankgroup = 4;
        numofBanks = BankperBankGroup * Bankgroup;
        numofRanks = 1;
        row_buffer_size = 0.5;
        subarray_col_size = row_buffer_size*1024*8; 
    }

    row_address = log2((long) size_per_DRAMchip * (1024*1024*1024)/numofBanks/(row_buffer_size*1024*8)); 
    total_row = 1 << row_address;

    BLSA_per_subarray = subarray_col_size;
    // 4 since 2 for combo DRAM, 2 for BLSA cross two mats
    BLSA_per_bank = isComboDRAM ? BLSA_per_subarray * (total_row / mat_row / 4):
                                  BLSA_per_subarray * (total_row / mat_row / 4);
    BLSA_per_chip = BLSA_per_bank * numofBanks;

    // Wordline Driver
    WLD_per_subarray = (subarray_col_size / mat_col) + 1; 
    // For 8 mat subarray, there are 9 WLDs
    WLD_per_bank = isComboDRAM ? WLD_per_subarray * (total_row / mat_row / 2):
                                 WLD_per_subarray * (total_row / mat_row);
    // div by 2 for comboDRAM
    WLD_per_chip = WLD_per_bank * numofBanks;

    CSL_per_subbank = 1;
    CSL_per_bank = CSL_per_subbank * (total_row / subbank_size);
    CSL_per_chip = CSL_per_bank * numofBanks;

    RDEC_per_bank = 1;
    RDEC_per_chip = RDEC_per_bank * numofBanks;
    RDEC_SUBBANK_per_chip = size_per_DRAMchip* RDEC_per_bank; 

    
    CDEC_per_bank = 1;
    CDEC_per_chip = CDEC_per_bank * numofBanks;


    crosssubarray_address = isComboDRAM ? row_address - (int)log2(mat_row) -1 : row_address - (int)log2(mat_row);
    combo_bit = isComboDRAM ? 1 : 0;
    subarray_address = (int)log2(mat_row);

    subarray_per_subbank_address = (int)log2(subarrays_per_subbank);

    column_address = (int)log2(row_buffer_size*8*1024/byN/Burstlen);

    combo_mask = isComboDRAM ? (0x1ULL) << (crosssubarray_address + subarray_address + column_address) : 0;

    //chips_per_rank = 64*8/ Burstlen / byN; 
    //total_size_perrank = chips_per_rank * size_per_DRAMchip;
    totalbits = log2(size_per_DRAMchip) + 30;

    unsigned long long DEFAULT_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-totalbits)) << (totalbits);
    unsigned long long SBIT_MASK = (0x0000000000000000ULL | DEFAULT_MASK);
    unsigned long long SWORD_MASK =(0x0000000000000000ULL | DEFAULT_MASK);
    unsigned long long MBANK_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(numofBanks)))) | DEFAULT_MASK);
    // MBANK_MASK : Any bank failure will be ovelaped with this
    unsigned long long SAME_BANKGROUP_MASK = MBANK_MASK ^ ((0xFFFFFFFFFFFFFFFFULL >> (64 - (int)ceil(log2(Bankgroup)))<<(column_address+row_address)));
    // SAME_BANKGROUP_MASK: When bank group number is the same, it will be overlapped.
    unsigned long long SAME_BANKIDX_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(Bankgroup)))) | DEFAULT_MASK);
    // SAME_BANKIDX_MASK: When bank index is the same, it will be overlapped.
    unsigned long long SBANK_MASK = SAME_BANKGROUP_MASK & SAME_BANKIDX_MASK;

    unsigned long long SCOL_MASK = (~(0xFFFFFFFFFFFFFFFFULL >> (64-column_address)) | DEFAULT_MASK) & SBANK_MASK;
    unsigned long long SROW_MASK = (~(0xFFFFFFFFFFFFFFFFULL >> (64-(row_address)) << column_address) | DEFAULT_MASK) & SBANK_MASK;
    unsigned long long SBANK_MASK_DEGRADE = 1;
    unsigned long long MRANK_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address+row_address+ceil(log2(numofBanks))+log2(numofRanks))) | DEFAULT_MASK);
    unsigned long long CHANNEL_MASK = (0xFFFFFFFFFFFFFFFFULL);

    // Two subarrays, one column
    unsigned long long BLSA_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(1 +subarray_address)) <<column_address | DEFAULT_MASK) & SBANK_MASK;

    // Bank_pattern
    // Have to attach more bit during execution. It only gives the frame of the bank pattern.
    // Select 1-4 bits from bank, select 0-1 bit from column if want to have 2 stride rows.
    // Use MBANK_MASK if want global bank errors.
    unsigned long long BANK_PATTERN_MASK = SCOL_MASK | 0x7;

    // CDEC, column decoder
    // Have to attach more bit during execution. It only gives the frame of the column decoder.
    // Select 1 bit from column for col_decoder_bank. Don't need to choose for column.
    // 0x7 for 8 burst length (128 granularity from 1024 columns)
    unsigned long long CDEC_MASK = 0x7 | combo_mask | (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(subarray_per_subbank_address + subarray_address)) <<column_address | DEFAULT_MASK) ;

    // CSL, column select line
    // Select 1 bit from column for csl_bank. Don't need to choose for column.
    // 0x7 for 8 burst length (128 granularity from 1024 columns)
    unsigned long long CSL_MASK = 0x7 | (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(subarray_per_subbank_address + subarray_address)) <<column_address | DEFAULT_MASK) ;

    // RDEC, row decoder
    // Select 1-N bits from external subarray address. 
    unsigned long long RDEC_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address + subarray_address)) | DEFAULT_MASK) | combo_mask;
    unsigned long long LWL_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address)) | DEFAULT_MASK) | combo_mask;

    // SWD 
    // select combo_mask bit based on some probability.
    unsigned long long SWD_MASK = (0xFFFFFFFFFFFFFFFFULL >> (64-(int)(column_address + subarray_address)) | DEFAULT_MASK);


    column_address_bits = column_address;
    row_address_bits = row_address;
    subarray_address_bits = subarray_address;


    // Use pre-calculated varialbes for FITrates
     if (strcmp(DRAMTYPE, "HBM3") == 0){
        BITS_per_chip = 2; // Based on capacity
        CDEC_per_chip=	64; // Based on bank number
        CSL_per_chip=	1024; // Based on bank number
        RDEC_per_chip=	64; // Based on bank number
        WLD_per_chip=	20480; // Based on capacity
        BLSA_per_chip=	8388608; // Based on capacity
        RDEC_SUBBANK_per_chip = 32; // Based on capacity
    } else if(strcmp(DRAMTYPE,"LPDDR5_SPLIT")==0){
        BITS_per_chip = 2; // Based on capacity
        CDEC_per_chip=	16; // Based on bank number
        CSL_per_chip=	256; // Based on bank number
        RDEC_per_chip=	16; // Based on bank number
        WLD_per_chip=	20480; // Based on capacity
        BLSA_per_chip=	8388608; // Based on capacity
        RDEC_SUBBANK_per_chip = 32; // Based on capacity
    } else if(strcmp(DRAMTYPE,"LPDDR5_MERGE")==0){
        BITS_per_chip = 2; // Based on capacity
        CDEC_per_chip=	16; // Based on bank number
        CSL_per_chip=	256; // Based on bank number
        RDEC_per_chip=	16; // Based on bank number
        WLD_per_chip=	20480; // Based on capacity
        BLSA_per_chip=	8388608; // Based on capacity
        RDEC_SUBBANK_per_chip = 32; // Based on capacity
    } else if (strcmp(DRAMTYPE,"DDR5")==0){
        BITS_per_chip = 2; // Based on capacity
        CDEC_per_chip=	32; // Based on bank number
        CSL_per_chip=	512; // Based on bank number
        RDEC_per_chip=	32; // Based on bank number
        WLD_per_chip=	20480; // Based on capacity
        BLSA_per_chip=	8388608; // Based on capacity
        RDEC_SUBBANK_per_chip = 32; // Based on capacity
    } else{
        BITS_per_chip = 1;
        RDEC_SUBBANK_per_chip = 16;
    }   

    printf("CDEC_per_chip: %d\n", CDEC_per_chip);
    printf("CSL_per_chip: %d\n", CSL_per_chip);
    printf("RDEC_per_chip: %d\n", RDEC_per_chip);
    printf("WLD_per_chip: %d\n",  WLD_per_chip);
    printf("BLSA_per_chip: %d\n", BLSA_per_chip);



}

#endif