#include "rsi.h"
#include "vma.h"

uint64_t get_rsi_version()
{
	uint64_t register fid asm("x0") = RSI_VERSION;
	asm("smc 0;");
	return fid;
}

uint64_t rsi_realm_config(uint64_t addr_frame)
{
	uint64_t register fid asm("x0") = RSI_REALM_CONFIG;
	uint64_t register _addr asm("x1") __attribute__((unused)) = (uint64_t)addr_frame;

	if (addr_frame == 0 || (addr_frame & 0xFFFF)) {
		return RSI_ERROR_INPUT;
	}
	asm("smc 0;");

	return fid;
}

int rsi_get_ipa_state(uint64_t ipa)
{
	uint64_t fid = RSI_IPA_STATE_GET;
	uint64_t address = ipa;
	asm("mov x0, %0; \
		mov x1, %1; \	
		smc 0;	\
		mov %0,x0; \
		mov %1,x1; ":"=r"(fid), "=r"(address)::"x0","x1");

	if (fid == RSI_SUCCESS) {
		return (address & 0xFF);
	}
	return -1;
}

int rsi_set_ipa_state(uint64_t ipa, uint64_t size, uint64_t ripas)
{
	uint64_t fid ;
	uint64_t base ;
	__asm__ volatile (
		"mov x0, %[fid_in]; 	\
		mov x1, %[ipa]; 	\
		mov x2, %[size]; 	\
		mov x3, %[ripas]; 	\
		smc 0;		\
		mov %[fid_out], x0;	\
		mov %[base], x1;":	
		[fid_out] "=r" (fid), [base]"=r"(base):
		[fid_in] "r" (RSI_IPA_STATE_SET),[size]"r"(size), [ripas]"r"(ripas), [ipa]"r"(ipa):
			 "x0","x1","x2","x3");

	return (fid != RSI_SUCCESS) ? 0 : 1;
}

struct rsi_host_call {
        uint16_t imm;
        uint8_t __pad[6];
        uint64_t gprs[31];
} __attribute__((aligned(0x1000)));

struct rsi_host_call rhc_instance __attribute__((__aligned__(0x1000)));

void rsi_host_call() {
       // struct rsi_host_call rhc = {
       //         .imm = 0,
       //         .__pad = {0,0,0,0,0,0},
       //         .gprs = {0, 0, 0, 0, 0,
       //                 0, 0, 0, 0, 0,
       //                 0, 0, 0, 0, 0,
       //                 0, 0, 0, 0, 0,
       //                 0, 0, 0, 0, 0,
       //                 0, 0, 0, 0, 0, 0}
       // };
        uint64_t ipa_val = (uint64_t)get_physical_address((uint64_t)&rhc_instance);
        memset(&rhc_instance,0,0x1000);
        rhc_instance.gprs[0] = RSI_HOST_CALL;
        rhc_instance.gprs[1] = 0xc0ffe;
        rhc_instance.gprs[3] = 0x000000e0;
        rhc_instance.gprs[4] = 0x000000ef;
        rhc_instance.gprs[5] = 0x00000e00;
        rhc_instance.gprs[6] = 0x00000Eef;
        rhc_instance.gprs[7] = 0x0000B000;
        rhc_instance.gprs[8] = 0x0000B00F;
        rhc_instance.gprs[9] = 0x0000B0E0;
        rhc_instance.gprs[10] = 0x0000BE00;
        rhc_instance.gprs[11] = 0x0000BEe0;
        rhc_instance.gprs[12] = 0x0000Beef;
        rhc_instance.gprs[13] = 0xD0000000;
        rhc_instance.gprs[14] = 0xDE000000;
        rhc_instance.gprs[15] = 0xDeA00000;
        rhc_instance.gprs[16] = 0xDeaD0000;
        rhc_instance.gprs[17] = RSI_HOST_CALL;
        rhc_instance.gprs[18] = ipa_val;
        rhc_instance.gprs[19] = 0x0000000f;
        rhc_instance.gprs[20] = 0x000000e0;
        rhc_instance.gprs[21] = 0x000000ef;
        rhc_instance.gprs[22] = 0x00000e00;
        rhc_instance.gprs[23] = 0x00000Eef;
        rhc_instance.gprs[24] = 0x0000B000;
        rhc_instance.gprs[25] = 0x0000B00F;
        rhc_instance.gprs[26] = 0x0000B0E0;
        rhc_instance.gprs[27] = 0x0000BE00;
        rhc_instance.gprs[28] = 0x0000BEe0;
        rhc_instance.gprs[29] = 0x0000Beef;
        rhc_instance.gprs[30] = 0xD0000000;

        uint64_t pc = 0xc0ffe;
        asm("adr %[res], 0x0"
                : [res] "=r" (pc)::);
        rhc_instance.gprs[2] = pc;
	uint64_t register fid asm("x0") = RSI_HOST_CALL;
	uint64_t register _addr asm("x1") = ipa_val;
	asm("smc 0;");
}
