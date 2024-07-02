#include "../headers/lib.h"

// Qui ci andranno le funzioni relative a virtual memory, paging, ecc.

void uTLB_RefillHandler()
{
    // Sezione 3

    //"1. Determine the page number (denoted as p) of the missing TLB entry by inspecting EntryHi in
    // the saved exception state located at the start of the BIOS Data Page."
    state_t exc_state = (state_t *)BIOSDATAPAGE;
    int p = GETVPN(exc_state->entry_hi);

    //"2. Get the Page Table entry for page number p for the Current Process. This will be located in
    // the Current Process’s Page Table, which is part of its Support Structure."
    pte_entry_t target = current_process->p_supportStruct->sup_privatePgTbl[p];

    //"3. Write this Page Table entry into the TLB. This is a three-set process:
    //(a) setENTRYHI
    //(b) setENTRYLO
    //(c) TLBWR"
    setENTRYHI(target.pte_entryHI);
    setENTRYLO(target.pte_entryLO);
    TLBWR();

    //"4. Return control to the Current Process to retry the instruction that caused the TLB-Refill event:
    // LDST on the saved exception state located at the start of the BIOS Data Page."
    LDST(exc_state);
}

void TLB_update(pteEntry_t pte)
{
    // Sezione 5.2
}

int getPage()
{
    // Sezione 5.4

    // If SPT contains a free slot, return that one
    for (int j = 0; j < POOLSIZE; j++)
    {
        if (swap_pool_table[j].sw_asid == NOPROC)
            return j;
    }

    // If SPT had no free slot, select a page to replace with a FIFO policy
    static int fifoIndex = -1; // NOTE: static variable
    fifoIndex = (fifoIndex + 1) % POOLSIZE;
    return fifoIndex;
}

void handleDirtyPage(int index)
{
    unsigned int curr_status = getSTATUS();
    setSTATUS(curr_status & ~IECON); // Disable interrupts

    // TODO

    curr_status = getSTATUS();
    setSTATUS(curr_status | IECON); // Re-enable interrupts
}

void pager()
{
    // Sezione 4.2

    // 1. Obtain the pointer to the Current Process’s Support Structure requesting the GetSupportData
    // service to the SSI. Important: Level 4/Phase 3 exception handlers are limited in their interaction
    // with the Nucleus and its data structures to the functionality of SYSCALLs identified by
    // negative numbers and service requests to the SSI.
    // 6

    // 2. Determine the cause of the TLB exception. The saved exception state responsible for this
    // TLB exception should be found in the Current Process’s Support Structure for TLB exceptions
    //(sup_exceptState[0]’s Cause register).

    // 3. If the Cause is a TLB-Modification exception, treat this exception as a program trap [Section
    // 9], otherwise continue.

    // 4. Gain mutual exclusion over the Swap Pool table sending a message to the swap_table PCB and
    // waiting for a response.

    // 5. Determine the missing page number (denoted as p): found in the saved exception state’s EntryHi.

    // 6. Pick a frame, i, from the Swap Pool. Which frame is selected is determined by the μPandOS
    // page replacement algorithm [Section 5.4].

    // 7. Determine if frame i is occupied; examine entry i in the Swap Pool table.

    // 8. If frame i is currently occupied, assume it is occupied by logical page number k belonging to
    // process x (ASID) and that it is “dirty” (i.e. been modified):
    //(a) Update process x’s Page Table: mark Page Table entry k as not valid. This entry is easily
    // accessible, since the Swap Pool table’s entry i contains a pointer to this Page Table entry.
    //(b) Update the TLB, if needed. The TLB is a cache of the most recently executed process’s
    // Page Table entries. If process x’s page k’s Page Table entry is currently cached in the TLB
    // it is clearly out of date; it was just updated in the previous step.
    // Important: This step and the previous step must be accomplished atomically [Section
    // 5.3].
    //(c) Update process x’s backing store. Write the contents of frame i to the correct location on
    // process x’s backing store/flash device [Section 5.1]. Treat any error status from the write
    // operation as a program trap [Section 9].

    // 9. Read the contents of the Current Process’s backing store/flash device logical page p into frame
    // i [Section 5.1]. Treat any error status from the read operation as a program trap [Section 9].

    // 10. Update the Swap Pool table’s entry i to reflect frame i’s new contents: page p belonging to the
    // Current Process’s ASID, and a pointer to the Current Process’s Page Table entry for page p.

    // 11. Update the Current Process’s Page Table entry for page p to indicate it is now present (V bit)
    // and occupying frame i (PFN field).

    // 12. Update the TLB. The cached entry in the TLB for the Current Process’s page p is clearly out
    // of date; it was just updated in the previous step.
    // Important: This step and the previous step must be accomplished atomically [Section 5.3].

    // 13. Release mutual exclusion over the Swap Pool table sending a message to the swap_table PCB.

    // 14. Return control to the Current Process to retry the instruction that caused the page fault: LDST
    // on the saved exception state.
}