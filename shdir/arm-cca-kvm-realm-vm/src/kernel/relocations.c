#include "linker_defines.h"
#include <stdint.h>

struct RelocStore {
	uint64_t address;
	uint64_t value;
};
struct RelocStore __attribute__(( aligned(4096) )) global_relocations[MAX_RELOCATIONS] = {{0xFF,0xFF},};

