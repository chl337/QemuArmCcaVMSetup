#ifndef _TIMER_H
#define _TIMER_H




#define RTCDR    0x000 //uint32 RO  Data Register, RTCDR
#define RTCMR    0x004 //uint32 RW  Match Register, RTCMR 
#define RTCLR    0x008 //uint32 RW  Load Register, RTCLR 
#define RTCCR    0x00C //uint1  RW  Control Register, RTCCR 
#define RTCIMSC  0x010 //uint1  RW  Interrupt Mask Set or Clear register, RTCIMSC
#define RTCRIS   0x014 //uint1  RO  Raw Interrupt Status, RTCRIS
#define RTCMIS   0x018 //uint1  RO  Masked Interrupt Status, RTCMIS
#define RTCICR   0x01C //uint1  WO  Interrupt Clear Register, RTCICR


#define ENABLE_FLAG 0x1

#endif
