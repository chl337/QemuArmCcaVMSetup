#include "psci.h"

#define PSCI_AFFINITY_INFO	0xC4000004
#define PSCI_CPU_OFF		0x84000002
#define PSCI_CPU_ON		0xC4000003
#define PSCI_CPU_SUSPEND	0xC4000001
#define PSCI_FEATURES		0x8400000A	
#define PSCI_SYSTEM_OFF		0x84000008
#define PSCI_SYSTEM_RESET	0x84000009
#define PSCI_VERSION		0x84000000


uint32_t psci_affinity(uint64_t mpid, uint64_t lowest_affinity)
{
	uint64_t register fid asm("x0") = PSCI_AFFINITY_INFO;
	uint64_t register target_affinity asm("x1") __attribute__((unused)) = mpid;
	uint64_t register low_affinity  asm("x0") __attribute__((unused)) = lowest_affinity;

	asm("smc 0");

	return (uint32_t)fid;	
}

void psci_power_off() 
{
	uint64_t register fid asm("x0") __attribute__((unused)) = PSCI_CPU_OFF;
	asm("smc 0;");
} 

uint32_t psci_cpu_on(uint64_t cpu, uint64_t pc_address, uint32_t ctx)
{
	uint64_t register fid 		asm("x0") = PSCI_CPU_ON;
	uint64_t register target_cpu 	asm("x1") __attribute__((unused)) = cpu;
	uint64_t register entry_address asm("x2") __attribute__((unused)) = pc_address;
	uint64_t register ctx_id 	asm("x3") __attribute__((unused)) = ctx;

	asm("smc 0;");

	return (uint32_t) fid;
}

void psci_cpu_suspend(uint64_t power_state, uint64_t pc_address, uint32_t ctx)
{
	uint64_t register fid 		asm("x0") __attribute__((unused)) = PSCI_CPU_SUSPEND;
	uint64_t register pstate	asm("x1") __attribute__((unused)) = power_state;
	uint64_t register entry_address asm("x2") __attribute__((unused)) = pc_address;
	uint64_t register ctx_id 	asm("x3") __attribute__((unused)) = ctx;

	asm("smc 0;");

}

uint32_t psci_features(uint32_t func_id)
{
	uint64_t register fid asm("x0") = PSCI_FEATURES;
	uint32_t register psci_func_id asm("x1") __attribute__((unused)) = func_id;
	asm("smc 0;");
	return (uint32_t) fid;
} 

void psci_system_off()
{
	uint64_t register fid asm("x0") __attribute__((unused)) = PSCI_SYSTEM_OFF;
	asm("smc 0;");
}

void psci_system_reset()
{
	uint64_t register fid asm("x0") __attribute__((unused)) = PSCI_SYSTEM_RESET;
	asm("smc 0;");
}

uint64_t psci_version()
{
	uint64_t register fid asm("x0") = PSCI_VERSION;
	asm("smc 0;");
	return (uint64_t)fid;
}
