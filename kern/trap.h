/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];
extern struct Pseudodesc idt_pd;

void trap_init(void);
void trap_init_percpu(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);
extern void DIVIDE();
extern void DEBUG();
extern void NMI();
extern void BRKPT();
extern void OFLOW();
extern void BOUND();
extern void ILLOP();
extern void DEVICE();
extern void DBLFLT();
extern void TSS();
extern void SEGNP();
extern void STACK();
extern void GPFLT();
extern void PGFLT();
extern void FPERR();
extern void ALIGN();
extern void MCHK();
extern void SIMDERR();
extern void IRQTIMER();
extern void IRQKBD();
extern void IRQ2();
extern void IRQ3();
extern void IRQSERIAL();
extern void IRQ5();
extern void IRQ6();
extern void IRQSPURIOUS();
extern void IRQ8();
extern void IRQ9();
extern void IRQ10();
extern void IRQ11();
extern void IRQ12();
extern void IRQ13();
extern void IRQIDE();
extern void IRQ15();
extern void IRQERROR();
extern void SYSCALL();
extern void DEFAULT();

#endif /* JOS_KERN_TRAP_H */
