/*
 * tch_port.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#ifndef TCHtch_port_H_
#define TCHtch_port_H_

#include "tch_ptypes.h"


#define GROUP_PRIOR_Pos                (uint8_t) (7)
#define SUB_PRIOR_Pos                  (uint8_t) (4)
#define GROUP_PRIOR(x)                 (uint8_t) ((x & 1) << (GROUP_PRIOR_Pos - SUB_PRIOR_Pos))
#define SUB_PRIOR(y)                   (uint8_t) ((y & 7))

#define MODE_KERNEL                    (uint32_t)(1 << GROUP_PRIOR_Pos)                                 // execution priority of kernel only supervisor call can interrupt
#define MODE_USER                      (uint32_t)(0)


#define HANDLER_SVC_PRIOR              (uint32_t)(GROUP_PRIOR(0) | SUB_PRIOR(0))
#define HANDLER_SYSTICK_PRIOR          (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(1))
#define HANDLER_HIGH_PRIOR             (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(2))
#define HANDLER_NORMAL_PRIOR           (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(3))
#define HANDLER_LOW_PRIOR              (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(4))

#define tch_port_setThreadSP(sp)           __set_PSP(sp)
#define tch_port_getThreadSP()             __get_PSP()
#define tch_port_setHandlerSP(sp)          __set_MSP(sp)
#define tch_port_getHandlerSP()            __get_MSP()

#define HARDFAULT_UNRECOVERABLE                    (-1)
#define HARDFAULT_RECOVERABLE                      (-2)


/*******************************************************************************************/
/****************   porting functions invoked in kernel mode        ************************/
/*******************************************************************************************/

/**
 * \brief enable ISR
 * \note should be invoked in kernel mode
 */
extern void tch_port_enableISR(void);
/**
 *
 */
extern void tch_port_disableISR(void);
/***
 *  Kernal lock action
 *   - to guarantee kernel operation not interrupted or preempted
 */
extern void tch_port_kernel_lock(void);
/***
 *  Kernel Unlock action
 *   - to allow interrupt or thread preemption when it's needed
 */
extern void tch_port_kernel_unlock(void);
extern BOOL tch_port_isISR();
extern int tch_port_clearFault(int fault);
extern int tch_port_reset();

extern void tch_port_switchContext(void* nth,void* cth,tchStatus kret) __attribute__((naked,noreturn));
extern void* tch_port_makeInitialContext(uaddr_t sp,uaddr_t initfn);
extern void tch_port_jmpToKernelModeThread(uaddr_t routine,uword_t arg1,uword_t arg2,uword_t arg3);



extern int tch_port_enterSv(word_t sv_id,uword_t arg1,uword_t arg2);
extern int tch_port_exclusiveCompareUpdate(uaddr_t dest,uword_t comp,uword_t update);
extern int tch_port_exclusiveCompareDecrement(uaddr_t dest,uword_t comp);



typedef struct _tch_exc_stack tch_exc_stack;
typedef struct _tch_thread_context tch_thread_context;
typedef struct _arm_sbrtn_ctx arm_sbrtn_ctx;



#endif /* TCHtch_port_H_ */
