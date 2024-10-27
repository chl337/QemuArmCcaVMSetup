#Running with Qemu
`qemu-system-aarch64 -cpu cortex-a57 -smp 1 -machine virt -m 64M  -S -s --nographic  -device loader,file=kernel_start.bin,addr=0x42000000 -device loader,addr=0x42000360,cpu-num=0 -serial mon:stdio`  

#Switch to Virtual Memory Mode
maintenance packet Qqemu.PhyMemMode:0



