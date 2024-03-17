#include "../headers/lib.h"
#include "p2test.c"

// Declare the Level 3 global variables
int process_count;
int softblock_count;
struct list_head ready_queue;
pcb_PTR current_process;
struct list_head blocked_queue;
pcb_PTR ssi_pcb;

extern void test(); // from p2test.c

// Non so a cosa serva ma era nelle specifiche
void uTLB_RefillHandler()
{
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((state_t *)0x0FFFF000);
}

int main(void)
{

    // Initialize Processor0 Pass Up Vector (qualsiasi cosa sia)
    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    // Initialize phase 1 structures
    initPcbs();
    initMsgs();

    // Initialize the previously declared variables
    process_count = 0;
    softblock_count = 0;
    mkEmptyProcQ(&ready_queue);
    current_process = NULL;
    mkEmptyProcQ(&blocked_queue);
    ssi_pcb = NULL;

    // Load the system-wide Interval Timer with 100 milliseconds (constant PSECOND)
    LDIT(PSECOND);

    // SSI PCB
    ssi_pcb = allocPcb();
    ssi_pcb->p_pid = 0;
    ssi_pcb->p_s.status |= IEPON | IMON; // Enable interrupts, set interrupt mask to all 1s
    ssi_pcb->p_s.status &= ~USERPON;     // Enable kernel mode
    RAMTOP(ssi_pcb->p_s.reg_sp);
    ssi_pcb->p_s.pc_epc = (memaddr)ssi;
    ssi_pcb->p_s.reg_t9 = (memaddr)ssi;
    insertProcQ(&ready_queue, ssi_pcb);
    process_count++;

    // Test PCB
    pcb_PTR test_pcb = allocPcb();
    test_pcb->p_pid = 1;
    test_pcb->p_s.status |= IEPON | IMON | TEBITON; // Enable interrupts, set interrupt mask to all 1s, enable PLT
    test_pcb->p_s.status &= ~USERPON;               // Enable kernel mode
    RAMTOP(test_pcb->p_s.reg_sp);
    test_pcb->p_s.reg_sp -= 2 * PAGESIZE;
    test_pcb->p_s.pc_epc = (memaddr)test;
    test_pcb->p_s.reg_t9 = (memaddr)test;
    insertProcQ(&ready_queue, test_pcb);
    process_count++;

    scheduler();
    return 0;
}
