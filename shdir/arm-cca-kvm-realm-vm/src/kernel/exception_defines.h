#include "process.h"

#define SYSCALL_NUM(ctx) (ctx->x[8])
#define SYSCALL_ARG1(ctx) (ctx->x[0])
#define SYSCALL_ARG2(ctx) (ctx->x[1])
#define SYSCALL_ARG3(ctx) (ctx->x[2])
#define SYSCALL_ARG4(ctx) (ctx->x[3])
#define SYSCALL_ARG5(ctx) (ctx->x[4])
#define SYSCALL_RETURN(ctx) (ctx->x[0])


///*define a syscall table*/
///*only valid if exception happens in el0, if it happens
// *in el1 it is invalid
// */
//struct _esr_el1 {
//	uint64_t ISS:25;
//	uint64_t IL:1;
//	/*
// 	 * Exception class
//	 * Indicates the reason for the exception tatht this registers holds information about.
//	 */
//	uint64_t EC:6;
//	/*for a memory access generated by an ST64BV or STV64BV0 instruction a Data
//	 * abort for Translation fault, access flag fault or permission fault, this
//	 * field holds register specifier, Xs
//	 * for any other data abort, this field is RES0
//	 */
//	uint64_t ISS2 : 5; //32-36
//	uint64_t reserved : 27;//37-63
//};

//enum esr_el1_ec {
//	EL1_EC_UKNOWN			= 0b000000,
//	EL1_EC_Trap_WF			= 0b000001, /*trapped WF instruction execution*/
//	EL1_EC_Trap_MCR_MRC		= 0b000011, /*Trapped MCR or MRC access with coproc=0b1111) applies when aarch32 is supporeted at any exception level*/
//	EL1_EC_Trap_MCRR_MRRC		= 0b000100, /*Trapped MCRR or MRCC access with coproc==0b1111 that is not supported using EC 0b000000*/
//	EL1_EC_Trap_MCR_MR    		= 0b000101, /*trapped mcr or mrc with coproc==0b1110*/
//	EL1_EC_Trap_LDC_STC		= 0b000110, /*trapped ldc or stc access*/
//	EL1_EC_SVE_SIMD			= 0b000111, /*access to SVE or advanced SIMD or floating-point functionality trapped CPACR_EL1.FPEN
//						     CPTR_EL2.FPEN, CPTR_EL2.TFP or CPTR_EL2.TFP control*/
//	EL1_EC_Trap_LDST64B		= 0b001010, /*trapped execution of an ld64b , st64b , ST64BV, OR ST64BV0, needs: FEAT_LS64*/
//	EL1_EC_Trap_MRRC		= 0b001100, /*trapped mrrc access with coproc==0b1110 :when aarch32 is supported*/
//	EL1_EC_BR_Target		= 0b001101, /*branch target exception*/
//	EL1_EC_IllExec			= 0b001110, /*illegal execution state*/
//	EL1_EC_SVC_32			= 0b010001, /*svc instruction execution in aarch32 :if aarch32 is supported at any level*/
//	EL1_EC_SVC_64			= 0b010101, /*svc instruction execution in aarch64 :if aarch64 is supported at any level*/
//	EL1_EC_TrapMSR_MRS_SystemInst64	= 0b011000,/*trapped msr, mrs or sys instruction exe in aarch64 not reported by ec==0,1,7. */
//	EL1_EC_SVE_TRAP			= 0b011001, /*access to sve functionality trapped as a result of CPACR_EL1.ZEN,CPTR_EL1.ZEN, CPTR_EL1.TZ,CPTR_EL3.EZ not in EC==0b0*/
//	EL1_EC_PTR_AUTH			= 0b011100, /*exception from a pinter authetication instruction authentication failure*/
//	EL1_EC_INST_ABORT_0		= 0b100000, /*instruction abort from a lower exception leve. used for mmu faults generated by 
//							instruction acccesses and synchrnous external aborts, including synchronous parity or ecc errors*/
//	EL1_EC_INST_ABORT_1		= 0b100001, /*instruction abort, taken without a change in exception level, for mmu faults generated
//							by instruction accesses and synchronous parity or ECC errors. */
//	EL1_EC_PC_ALIGN_FAULT   	= 0b100100, /*pc alignment fault exception*/
//	EL1_EC_DATA_ABORT_0		= 0b100100, /*data abort from a lower exception level except:stack misalignment, external aborts, sync parity or ecc*/
//	EL1_EC_DATA_ABORT_1		= 0b100101, /*data abort taken without change in exception level. mmu faults caused by data acccesses, alignment, faults
//						     *or other than those caused by pointer misalignment and synchronous external aborts, sync parity or ECC errors */
//	EL1_EC_SP_ALIGN			= 0b100110, /*sp alignment fault exception*/
//	EL1_EC_TRAP_FP_0		= 0b101000, /*trapped floating point exception taken from aarch32: implementation should support fp trapping else 0b00*/
//	EL1_EC_TRAP_FP_1		= 0b101100, /*trapped floating point exception taken from aarch64: implementation should support fp trapping else 0b00*/
//	EL1_EC_SError			= 0b101111, /*SError interrupt*/
//	EL1_EC_BP_Except_0 		= 0b110000, /*breakpoint exception from a lower exception level*/
//	EL1_EC_BP_Except_1		= 0b110001, /*breakpoint exception taken without a change in exception level*/
//	EL1_EC_SW_Step_0		= 0b110010, /*sw step exception from a lower exception level*/
//	EL1_EC_SW_Step_1		= 0b110011, /*sw step exception taken without a change in exception level*/
//	EL1_EC_WP_0			= 0b110100, /*wp exception from lower level*/
//	EL1_EC_WP_1			= 0b110101, /*wp exception without change in level*/
//	EL1_EC_BKPT_32			= 0b111000, /*BKPT instruction execution in aarch32*/
//	EL1_EC_BKPT_64			= 0b111100, /*BKPT instruction execution in aarch64*/
//	/*all other values are reserved
//	EL1_SYNC_RANGE (0b000000 - 0b101100 ) future synchronous
//	EL1_UNUSED (0x2D -0x3F) future used */
//};

/*instruction length IL*/
#define ESR_EL1_IL_16b_Inst (0b0)
#define ESR_EL1_IL_32b_Inst (0b0)

/*ISS:Instruction specific syndrome*/

/*
1. ISS encoding for exceptions with an unknown reason
  --> IL field == 0x1
  --> See manual for all causes not listed above
2. EL1_EC_TrapWF 
*/

struct _ISS_Inst_abort {
	uint32_t ifsc:6;
	uint32_t :1;
	uint32_t siptw:1;
	uint32_t :1;
	uint32_t ea:1; /*implementation specific, external abort*/
	uint32_t fnv:1;
	uint32_t set:2;
	uint32_t :19;
};
//synchronous error type from aarch32 state 
#define ISS_INST_ABORT_32_SET_RECV 0b00 //recoverable
#define ISS_INST_ABORT_32_SET_UC   0b10 //uncontainable
#define ISS_INST_ABORT_32_SET_UEO  0b11 //restatable

//fnv far not valid
#define ISS_IST_ABORT_FnV_32_True	0b0
#define ISS_IST_ABORT_FnV_32_False	0b1

#define ISS_IST_ABORT_S1PTW_F	0b0
#define ISS_IST_ABORT_S1PTW_T	0b1


/*instruction fault status code */

#define ISS_IST_IFSC_00
#define ISS_IST_IFSC_ADRSZ_L0    0b000000 //Address size fault, level 0 of translation or translation table base register. 
#define ISS_IST_IFSC_ADRSZ_L1    0b000001 //Address size fault, level 1.
#define ISS_IST_IFSC_ADRSZ_L2    0b000010 //Address size fault, level 2.
#define ISS_IST_IFSC_ADRSZ_L3    0b000011 //Address size fault, level 3.
#define ISS_IST_IFSC_ADRTF_L0    0b000100 //Translation fault, level 0.
#define ISS_IST_IFSC_ADRTF_L1    0b000101 //Translation fault, level 1.
#define ISS_IST_IFSC_ADRTF_L2    0b000110 //Translation fault, level 2.
#define ISS_IST_IFSC_ADRTF_L3    0b000111 //Translation fault, level 3.
#define ISS_IST_IFSC_AFF_L0   	 0b001000 //Access flag fault, level 0. When FEAT_LPA2 is implemented
#define ISS_IST_IFSC_AFF_L1   	 0b001001 //Access flag fault, level 1.
#define ISS_IST_IFSC_AFF_L2   	 0b001010 //Access flag fault, level 2.
#define ISS_IST_IFSC_AFF_L3   	 0b001011 //Access flag fault, level 3.
#define ISS_IST_IFSC_PERMF_L0    0b001100 //Permission fault, level 0. When FEAT_LPA2 is implemented
#define ISS_IST_IFSC_PERMF_L1    0b001101 //Permission fault, level 1.
#define ISS_IST_IFSC_PERMF_L2    0b001110 //Permission fault, level 2.
#define ISS_IST_IFSC_PERMF_L3    0b001111 //Permission fault, level 3.
#define ISS_IST_IFSC_SEA_ntl     0b010000 //Synchronous External abort, not on translation table walk or hardware update of translation table.
#define ISS_IST_IFSC_SEA_tl      0b010011 //Synchronous External abort on translation table walk or hardware update of translation table, level -1.  When FEAT_LPA2 is implemented
#define ISS_IST_IFSC_SEA_tl0     0b010100 //Synchronous External abort on translation table walk or hardware update of translation table, level 0.
#define ISS_IST_IFSC_SEA_tl1     0b010101 //Synchronous External abort on translation table walk or hardware update of translation table, level 1.
#define ISS_IST_IFSC_SEA_tl2     0b010110 //Synchronous External abort on translation table walk or hardware update of translation table, level 2.
#define ISS_IST_IFSC_SEA_tl3     0b010111 //Synchronous External abort on translation table walk or hardware update of translation table, level 3.
#define ISS_IST_IFSC_SP_ECC_N    0b011000 //Synchronous parity or ECC error on memory access, not on translation table walk.  When FEAT_RAS is not implemented
#define ISS_IST_IFSC_SP_ECC      0b011011 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level -1.  needs FEAT_LPA2 & !FEAT_RAS 
#define ISS_IST_IFSC_SP_ECC_tl0  0b011100 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 0.  When FEAT_RAS is not implemented
#define ISS_IST_IFSC_SP_ECC_tl1  0b011101 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 1.  When FEAT_RAS is not implemented
#define ISS_IST_IFSC_SP_ECC_tl2  0b011110 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 2.  When FEAT_RAS is not implemented
#define ISS_IST_IFSC_SP_ECC_tl3  0b011111 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 3.  When FEAT_RAS is not implemented
#define ISS_IST_IFSC_ADR_SZF     0b101001 //Address size fault, level -1. When FEAT_LPA2 is implemented ESR_EL1, Exception Syndrome Register (EL1) Page 510
#define ISS_IST_IFSC_ADR_SZF_     0b101011 //Translation fault, level -1. When FEAT_LPA2 is implemented
#define ISS_IST_IFSC_TLB_ABRT    0b110000 //TLB conflict abort.
#define ISS_IST_IFSC_ATOMIC      0b110001 //Unsupported atomic hardware update fault.  When FEAT_HAFDBS is
/*end instruction abort*/

struct _ISS_data_abort {
	uint8_t dfsc:6;
	uint8_t WnR:1;
	uint8_t S1PTW:1;
	uint8_t CM:1;
	uint8_t EA:1;
	uint8_t FnV:1;
	uint8_t none:2;
	uint8_t vncr:1;
	uint8_t ar:1;
	uint8_t sf:1;
	uint8_t srt:5;
	uint8_t sse:1;
	uint8_t sas:2;
	uint8_t isv:1;/*0b0 no valid syndrome, 0b1, ISS[23-14] holds valid syndrome*/
	uint8_t ignore:7;
};


#define ISS_DATA_IFSFC_ADR_SZ_L0   0b000000 //Address size fault, level 0 of translation or translation table base register.
#define ISS_DATA_IFSFC_ADR_SZ_L1   0b000001 //Address size fault, level 1.
#define ISS_DATA_IFSFC_ADR_SZ_L2   0b000010 //Address size fault, level 2.
#define ISS_DATA_IFSFC_ADR_SZ_L3   0b000011 //Address size fault, level 3.
#define ISS_DATA_IFSFC_TF_L0 	   0b000100 //Translation fault, level 0.
#define ISS_DATA_IFSFC_TF_L1 	   0b000101 //Translation fault, level 1.
#define ISS_DATA_IFSFC_TF_L2 	   0b000110 //Translation fault, level 2.
#define ISS_DATA_IFSFC_TF_L3 	   0b000111 //Translation fault, level 3.
#define ISS_DATA_IFSFC_ACF_L0 	   0b001000 //Access flag fault, level 0. When FEAT_LPA2 is implemented
#define ISS_DATA_IFSFC_ACF_L1 	   0b001001 //Access flag fault, level 1.
#define ISS_DATA_IFSFC_ACF_L2 	   0b001010 //Access flag fault, level 2.
#define ISS_DATA_IFSFC_ACF_L3 	   0b001011 //Access flag fault, level 3.
#define ISS_DATA_IFSFC_PERM_F0     0b001100 //Permission fault, level 0. When FEAT_LPA2 is implemented
#define ISS_DATA_IFSFC_PERM_F1     0b001101 //Permission fault, level 1.
#define ISS_DATA_IFSFC_PERM_F2     0b001110 //Permission fault, level 2.
#define ISS_DATA_IFSFC_PERM_F3     0b001111 //Permission fault, level 3.
#define ISS_DATA_IFSFC_SEA 	   0b010000 //Synchronous External abort, not on translation table walk or hardware update of translation table.
#define ISS_DATA_IFSFC_TCF 	   0b010001 //Synchronous Tag Check Fault. When FEAT_MTE2 is implemented 0b010011 SEA on translation table walk or hardware update of translation table, level -1. Needs FEAT_LPA2
#define ISS_DATA_IFSFC_SEA_TL0     0b010100 //Synchronous External abort on translation table walk or hardware update of translation table, level 0.
#define ISS_DATA_IFSFC_SEA_TL1     0b010101 //Synchronous External abort on translation table walk or hardware update of translation table, level 1.
#define ISS_DATA_IFSFC_SEA_TL2     0b010110 //Synchronous External abort on translation table walk or hardware update of translation table, level 2.
#define ISS_DATA_IFSFC_SEA_TL3     0b010111 //Synchronous External abort on translation table walk or hardware update of translation table, level 3.
#define ISS_DATA_IFSFC_SEA_ECCNT   0b011000 //Synchronous parity or ECC error on memory access, not on translation table walk.  When FEAT_RAS is not implemented
#define ISS_DATA_IFSFC_SEA_ECC_TL  0b011011 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level -1. Needs FEAT_LPA2 and !FEAT_RAS
#define ISS_DATA_IFSFC_SEA_ECC_TL0 0b011100 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 0.  When FEAT_RAS is not implemented
#define ISS_DATA_IFSFC_SEA_ECC_TL1 0b011101 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 1.  When FEAT_RAS is not implemented
#define ISS_DATA_IFSFC_SEA_ECC_TL2 0b011110 //Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 2.  When FEAT_RAS is not implemented
#define ISS_DATA_IFSFC_SEA_ECC_TL3 0b011111 //Synchronous parity or ECC error on memory access on translation When !FEAT_RAS, Exception Syndrome Register (EL1)table walk or hardware update of translation table, l3.
#define ISS_DATA_IFSFC_ALIGN_FAULT 0b100001 //Alignment fault.
#define ISS_DATA_IFSFC_ADRSZ_FAULT 0b101001 //Address size fault, level -1. When FEAT_LPA2 is implemented
#define ISS_DATA_IFSFC_TRANS_FAULT 0b101011 //Translation fault, level -1. When FEAT_LPA2 is implemented
#define ISS_DATA_IFSFC_TLB_ABORT   0b110000 //TLB conflict abort.
#define ISS_DATA_IFSFC_UNSUPPORTED 0b110001 //Unsupported atomic hardware update fault.  When FEAT_HAFDBS is implemented
#define ISS_DATA_IFSFC_IMPL_DEFINE_LOCKED 0b110100 //IMPLEMENTATION DEFINED fault (Lockdown).
#define ISS_DATA_IFSFC_IMPL_DEFINE_ATOMIC 0b110101 //IMPLEMENTATION DEFINED fault (Unsupported Exclusive or Atomic access


#define ISS_DATA_ISV_INVALID	0b0
#define ISS_DATA_ISV_VALID	0b1
/*syndrome access size*/
/*FEAT_LS64: is implemented ST64BV,ST64BV0,ST64B,LD64B
 * insts. generate a data abort for a translation fault, access flag fault,
 *  or permission fault then field is 0b11
*/
#define ISS_DATA_SAS_U8		0b00
#define ISS_DATA_SAS_U16	0b01
#define ISS_DATA_SAS_U32	0b10
#define ISS_DATA_SAS_U64	0b11

/*sse bit*/
#define ISS_DATA_SSE_NOSIGN 0b0//no sign extension
#define ISS_DATA_SSE_SIGN   0b1//need sign extension

/*sf, instruction stores/loads a 64bit reg bit*/
#define ISS_DATA_SF_W32 0b0 //32 bit width
#define ISS_DATA_SF_W64 0b1 //64 bit width

#define ISS_DATA_ACQUIRE_NONE	0b0 //inst had no acquire/release semantics
#define ISS_DATA_ACQUIRE	0b1 //inst had acquire/release semantics

#define ISS_DATA_VNCR_TRUE	0b0 //fault not generated by use of VNCR_EL2, with mrs/msr
#define ISS_DATA_VNCR_FALSE	0b1 //fault generated by use of VNCR_EL2, with msr/mrs

//DFSC code 0b010000 needs feature FEAT_RAS and !FEAT_LS64
#define ISS_DATA_SET_RAS_UER 0b00 //recoverable state (UER)
#define ISS_DATA_SET_RAS_UC  0b10 //uncontainable UC
#define ISS_DATA_SET_RAS_UEO 0b11 //restartable state (UEO)

//DFSC code 0b110101 needs FEAT_LS64
#define ISS_DATA_SET_LST_STBV  0b01 //an st64bv inst. generated by data abort
#define ISS_DATA_SET_LST_LDBV  0b10 //an LD64B or ST64B inst. generated the data abort
#define ISS_DATA_SET_LST_STBV0 0b11 //an st64BV0 inst. generated the data abort

#define ISS_DATA_CM_F 0b0 //data abort was not generated by execution of Sys Inst
#define ISS_DATA_CM_T 0b1 //cause: CM inst or sync. fault on addr. Tranx inst.

#define ISS_DATA_S1PTW_F 0b0 //fault not on stage 2 tranx for a stage 1 tranx table walk
#define ISS_DATA_S1PTW_NF 0b1 //fault on stage 2 tranx for a stage 1 tranx table walk

#define ISS_DATA_WnR_Read  0b0 //abort caused by inst. read from mem
#define ISS_DATA_WnR_Write 0b1 //abort caused by inst. write from mem

struct _iss_float_point_except {
	uint32_t iof:1;
	uint32_t dzf:1;
	uint32_t off:1;
	uint32_t uff:1;
	uint32_t ixf:1;
	uint32_t :2;
	uint32_t idf:1;
	uint32_t vecitr:3;
	uint32_t :12;
	uint32_t tfv:1;
	uint32_t :8;//reserved
};

#define ISS_FLOAT_TFV_T 0b1
#define ISS_FLOAT_TFV_F 0b0

#define ISS_FLOAT_IDF_F 0b0
#define ISS_FLOAT_IDF_T 0b1 /*input denormal fp exception occured*/

#define ISS_FLOAT_IXF_F 0b0
#define ISS_FLOAT_IXF_T 0b1 /*inexact fp exception occured*/

#define ISS_FLOAT_UFF_F 0b0
#define ISS_FLOAT_UFF_T 0b1 /*underflow fp exception occured*/

#define ISS_FLOAT_DZF_F 0b0
#define ISS_FLOAT_DZF_T 0b1 /*divide by zero fp exception occured*/

#define ISS_FLOAT_IOF_F 0b0
#define ISS_FLOAT_IOF_T 0b1 /*invalid operation fp exception occured*/


struct _iss_serror_interrupt {
	uint16_t dfsc:6;
	uint16_t :3;
	uint16_t ea:1;
	uint16_t aet:3;
	uint16_t iesb:1;
	uint16_t :9;
	uint16_t ids:1;
	uint16_t :7;
};


//implementation defined syndrome in RES0
#define ISS_SERROR_IDS_F 0b0
#define ISS_SERROR_IDS_T 0b1

//implicit error synchronization event
#define ISS_SERRO_IESB_F 0b0
#define ISS_SERRO_IESB_T 0b1

//asynchronous error type
#define ISS_SERROR_AET_UC	0b000 //uncontainable
#define ISS_SERROR_AET_UEU	0b001 //unrecoverable state 
#define ISS_SERROR_AET_UE0	0b010 //retartable state 
#define ISS_SERROR_AET_UER	0b011 //recovrable state 
#define ISS_SERROR_AET_CE	0b110 //corrected 
//external abort type, provides implementation defined classification of external aborts
#define ISS_SERROR_EA  0b1

#define ISS_SERROR_DFSC_NOCAT 	   0b000000
#define ISS_SERROR_DFSC_ASYSE_INT  0b010001


struct _iss_bp_vec_catch_debug_exception {
	uint32_t ifsc:6;
	uint32_t :26;
};

#define ISS_BP_VCB_DEBUG 0b100010 //debug exception

struct _iss_sw_step_exception {
	uint32_t ifsc:6;
	uint32_t ex:1;
	uint32_t res0:18;
	uint32_t isv:1;
	uint32_t :7;
};

#define ISS_SW_STEP_EX_NOLOAD  0b0 //an instruction other than a load exclusive instruction was stepped
#define ISS_SW_STEP_EX_LOAD    0b1 //a load exclusive instruction was stepped

#define ISS_SW_STEP_IFSC_DEBUG 0b100010 //debug exception

struct _iss_watchpoint_exception {
	uint32_t dfsc:6;
	uint32_t wnr:1;
	uint32_t res0:1;
	uint32_t cm:1; //cache maintenance
	uint32_t :4;
	uint32_t vncr:1;
	uint32_t :1;
	uint32_t :8;
	uint32_t :8;
};

#define ISS_WATCHPOINT_EXCEPTION_VNCR_NO  0b0 
#define ISS_WATCHPOINT_EXCEPTION_VNCR 	  0b1 //watchpoint was generated by use of VNCR_EL2 by EL1 code

#define ISS_WATCHPOINT_EXCEPTION_CM_NO	 0b0 
#define ISS_WATCHPOINT_EXCEPTION_CM	 0b1 //wp exception was generated by cache maintenance or translation inst


/*
 * For Watchpoint exceptions on cache maintenance and address translation instructions, this bit always returns
 * a value of 1. For Watchpoint exceptions from an atomic instruction, this field is set to 0 if a read of the 
 * location would have generated the Watchpoint exception, otherwise it is set to 1
**/
#define ISS_WATCHPOINT_EXCEPTION_WnRR   0b0 //caused by read inst at a memory location  
#define ISS_WATCHPOINT_EXCEPTION_WnRW   0b1 //caused by write inst at a memory location 
//data fault status code
#define ISS_WATCHPOINT_EXCEPTION_WnRW   0b100010 //debug exception 

struct _iss_breakpoint {
	uint16_t comment;
	/*TODO: incomplete*/
};

/*encoding for exception from eret, eretaa, or eretab instruction*/
struct _iss_erets {
	uint16_t ereta:1;
	uint16_t eret:1;
	uint16_t :14;
	uint16_t :16;
};

#define ISS_ERETS_ERET_ERET    0b0 //eret instruction trapped to el2
#define ISS_ERETS_ERET_ERETTAX 0b1 //eretaa or eretab instruction trapped to el2

#define ISS_ERETS_ERATAA 0b0 //eretaa instruction trapped to el2
#define ISS_ERETS_ERATAB 0b1 //eretab instruction trapped to el2

struct _iss_branch_target_id_inst {
	uint16_t btype:2; /*set pstate.btype value that generated the branch target exception*/	
	uint16_t :14;	
	uint16_t :16;	
};

//pointer authentication wht hcr_el2.api==0 || scr_el3.api == 0
struct _iss_ptr_auth_inst {
	uint32_t res0:25;
	uint32_t :7;
};
/*TODO: differentiate with above*/
struct __iss_ptr_auth_inst {
	uint32_t a_or_b:1;
	uint32_t inst_or_data:1;
	uint32_t res0:23;
	uint32_t :7;
};

#define ISS_PTR_AUTH_INST_INST_KEY 0b0
#define ISS_PTR_AUTH_INST_DATA_KEY 0b1

#define ISS_PTR_AUTH_INST_A_KEY 0b0
#define ISS_PTR_AUTH_INST_B_KEY 0b1
/*the following instructions generate an exception when the pointer authentication code (PAC) is incorrect*/
/*
 *  AUTIASP, AUTIAZ, AUTIA1716.
 *  AUTIBSP, AUTIBZ, AUTIB1716.
 *  AUTIA, AUTDA, AUTIB, AUTDB.
 *  AUTIZA, AUTIZB, AUTDZA, AUTDZB
*/
/*
 * The generation by following exceptions is implementation defined
 * � RETAA, RETAB.
 * � BRAA, BRAB, BLRAA, BLRAB.
 * � BRAAZ, BRABZ, BLRAAZ, BLRABZ.
 * � ERETAA, ERETAB.
 * � LDRAA, LDRAB whether the authenticated address is written back to base register or not
*/


void serror_el1_handler(context_t *ctx)
{
}

void irq_el1_handler(context_t *ctx)
{
}

void fiq_el1_handler(context_t *ctx)
{
}
