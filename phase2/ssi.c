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

void DoIO(){//TODO

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

void ssi()
{
  /*TODO:
  ci sono funzioni che ritornano tipi diversi (invece che return salviamo i risultati in variabili ???)
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
                    return GetCPUTime();
                    break;
                case CLOCKWAIT:
                    break;
                case GETSUPPORTPTR:
                    return GetSupportData(msg);
                    break;
                case GETPROCESSID:
                    return msg->m_sender->p_pid;
                    break;

            }
        }
    }
}
