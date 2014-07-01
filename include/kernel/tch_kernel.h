/*
 * tch_kernel.h
 *
 *  Created on: 2014. 6. 22.
 *      Author: innocentevil
 */

#ifndef TCH_KERNEL_H_
#define TCH_KERNEL_H_

/***
 *   Kernel Interface
 *   - All kernel functions(like scheduling,synchronization) are based on this module
 *   - Initialize kernel enviroment (init kernel internal objects,
 */
#include "tch.h"
#include "lib/tch_absdata.h"
#include "port/acm4f/tch_port.h"
#include "hal/STM_CMx/tch_hal.h"
/***
 *  Supervisor call table
 */
typedef struct tch_kernel_instance{
	tch                     tch_api;
	tch_port_ix*            tch_port;
} tch_kernel_instance;


typedef enum tch_thread_state {
	PENDED = 1,                              // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = 2,                             // state in which thread occupies cpu
	READY = 3,
	WAIT = 4,                                // state in which thread wait for event (wake)
	SLEEP = 5,                               // state in which thread is yield cpu for given amount of time
	TERMINATED = -1                          // state in which thread has finished its task
} tch_thread_state;

typedef struct tch_thread_header {
	tch_genericList_node_t      t_listNode;   ///<extends genericlist node class
	tch_genericList_queue_t     t_joinQ;      ///<thread queue to wait for this thread's termination
	tch_thread_routine          t_fn;         ///<thread function pointer
	const char*                 t_name;       ///<thread name /* currently not implemented */
	void*                       t_arg;        ///<thread arg field
	void*                       t_sys;        ///<reference to kernel handle
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	tch_thread_state            t_state;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint64_t                    t_to;
	tch_thread_context*         t_ctx;
	uint32_t                    t_chks;
} tch_thread_header   __attribute__((aligned(4)));

typedef struct tch_thread_queue{
	tch_genericList_queue_t                  thque;
} tch_thread_queue;



#define SV_RETURN_TO_KTHREAD             ((uint32_t) 0x01)              /**
                                                                      *  Return to temporal thread mode in kernel mode
                                                                      *   - Simplify Thread Context Switching OP.
                                                                      *
                                                                      */
#define SV_EXIT_FROM_SV                  ((uint32_t) 0x02)

#define SV_MTX_LOCK                      ((uint32_t) 0x10)
#define SV_MTX_UNLOCK                    ((uint32_t) 0x11)

#define SV_THREAD_START                  ((uint32_t) 0x20)              ///< Supervisor call id for starting thread
#define SV_THREAD_TERMINATE              ((uint32_t) 0x21)              ///< Supervisor call id for terminate thread      /* Not Implemented here */
#define SV_THREAD_SLEEP                  ((uint32_t) 0x22)              ///< Supervisor call id for yeild cpu for specific  amount of time
#define SV_THREAD_JOIN                   ((uint32_t) 0x23)              ///< Supervisor call id for wait another thread is terminated


extern void tch_kernelInit(void* arg);
extern void tch_kernelSysTick(void);
extern void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2);
extern BOOL tch_kernelThreadIntegrityCheck(tch_thread_id thrtochk);



extern const tch_thread_ix* Thread;
extern const tch_signal_ix* Sig;
extern const tch_timer_ix* Timer;
extern const tch_condv_ix* Condv;
extern const tch_mtx_ix* Mtx;
extern const tch_semaph_ix* Sem;
extern const tch_msgq_ix* MsgQ;
extern const tch_mbox_ix* MailQ;
extern const tch_mpool_ix* Mempool;
extern const tch_hal* Hal;

#endif /* TCH_KERNEL_H_ */
