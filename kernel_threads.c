#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "tinyos.h"
#include <assert.h>

//allagmeno2
PTCB* create_PTCB(Task task, int argl, void* args, PCB* pcb)
{
    PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));

    /* Push back to CURPROC->PTCB_list */
    rlnode_init(& ptcb->PTCB_node, ptcb); 
    rlist_push_back(& (pcb->PTCB_list), & ptcb->PTCB_node);

     /* Initialize PTCB attrs*/
    ptcb->main_task = task;
    ptcb->owner_pcb = pcb;
    ptcb->ref_counter = 1;
    ptcb->exitval = DEFAULT_EXITVAL; // Default val  
    ptcb->j_state = JOINABLE;
    ptcb->cv = COND_INIT;
    ptcb->exit_state = P_NOT_EXITED;
    ptcb->argl = argl;
    ptcb->args = args;

    return  ptcb;
}

/** 
  @brief Create a new thread in the current process.
  */
//allagmeno
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
    TCB* tcb;
    PTCB* ptcb = (PTCB*) xmalloc(sizeof(PTCB));

    /* Initialize PTCB attrs*/
    rlnode_init(& ptcb->PTCB_node, ptcb); 
    if(task!=NULL)
    {
      tcb = spawn_thread(CURPROC, start_PTCB_thread);
      ptcb->main_thread=tcb; 
      ptcb->main_thread->owner_ptcb = ptcb;
    }

    /* Push back to CURPROC->PTCB_list */
    rlist_push_back(& (CURPROC->PTCB_list), & ptcb->PTCB_node);

    ptcb->main_task = task;
    ptcb->owner_pcb = CURPROC;
    ptcb->ref_counter = 0;
    ptcb->exitval = DEFAULT_EXITVAL; // Default val  
    ptcb->j_state = JOINABLE;
    ptcb->cv = COND_INIT;
    ptcb->exit_state = P_NOT_EXITED;
    ptcb->argl = argl;
    ptcb->args=args;
    
    wakeup(ptcb->main_thread);

    return (Tid_t) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{

  PTCB* ptcb = (PTCB*) tid;

  // TODO-maybe: Check if ptcb (or tid) exists ???

  if((ptcb->j_state != DETACHED) && (ptcb->owner_pcb == CURPROC) )
  {

    ptcb->ref_counter++;

    // Spontaneous-wake-up protection loop
    while( (ptcb->exit_state!=P_EXITED) ){
      // fprintf(stderr, "%s\n", "in JOIN() before kernel_wait()");
      kernel_wait(& ptcb->cv, SCHED_USER); // ???
      // fprintf(stderr, "%s\n", "in JOIN() after kernel_wait()");
    }

    // fprintf(stderr, "%s\n", "in JOIN() after while()");

    ptcb->ref_counter--;

    if(exitval!=NULL) *exitval = ptcb->exitval;

    int final_exitval;

    if( (ptcb->ref_counter<0) && (ptcb->j_state=EXITED) ) 
    {
      // fprintf(stderr, "%s%d\n", "in JOIN() before delete_PTCB(), ref_counter:", ptcb->ref_counter);
      delete_PTCB(ptcb);

      final_exitval = 0; // Success
    }
    else 
    {
      if( (ptcb==NULL) || (ptcb == CURPTHREAD) || (ptcb->j_state == DETACHED) ) 
        final_exitval = -1; // Error
    }
      
    return final_exitval;  

  }
  
  // fprintf(stderr, "%s\n", "in JOIN before (return -1)"); 
  return -1; //Error

}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb = (PTCB*) tid;
  if(ptcb != NULL && ptcb->exit_state!= P_EXITED)
  {
    ptcb->j_state = DETACHED;
    ptcb->ref_counter = 0;
    kernel_broadcast(& ptcb->cv);
    return 0; // Success 
  }
  
  return -1; // Error
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  // fprintf(stderr, "%s%lu\n","in ThreadExit with ptcb tid:", (Tid_t) CURPTHREAD );
  CURPTHREAD->exitval = exitval;
  CURPTHREAD->exit_state = P_EXITED;
  kernel_broadcast(& CURPTHREAD->cv);
  // fprintf(stderr, "%s\n", "in ThreadExit() after kernel_broadcast(), before kernel_sleep()" );
  // TODO-maybe: if(DETACHED): kernel_sleep() and delete_PTCB()
  kernel_sleep(EXITED, SCHED_USER);
}
  
