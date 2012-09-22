/* Declaration of trap handler */
#ifndef JOS_KERN_TRAPHANDLER_H
#define JOS_KERN_TRAPHANDLER_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

// This file declare all exception handler
// 32 exception handlers
void DE();               // divide error
void DB();               // debug exception
void NMI();              // NMI interrupt
void BP();               // breakpoint exception
void OF();               // overflow exception
void BR();               // BOUND range exceeded exception
void UD();               // invalid opcode exception
void NM();               // device not available exception
void DF();               // double fault exception
// trap 9 is reserved
void TS();               // invalid tss exception
void NP();               // segment not present
void SS();               // stack fault exception
void GP();               // general protection exception
void PF();               // page fault exception
// trap 15 is reserved
void MF();               // x87 FPU floating-point error
void AC();               // alignment check exception
void MC();               // machine check exception
void XF();               // SIMD floating-point exception
// interrupt 20 - 31 is reserved
// 32-47 for device IRQs
// interrupt 32 - 47
// reserved

#endif                   // JOS_KERN_TRAPHANDLER_H
