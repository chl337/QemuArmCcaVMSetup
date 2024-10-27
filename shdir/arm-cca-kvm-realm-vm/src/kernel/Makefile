PREFIX := 
#PREFIX := gcc/bin/arm-none-eabi
CC :=$(PREFIX)gcc
LD :=$(PREFIX)ld
OBJCOPY :=$(PREFIX)objcopy
COMMON_OBJECTS := vma.o exception.o main.o stack.o memops.o pl031.o hypercall.o fd.o handler.o execve.o heap.o uart.o string.o  syscall.o arch_ops.o uart_syscall.o relocations.o feature.o psci.o rsi.o

FLAGS :=  -march=native -fno-tree-vectorize -fPIC -g #-march=armv8-a -mpc-relative-literal-loads 

all: kernel_start.bin

memops.o: memops.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

init.o: init.s
	$(CC) -Wall $(FLAGS) -c -o $@ $<

arch-ops.o: arch-ops.s
	$(CC) -Wall $(FLAGS) -c -o $@ $<

string.o: string.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

feature.o: feature.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

exception.o: exception.S
	$(CC) -Wall $(FLAGS) -c -o $@ $<

main.o: main.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

stack.o: stack.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

relocations.o: relocations.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

uart.o: uart.c 
	$(CC) -Wall $(FLAGS) -c -o $@ $<

uart_syscall.o: uart_syscall.c 
	$(CC) -Wall $(FLAGS) -c -o $@ $<

vma.o: vma.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

syscall.o: syscall.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

execve.o: execve.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

fd.o: fd.c 
	$(CC) -Wall $(FLAGS) -c -o $@ $<

handler.o : handler.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

heap.o : heap.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

hypercall.o : hypercall.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

pl031.o : pl031.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

psci.o : psci.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

rsi.o : rsi.c
	$(CC) -Wall $(FLAGS) -c -o $@ $<

mm.bin : mm.s
	$(CC) -Wall mm.s -o mm.o -nostdlib -nostartfiles -g  -static-pie
	$(OBJCOPY) -O binary mm.o mm.bin


image_generator: image_generator.c
	gcc -Wall -g -o image-generator image_generator.c 

kernel_start: start.c image_generator $(COMMON_OBJECTS) 
	$(CC) -g -Wl,-Tlinker.ld.static -fno-tree-vectorize -Wl,-shared -Bsymbolic -z notext -g -Wall  -nostdlib -nostartfiles  -O2 start.c $(COMMON_OBJECTS) -o $@.elf

kernel_start.bin: kernel_start
	./image-generator -b kernel_start.elf kernel_start.bin	

qemu_reloc_kernel: kernel_start.bin

.phony: clean all

clean:
	rm -f *.o kernel.[e,o]* *.elf *.bin image-generator
