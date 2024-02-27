#include "../headers/lib.h"

extern pcb_PTR current_process;

void exceptionHandler()
{
    // Here, BIOSDATAPAGE contains the processor state present during the exception
    // The cause of this exception is encoded in the .ExcCode field of the Cause register (Cause.ExcCode)
    // in the saved exception state [Section 3.3-pops].
    // Tip: To get the ExcCode you can use getCAUSE(), the constants GETEXECCODE and CAUSESHIFT.
}