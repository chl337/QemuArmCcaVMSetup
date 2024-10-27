#ifndef _DEFINES_H
#define _DEFINES_H
#include <stdint.h>
#include <string.h>

#define RAM_KERNEL_MAPPING_START (0x40000000)

#define RAM_SECURE_SIZE (0x4 * 0x1000 * 0x1000)
#define RAM_SECURE_BASE  (0x40080000)
#define SECURE_VA_START (0x42000000)

#define RAM_SHARED_BASE (0x42000000)
#define RAM_SHARED_SIZE    (0x70000)
#define NS_VA_START	   (0x10000) //avoid zero page
#define RAM_SHARED_END	   (0x80000)
/*configs*/
/*configs*/
#define CONFIG_ENABLE_REAL_MAPPING 0

//implemented PA size is indicated by the value of ID_AA64MMFRO_EL1.PAR field
//all other values are reserved
//for PA size greater than 48 bits, FEAT_LPA is required

#define PA_SIZE_ID_MMFR0_EL1_4GB	(0b0000)	//32 bits
#define PA_SIZE_ID_MMFR0_EL1_64GB	(0b0001)	//36 bits
#define PA_SIZE_ID_MMFR0_EL1_1TB	(0b0010)	//40 bits
#define PA_SIZE_ID_MMFR0_EL1_4TB	(0b0011)	//42 bits
#define PA_SIZE_ID_MMFR0_EL1_16TB	(0b0100)	//44 bits
#define PA_SIZE_ID_MMFR0_EL1_256TB	(0b0101)	//48 bits
#define PA_SIZE_ID_MMFR0_EL1_4PB	(0b0110)	//52 bits

//configuring the maximum OA that can be described by translation table
//entries is one of the following

#define TCR_EL_IPS_SIZE_4GB		(0b000) //4GB 	   32 bits  
#define TCR_EL_IPS_SIZE_64GB	(0b001) //64GB	   36 bits
#define TCR_EL_IPS_SIZE_1TB		(0b010) //1TB 	   40 bits
#define TCR_EL_IPS_SIZE_4TB		(0b011) //4TB 	   42 bits
#define TCR_EL_IPS_SIZE_16TB	(0b100) //16TB       44 bits
#define TCR_EL_IPS_SIZE_256TB	(0b101) //256TB      48 bits
#define TCR_EL_IPS_SIZE_4PB		(0b110) //4PB        52 bits

#define TCR_EL_GRANULE_4KB		(0b00) 
#define TCR_EL_GRANULE_64KB		(0b10) 
#define TCR_EL_GRANULE_16KB		(0b01) 

#define TCR_SHR_NON_SHARE		(0b00) 
#define TCR_SHR_OUT_SHARE		(0b10) 
#define TCR_SHR_IN_SHARE		(0b11) 

#define TCR_RGN_OUTER_NON       (0b00)
#define TCR_RGN_OUTER_WBRAWAC   (0b01)
#define TCR_RGN_OUTER_WTRANWAC  (0b10)
#define TCR_RGN_OUTER_WBRANWAC  (0b11)

#define PERM_R (0b001)
#define PERM_W (0b010)
#define PERM_X (0b100)

#define SHIFT_VA_48_L0 (39)
#define SHIFT_VA_48_L1 (30)
#define SHIFT_VA_48_L2 (21)
#define SHIFT_VA_48_L3 (12)

#define MASK_9BIT (0x01FF)
#define MASK_18BIT (0x03FFFF)
#define MASK_27BIT (0x07FFFFFF)
#define MASK_36BIT (0x0FFFFFFFFF)

#define MASK_VA(address, shift, mask)  ( ((((uint64_t)address)) >> (shift) ) & (mask) )

#define VA_OFFSET_48_L0(address) MASK_VA(address, SHIFT_VA_48_L0 , MASK_9BIT)
#define VA_OFFSET_48_L1(address) MASK_VA(address, SHIFT_VA_48_L1 , MASK_9BIT)
#define VA_OFFSET_48_L2(address) MASK_VA(address, SHIFT_VA_48_L2 , MASK_9BIT)
#define VA_OFFSET_48_L3(address) MASK_VA(address, SHIFT_VA_48_L3 , MASK_9BIT)

#define OA_48_L0(address) MASK_VA(address, SHIFT_VA_48_L0 , MASK_9BIT)
#define OA_48_L1(address) MASK_VA(address, SHIFT_VA_48_L1 , MASK_9BIT)
#define OA_48_L2(address) MASK_VA(address, SHIFT_VA_48_L2 , MASK_9BIT)
#define OA_48_L3(address) MASK_VA(address, SHIFT_VA_48_L3 , MASK_9BIT)


#define MASK_VA_36_TABLE_ADDRESS(address) ((((uint64_t)address) & 0x0000FFFFFFFFF000) >> 12)

#define PAGE_SIZE_4KB	(0x1000)
#define PAGE_SIZE_2MB	(0x200000)
#define NS_PT3_PAGE_COUNT (16) //16*4 pages ==> 64KB

#define SHARE_RAM_PA(address)   ((address) | 0x0001000000000000)
#define UNSHARE_RAM_PA(address) ((address) & 0xFFFEFFFFFFFFFFFF)


/* block descriptors bit masking */
#define BD_OA_l0_get(entry) (((entry) >> 39) & (0x1FF))
#define BD_OA_l0_set(entry,value) ((entry) | (((value) & 0x1FF) << 39))

#define BD_OA_l1_get(entry) (((entry) >> 30) & (0x3FFFF))
#define BD_OA_l1_set(entry,value) ((entry) | (((value) & 0x3FFFF) << 30))

#define BD_OA_l2_get(entry) (((entry) >> 21) & (0x7FFFFFF))
#define BD_OA_l2_set(entry,value) ((entry) | (((value) & 0x7FFFFFF) << 21))

#define BD_OA_l3_get(entry) (((entry) >> 12) & (0x0FFFFFFFFF))
#define BD_OA_l3_set(entry,value) ((entry) | (((value) & 0x0FFFFFFFFF) << 12))

#define BD_UA_get(entry) ((entry) >> 49)
#define BD_UA_set(entry,value) ( (((entry) >> 49) | (value & 0x3FFF)) << 49)

#define BD_LA_get(entry) (((entry) >> 2) & 0x3FF)
#define BD_LA_set(entry,value) ( ((value & 0x3FF) << 2) | entry)


/* end block descriptors bit masking */
#define DESCR_VALID 0x1
#define DESCR_IS_LINK  0x1
#define DESCR_IS_BLOCK 0x0

#define V48_GET_L3_OFFSET(address) (((address) >> 12 ) & 0x1FF)
#define V48_GET_SECURE_BIT(address) (((address) >> 47) && 0x1)

#define TABLE_FORMAT_4KB_52_OA(next_table_pa ,attributes) \
		( (((next_table_pa & 0x0003_FFFF_FFFF_F000) | \
			((next_table_pa & 0x000C_0000_0000_0000) >> 43))) | \
			((attributes & 0xFF) << 56 ) ) 

#define TABLE_FORMAT_4KB_48_OA(next_table_pa ,attributes) \
		( (next_table_pa & 0x0000_FFFF_FFFF_F000) | \
		  ((attributes & 0xFF) << 56) ) 


#define PT_ATTR_AP_KRW_UNN  (0b00)
#define PT_ATTR_AP_KRW_URW  (0b01)
#define PT_ATTR_AP_KRO_UNN  (0b10)
#define PT_ATTR_AP_KRO_URO  (0b11)


/*tables layout */
#define PGD_QUAD_SIZE (512 ) //1 4kB page address 2 TB
#define PUD_QUAD_SIZE (512 ) //1 4kB page address 512 GB
#define PMD_QUAD_SIZE (512 * 32) //1 4kB page address 1GB range  <32GB>
#define PT3_QUAD_SIZE (512 * 4 * 512) //size in 8 bytes chuncks <512MB>


union TCR_EL1_table {
	struct {
		uint64_t T0SZ:6;//address region for TTBR0_EL1 (2**(64-T0SZ))
		uint64_t RES0:1;
		uint64_t EPD0:1;//enable TTBR0_EL1 table walk
		uint64_t IRGN0:2; //inner cacheability
		uint64_t ORGN0:2; //outer cacheability
		uint64_t SH0:2;//see share above
		uint64_t TG0:2; //see granule size above
		uint64_t T1SZ:6;
		uint64_t A1:1;
		uint64_t EPD1:1;//restrict EL0 to lower half of the addresses in memory map
		uint64_t IRGN1:2; //inner cacheability
		uint64_t ORGN1:2;//outer cacheability
		uint64_t SH1:2;
		uint64_t TG1:2; //see granule size above
		uint64_t IPS:3; // IPA size or see defines above
		uint64_t RES1:1;//
		uint64_t AS:1;  //us upper 16 bits of TTBR*_EL1 as address space identifiers
		uint64_t TBI0:1;// top byte ignore for tagged addresses, bit[55] passed to PC 
		uint64_t TBI1:1;// top byte ignore for tagged addresses, bit[55] passed to PC
		uint64_t HA:1; //hw access flag in stage2 
		uint64_t HD:1; // hw dirty state in stage 1
		uint64_t HPD0:1;// block hierarchical permission disables FEAT_HPDS  
		uint64_t HPD1:1;// block hierarchical permission disables FEAT_HPDS
		uint64_t HWU059:1; //block hardware use of corresponding bit in the name 
		uint64_t HWU060:1; //block hardware use of corresponding bit in the name
		uint64_t HWU061:1; //block hardware use of corresponding bit in the name
		uint64_t HWU062:1; //block hardware use of corresponding bit in the name
		uint64_t HWO159:1; //block hardware use of corresponding bit in the name
		uint64_t HWO160:1; //block hardware use of corresponding bit in the name
		uint64_t HWO161:1; //block hardware use of corresponding bit in the name
		uint64_t HWO162:1; //block hardware use of corresponding bit in the name
		uint64_t TB1D0:1;//PA_auth, cache maintenance   
		uint64_t TB1D1:1;//PA_auth, cache maintenance  
		uint64_t NFD0:1; //disable stage 1 non-fault translations for stage 1 translations
		uint64_t NFD1:1; //disable stage 1 non-fault translations for stage 1 translations
		uint64_t E0PD0:1;//us access to addr translation generates faults
		uint64_t E0PD1:1;//us access to addr translation generates faults
		uint64_t TCMA0:1;//mem tag checks, EL2  [59:55] == 0b00000
		uint64_t TCMA1:1;//mem tag checks, EL2  [59:55] == 0b11111
		uint64_t DS:1;   //affects 52 bit addressing, turns bits [9:8] for addr. sharability in TCL_ELx.SH
		uint64_t RES3:4;
	};
	uint64_t reg;
};


//48 bit next leve table address
//TCR_ELx.DS == 1 , 53 bit next level table address
struct TABLE_FORMAT_4KB_52_OA 
{
	uint64_t is_valid:1;
	uint64_t is_link:1;
	uint64_t ignore_0:6;
	uint64_t next_level_table_address_msb:2;
	uint64_t ignore_1:2;
	uint64_t next_level_table_address:38;
	uint64_t res0:1;
	uint64_t ignore_2:8;
	uint64_t attributes:4;
};

/*-------------------------------------------------------------------------------------------------------*/

union TABLE_FORMAT_4KB_48_OA
{
	struct {
		uint64_t is_valid:1;
		uint64_t is_link:1;
		uint64_t ignore_0:10;
		uint64_t next_level_table_address:36;
		uint64_t res0:3;
		uint64_t ignore_1:8;
		uint64_t attributes:5;
	};
	uint64_t reg;
};
/*-------------------------------------------------------------------------------------------------------*/

//for stage 1 translation
//the following show next level attributes
struct table_attributes {
	uint8_t reserved:3;
	uint8_t pxn_table:1;//privileged execute never 
	uint8_t uxn_table:1;//unprivileged execute never
	uint8_t ap_table:2;//limit permissions for subsequent levels
	uint8_t ns_table:1; /*ignored for realms*/
};

union block_page_descriptor_4KB_48_OA {

	struct {
		uint64_t is_valid:1;
		uint64_t is_link:1;
		uint64_t lower_attributes:10;
		uint64_t res0:4;
		uint64_t nT:1;
		uint64_t reserved:22;
		uint64_t oa:9;
		uint64_t res1:2;
		uint64_t upper_attributes:14;
	
	} l0;
	
	struct {
		uint64_t is_valid:1;
		uint64_t is_link:1;
		uint64_t lower_attributes:10;
		uint64_t res0:4;
		uint64_t nT:1;
		uint64_t reserved:13;
		uint64_t oa:18;
		uint64_t res1:2;
		uint64_t upper_attributes:14;
	
	} l1;
	
	struct {
		uint64_t is_valid:1;
		uint64_t is_link:1;
		uint64_t lower_attributes:10;
		uint64_t res0:4;
		uint64_t nT:1;
		uint64_t reserved:4;
		uint64_t oa:27;
		uint64_t res1:2;
		uint64_t upper_attributes:14;
	
	} l2;
	
	struct 
	{
		uint64_t is_valid:1;
		uint64_t is_link:1;
		uint64_t lower_attributes:10;
		uint64_t oa:36;
		uint64_t res0:2;
		uint64_t upper_attributes:14;
	} l3;
	
	uint64_t reg;
};
//attributes for block and page descriptors

union block_page_upper_attr
{
	struct {
		uint16_t res0:2;	
		uint16_t gp:1;//guarded page field requires FEAT_BTI allows pages to be guarded against the execution of
					 // instructions not intended target of a branch 	
		uint16_t dbm:1;	//dirty bit modifier 
		uint16_t contiguous:1;	
		uint16_t pxn:1;	
		uint16_t uxn:1;	// always xn
		uint16_t ignore0:4;//reserved for software
		uint16_t pbha:4; //ignore  system mmu
		uint16_t ignore1:1;
	};
	struct {
		uint16_t reserved0:2;
		uint16_t attributes:14;
	};

};

union block_page_lower_attr
{
	struct {
		uint16_t is_valid:1;
		uint16_t is_link:1;
		uint16_t attr:2;//indexes MAIR_ELx for memory type and cacheablity fields
		uint16_t ns:1;//ignore in realms (secure /non-secure)
		uint16_t ap:2;//access perms if dirty bit is enabled, bits can be cleared by hw
			/*----------------------------------*/
			/*             access               */
			/* ap[1:0]   EL1        EL0         */
			/*  00 	    read/write	none        */
			/*  01 	    read/write	read/write  */
			/*  10 	    read-only	none        */
			/*  11 	    read-only	read-only   */
			/*----------------------------------*/
		uint16_t sh:2;//sharability field
			/*00 non-sharable*/
			/*01 reserved */
			/*10 outer sharable*/
			/*11 innter sharable*/
			/*dev mem region or normal non-cacheable mem => has effective outer sharable*/
		uint16_t af:1;//access flag
		uint16_t ng:1;//not global 
		uint16_t oa_upper:2; //upper msb 2 for 52 bit addressing or sh if not
		uint16_t padding0:2;
	};
	struct {
		uint16_t ignore:2;
		uint16_t attributes:10;
		uint16_t padding1:4;
		
	};
};

typedef union {
	struct {
        uint64_t M:1;
        uint64_t A:1; 
        uint64_t C:1; 
        uint64_t SA:1; 
        uint64_t SA0:1;    
        uint64_t CP15BEN:1;
        uint64_t nAA:1;
        uint64_t ITD:1;
        uint64_t SED:1;
        uint64_t UMA:1;   
        uint64_t EnRCTX:1;
        uint64_t EOS:1;
        uint64_t I:1;   
        uint64_t EnDB:1;
        uint64_t DZE:1;
        uint64_t UCT:1; 
        uint64_t nTWI:1;
        uint64_t RES0:1;
        uint64_t nTWE:1;
        uint64_t WXN:1;  
        uint64_t TSCXT:1;
        uint64_t IESB:1;
        uint64_t EIS:1; 
        uint64_t SPAN:1;
        uint64_t E0E:1;
        uint64_t EE:1; 
        uint64_t UCI:1; 
        uint64_t EnDA:1;  
        uint64_t nTLSMD:1;
        uint64_t LSMAOE:1;
        uint64_t EnIB:1;
        uint64_t EnIA:1;
        uint64_t CMOW:1; 
        uint64_t MSCEn:1;
        uint64_t RES1:1;
        uint64_t BT0:1;
        uint64_t BT1:1;  
        uint64_t ITFSB:1;
        uint64_t TCF0:2;
        uint64_t TCF:2; 
        uint64_t ATA0:1;
        uint64_t ATA:1;  
        uint64_t DSSBS:1; 
        uint64_t TWEDEn:1;
        uint64_t TWEDEL:4;
        uint64_t TMT0:1;
        uint64_t TMT:1; 
        uint64_t TME0:1;
        uint64_t TME:1;  
        uint64_t EnASR:1;
        uint64_t EnAS0:1;
        uint64_t EnALS:1;
        uint64_t EPAN:1;
        uint64_t RES2:2;  
        uint64_t EnTP2:1;
        uint64_t NMI:1;
        uint64_t SPINTMASK:1;
        uint64_t TIDCP:1;
	};
	uint64_t reg;
} SCTLR_EL1_t;


typedef struct {
	uint64_t *PGD;	
	uint64_t *PUD;	
	uint64_t *PMD;	
	uint64_t *PT3;	
	uint64_t start_va;
	uint64_t start_pa;
	uint64_t length;//in pages
	uint64_t flags;
	uint64_t *va_PGD;	
	uint64_t *va_PUD;	
	uint64_t *va_PMD;	
	uint64_t *va_PT3;	
} TranslationTables_t;

void normal_map_kernel_4KB(TranslationTables_t *table, uint64_t ram_size, uint64_t ram_shared_size);
void configure_translation();//int upper_on);
void initialize_page_tables(uint64_t ram_size, uint64_t ram_shared_size);

void set_upper_translation_tables(TranslationTables_t *table);
void set_lower_translation_tables(TranslationTables_t *table);

#endif
