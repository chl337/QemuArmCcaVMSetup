#ifndef _PSCI_H
#define _PSCI_H
#include <stdint.h>
#include <stdbool.h>

#define PSCI_SUCCESS			 0
#define PSCI_OFF    			 1
#define PSCI_NOT_SUPPORTED 		-1
#define PSCI_INVALID_PARAMETERS 	-2
#define PSCI_DENIED 			-3
#define PSCI_ALREADY_ON 		-4
#define PSCI_ON_PENDING 		-5
#define PSCI_INTERNAL_FAILURE 		-6
#define PSCI_NOT_PRESENT 		-7
#define PSCI_DISABLED 			-8
#define PSCI_INVALID_ADDRESS 		-9

#define POWER_OFF   0
#define POWER_RESET 1

void psci_cpu_suspend(uint64_t power_state, uint64_t pc_address, uint32_t ctx);
uint32_t psci_cpu_on(uint64_t cpu, uint64_t pc_address, uint32_t ctx);
uint32_t psci_affinity(uint64_t mpid, uint64_t lowest_affinity);
uint32_t psci_features(uint32_t func_id);
void psci_system_reset();
uint64_t psci_version();
void psci_system_off();
void psci_power_off();

#endif
