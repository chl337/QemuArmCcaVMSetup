#include "defines.h"
#define STACK_SIZE 0x10000
uint8_t __attribute__(( aligned(0x10000) )) __attribute__(( section(".stack") ))  _kernel_stack[STACK_SIZE] = {0};

uint64_t get_stack_base()
{
	return (uint64_t)&_kernel_stack[STACK_SIZE];
}
