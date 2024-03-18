#ifndef LIB_H
#define LIB_H

#include <umps/libumps.h>
#include <umps/types.h>
#include <limits.h>
#include "const.h"
#include "listx.h"
#include "msg.h"
#include "pcb.h"
#include "types.h"

extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head blocked_queue;
extern int process_count;
extern int softblock_count;
extern state_t *exc_state;
extern struct list_head pcbFree_h;

void exceptionHandler(), interruptHandler(), scheduler(), ssi(), uTLB_RefillHandler();

#endif