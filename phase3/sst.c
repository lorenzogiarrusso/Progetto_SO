#include "../headers/lib.h"

// Get TOD value (number of microseconds since the system was last booted/reset)
unsigned int sst_getTOD()
{
    unsigned int tmp;
    STCK(tmp);
    return tmp;
}

// Function to let an U-proc request termination of itself and its SST
unsigned int sst_terminate(int asid)
{
    // TODO
    // Terminazione da fare tramite richiesta all'SSI
    // Ma deve anche informare sulla terminazione il processo test
    // Deve anche invalidare le proprie entry nella swap pool table
    return 42; // Value doesn't matter, it's just as an ACK
}

unsigned int sst_write(support_t *sup, sst_print_t *payload, int dev_type)
{
    payload->string[payload->length] = '\0'; // Ensures string is null-terminated

    pcb_PTR dst = NULL;
    int indexInArray = sup->sup_asid - 1;

    // Fetch pointer to destination from either printer pcbs or terminal pcbs
    if (dev_type == 6)
        dst = printer_processes[indexInArray];
    else if (dev_type == 7)
        dst = terminal_processes[indexInArray];

    SYSCALL(SENDMESSAGE, (unsigned int)dst, (unsigned int)payload->string, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)dest, 0, 0);
    return 42; // Value doesn't matter, it's just as an ACK
}

unsigned int sst_writePrinter(support_t *sup, sst_print_t *payload)
{
    return sst_write(sup, payload, 6);
}

unsigned int sst_writeTerminal(support_t *sup, sst_print_t *payload)
{
    return sst_write(sup, payload, 7);
}

// Function used by an SST to get the pointer to its support from the SSI.
support_PTR get_support_ptr()
{
    support_PTR sup_ptr = NULL;
    ssy_payload_t payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL};
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&sup_ptr), 0);
    return sup_ptr;
}

// Function that implements each SST's infinite loop
void sst()
{
    support_PTR sup_ptr = get_support_ptr();
    state_t *state = &uproc_states[sup->sup_asid - 1];

    pcb_PTR *child = createNewProcess(sup_ptr, state);

    while (1)
    {
        ssi_payload_t *payload = NULL;
        unsigned int sender = 0;
        sender = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)(&payload), 0);
        // Forse non va bene ANYMESSAGE? Può ricevere messaggi anche da qualcuno che non è il figlio? Bohhhh!! In tal caso sender non serve

        unsigned int result = 0;

        if (payload != NULL)
        {
            switch (payload->service_code)
            {
            case GET_TOD:
                result = sst_getTOD();
                break;
            // case TERMINATE: handled in default case
            case WRITEPRINTER:
                res = sst_writePrinter(sup_ptr, (sst_print_PTR)payload->arg);
                break;
            case WRITETERMINAL:
                res = sst_writeTerminal(sup_ptr, (sst_print_PTR)payload->arg);
                break;
            default:
                // Also covers the TERMINATE case
                result = sst_terminate(sup_ptr->sup_asid);
                break;
            }
        }
    }

    SYSCALL(SENDMESSAGE, (unsigned int)sender, result, 0);
}