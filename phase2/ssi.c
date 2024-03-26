#include "../headers/lib.h"
#include <umps/libumps.h>

extern struct list_head ready_queue;
//extern pcb_PTR ssi_pcb; //probabilmente viene preso globalmente e non serve dichiararlo di nuovo (nel caso però c'è)


void createProcess(ssi_payload_PTR p){//inizializza un nuovo pcb e lo inserisce nella ready_queue
    pcb_t* new_pcb;
    reinitPcb(new_pcb);

    ssi_create_process_PTR create_info = p->arg;
    new_pcb->p_s = create_info->state;
    new_pcb->p_supportStruct = create_info->support;

    insertProcQ(&m->m_sender->p_list,new_pcb);
    insertChild(&m->m_sender->p_sib,new_pcb);

    insertProcQ(&ready_queue,new_pcb);
}

void terminateProcess(ssi_payload_PTR p){//elimina o il processo che fa la send o quello specificato in arg dalla ready_queue

    if(p->arg == NULL){
        outProcQ(&ready_queue,m->m_sender);
    }else{
        outProcQ(&ready_queue,p->arg);
    }

}

typedef struct WaitingDoIO{
    pct_PTR pcb;
    unsigned int device;
}

//Inizializzare una nuova lista dei processi bloccati per il DoIO. Non la metto in quanto non so se ne è già una presente

void DoIO(){

    insertProcQ(blocked_process, current_process);   
    insertProcQ(blocked_process_DoIO, (current_process, //Indirizzo device IO)) //So anche che non si può inserire perchè serve un pcb ma non penso sia efficace inserire un nuovo campo su PCB. Dobbiamo per forza creare una nuova lista?
    current_process = NULL;
    WAIT();

    unsigned int device_address = //Indirizzo device IO;
    unsigned int ack_command = ACK;
    SYSCALL(SENDMESSAGE, device_address, (unsigned int)(&ack_command), 0);


    ssi_do_io_t do_io = {
        .commandAddr = command,
        .commandValue = value,
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &do_io,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int) ssi_pcb, (unsigned int)(&response), 0);

    WaitingDoIO current_process = headProcQ(blocked_process_DoIO); //Stessa cosa ultimo commento
    
    while(WaitingDoIO != NULL && WaitingDoIO->device != device_address){
        WaitingDoIO = WaitingDoIO->next;
    }
    insertProcQ(ready_queue, WaitingDoIO->pcb);
    removeProcQ(blocked_process_DoIO, WaitingDoIO);
    //Devo modificare lo stato del pcb? In realtà lo stato non è mai stato in pausa quindi dovrebbe essere ancora in run no?
 }

signed int GetCPUTime(){//ciclo che prende i p_time da tutti i processi nella ready_queue e restituisce la somma
    pcb_PTR p;
    signed int time = 0;
    list_for_each_entry(p,&ready_queue,p_list){
        time += p->p_time;
    }
    return time;
}

support_t* GetSupportData(msg_t* m){//ciclo che scorre la ready_queue e trova il pcb che ha fatto la send per restituire la struttura di supporto
  //come nome dentro al pcb per gli elementi list_head della ready_queue ho utilizzato p_list ???
    pcb_PTR p == NULL;
    list_for_each_entry(p,&ready_queue,p_list){
        if(p == m->m_sender){
            return p->p_supportStruct;
        }
    }
    return NULL;
}

pcb clock_wait_list = NULL; // Global list of PCBs waiting for clock tick

void WaitForClock() {
    pcb_PTR current_process = m_sender;
    
    pcb_PTR tmp = headProcQ(clock_wait_list)
    while(tmp != NULL){
        pcb_PTR aux = tmp;
        tmp = tmp->next;
        insertProcQ(ready_queue, aux);
        removeProcQ(clock_wait_list);
    }

    insertProcQ(clock_wait_list, current_process);
}


void ssi()
{
  /*TODO:
  creare variabili per salvare i risultati di alcune funzioni ???
  "If service does not match any of those provided by the SSI, the SSI should terminate the process
and its progeny" ???
  SSIRequest ???

  */
    ssi_pcb = headProcQ(&ready_queue);
    msg_t* msg;
    while(true){
        if((msg = popMessage(&ssi_pcb->msg_inbox)) != NULL){//msg in inbox
            ssi_payload_PTR payload = msg->m_payload;
            switch(payload->service_code){
                case CREATEPROCESS://create process ???
                    createProcess(payload);
                    break;
                case TERMPROCESS:
                    terminateProcess(payload);
                    break;
                case DOIO:
                    DoIO();
                    break;
                case GETTIME:
                    GetCPUTime();
                    break;
                case CLOCKWAIT:
                    break;
                case GETSUPPORTPTR:
                    GetSupportData(msg);
                    break;
                case GETPROCESSID:
                    /*tocca salvarlo da qualche parte*/ msg->m_sender->p_pid;
                    break;

            }
        }
    }
}
