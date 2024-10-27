#include <asm/hwcap.h>
#include <sys/auxv.h>
#include "kernel.h"

#define get_cpu_ftr(id) ({         \
              unsigned long __val;   \
              asm("mrs %0, "#id : "=r" (__val));\
              return __val;   \
      })

uint64_t get_feature_ID_AA64ISAR0_EL1() {
	 get_cpu_ftr(ID_AA64ISAR0_EL1);
}
uint64_t get_feature_ID_AA64ISAR1_EL1() {
	 get_cpu_ftr(ID_AA64ISAR1_EL1);
}
uint64_t get_feature_ID_AA64MMFR0_EL1() {
	 get_cpu_ftr(ID_AA64MMFR0_EL1);
}
uint64_t get_feature_ID_AA64MMFR1_EL1() {
	 get_cpu_ftr(ID_AA64MMFR1_EL1);
}
uint64_t get_feature_ID_AA64PFR0_EL1() {
	 get_cpu_ftr(ID_AA64PFR0_EL1);
}
uint64_t get_feature_ID_AA64PFR1_EL1() {
	 get_cpu_ftr(ID_AA64PFR1_EL1);
}
uint64_t get_feature_ID_AA64DFR0_EL1() {
	 get_cpu_ftr(ID_AA64DFR0_EL1);
}
uint64_t get_feature_ID_AA64DFR1_EL1() {
	 get_cpu_ftr(ID_AA64DFR1_EL1);
}
uint64_t get_feature_MIDR_EL1() {
	 get_cpu_ftr(MIDR_EL1);
}
uint64_t get_feature_MPIDR_EL1() {
	 get_cpu_ftr(MPIDR_EL1);
}
uint64_t get_feature_REVIDR_EL1() {
	 get_cpu_ftr(REVIDR_EL1);
}

int cpu_supports_PAN()
{
	uint64_t val = get_feature_ID_AA64MMFR1_EL1();

	return (int)((val & (0b11 << 20))  >> 20);
}
