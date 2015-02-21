/*
 * tch_sys.c
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
 *
 *
 *
 *      this module contains most essential part of tch kernel.
 *
 */



#include "tch.h"

#include "tch_hal.h"
#include "tch_kernel.h"
#include "tch_mailq.h"
#include "tch_msgq.h"
#include "tch_mem.h"
#include "tch_nclib.h"
#include "tch_port.h"
#include "tch_thread.h"
#include "tch_time.h"
#include "tch_board.h"

const uint8_t testimg[] = { 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa7, 0x0, 0x0, 0x0, 0xaa, 0x0, 0x0, 0x0,
		0x1b, 0x2, 0x0, 0x0, 0x84, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x4, 0x0,
		0x0, 0x0, 0xd4, 0x1, 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0xa8, 0x1, 0x0, 0x0,
		0x6, 0x0, 0x0, 0x0, 0x8, 0x1, 0x0, 0x0, 0xa, 0x0, 0x0, 0x0, 0x2b, 0x0,
		0x0, 0x0, 0xb, 0x0, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0x11, 0x0, 0x0, 0x0,
		0x10, 0x2, 0x0, 0x0, 0x12, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x13, 0x0,
		0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x90, 0xb5, 0x85, 0xb0, 0x0, 0xaf, 0x78, 0x60, 0x13, 0x4c, 0x7c, 0x44,
		0x13, 0x4b, 0xe3, 0x58, 0x1a, 0x46, 0x0, 0x23, 0x13, 0x60, 0x0, 0x23,
		0xfb, 0x60, 0x11, 0x4b, 0x7b, 0x44, 0x1b, 0x68, 0x1, 0x33, 0x10, 0x4a,
		0x7a, 0x44, 0x13, 0x60, 0xfb, 0x68, 0x1, 0x33, 0xfb, 0x60, 0xb, 0x4b,
		0xe3, 0x58, 0x1b, 0x68, 0x1, 0x33, 0x9, 0x4a, 0xa2, 0x58, 0x13, 0x60,
		0x7b, 0x68, 0x1b, 0x68, 0x5b, 0x69, 0xa, 0x20, 0x98, 0x47, 0x7b, 0x68,
		0x1b, 0x6b, 0x1b, 0x68, 0xdb, 0x69, 0x6, 0x4a, 0x7a, 0x44, 0x10, 0x46,
		0x98, 0x47, 0xe7, 0xe7, 0xe2, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0,
		0x5c, 0xff, 0xff, 0xff, 0x54, 0xff, 0xff, 0xff, 0x18, 0x0, 0x0, 0x0,
		0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0xa,
		0x0, 0x0, 0x0, 0x0, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3,
		0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x84, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x3, 0x0, 0x5, 0x0, 0x1f, 0x0, 0x0, 0x0, 0x18, 0x2, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x10, 0x0, 0xa, 0x0, 0x1, 0x0, 0x0, 0x0, 0x18, 0x2, 0x0,
		0x0, 0x4, 0x0, 0x0, 0x0, 0x11, 0x0, 0xa, 0x0, 0xa, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0x1, 0x0, 0x25, 0x0, 0x0,
		0x0, 0x1c, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0xa, 0x0, 0x5,
		0x0, 0x0, 0x0, 0x85, 0x0, 0x0, 0x0, 0x6c, 0x0, 0x0, 0x0, 0x12, 0x0, 0x5,
		0x0, 0x11, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10,
		0x0, 0x1, 0x0, 0x18, 0x0, 0x0, 0x0, 0x84, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x10, 0x0, 0x5, 0x0, 0x0, 0x63, 0x6e, 0x74, 0x0, 0x6d, 0x61, 0x69,
		0x6e, 0x0, 0x5f, 0x73, 0x64, 0x61, 0x74, 0x61, 0x0, 0x5f, 0x65, 0x64,
		0x61, 0x74, 0x61, 0x0, 0x5f, 0x73, 0x74, 0x65, 0x78, 0x74, 0x0, 0x5f,
		0x73, 0x62, 0x73, 0x73, 0x0, 0x5f, 0x65, 0x62, 0x73, 0x73, 0x0, 0x0,
		0x3, 0x0, 0x0, 0x0, 0xa, 0x0, 0x0, 0x0, 0x9, 0x0, 0x0, 0x0, 0x7, 0x0,
		0x0, 0x0, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x8, 0x0, 0x0, 0x0, 0x74, 0x0, 0x0, 0x0, 0x15, 0x4, 0x0, 0x0, 0x3d,
		0x22, 0x0, };

#define SYSTSK_ID_SLEEP             ((int) -3)
#define SYSTSK_ID_ERR_HANDLE        ((int) -2)
#define SYSTSK_ID_RESET             ((int) -1)

struct tch_monitor_t {
	tch_mtxId mtx;
	uint32_t mval;
};


static DECLARE_THREADROUTINE(systhreadRoutine);
static DECLARE_THREADROUTINE(idle);
static DECLARE_SYSTASK(kernelTaskHandler);


static struct tch_monitor_t tch_busyMonitor;
static tch RuntimeInterface;
const tch* tch_rti = &RuntimeInterface;

static tch_threadId mainThread = NULL;
static tch_threadId idleThread = NULL;

static tch_mailqId sysTaskQ;
tch_memId sharedMem;
const struct tch_bin_descriptor BIN_DESC;
static tch_threadId sysThread;


/***
 *  Initialize Kernel including...
 *  - initailize device driver and bind HAL interface
 *  - initialize architecture dependent part and bind port interface
 *  - bind User APIs to API type
 *  - initialize Idle thread
 */
void tch_kernelInit(void* arg){


	/*Bind API Object*/

	RuntimeInterface.uStdLib = tch_initStdLib();

	RuntimeInterface.Thread = Thread;
	RuntimeInterface.Mtx = Mtx;
	RuntimeInterface.Sem = Sem;
	RuntimeInterface.Condv = Condv;
	RuntimeInterface.Barrier = Barrier;
	RuntimeInterface.Mempool = Mempool;
	RuntimeInterface.MailQ = MailQ;
	RuntimeInterface.MsgQ = MsgQ;
	RuntimeInterface.Mem = uMem;
	RuntimeInterface.Event = Event;


	tch_listInit((tch_lnode_t*) &tch_procList);
	uint8_t* shMemBlk = kMem->alloc(TCH_CFG_SHARED_MEM_SIZE);
	sharedMem = tch_memCreate(shMemBlk,TCH_CFG_SHARED_MEM_SIZE);

	/**
	 *  dynamic binding of dependecy
	 */

	if(!tch_kernel_initPort()){
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	}

	tch_port_kernel_lock();

	tch_threadCfg thcfg;
	thcfg._t_name = "sysloop";
	thcfg._t_routine = systhreadRoutine;
	thcfg.t_proior = KThread;
	thcfg.t_stackSize = 1 << 10;
	tch_currentThread = ROOT_THREAD;
	sysThread = Thread->create(&thcfg,(void*)tch_rti);

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(sysThread);
	return;
}


tchStatus tch_kernel_exec(const void* loadableBin,tch_threadId* nproc){
	if(!loadableBin)
		return tchErrorParameter;

	tch_loadableInfo header = (tch_loadableInfo) loadableBin;
	loadableBin += sizeof(struct tch_dynamic_bin_meta_struct);
	uint8_t* exImg = kMem->alloc(header->b_sz);
	uStdLib->string->memcpy(exImg,loadableBin,header->b_sz);
	tch_thread_routine entry = (tch_thread_routine)(((uint32_t)exImg + header->b_entry) | 0x1); // the address value for indirect branch in ARM should be 1 in its '0' bit, otherwise usagefault
	tch_threadCfg thcfg;
	thcfg.t_heapSize = 0;
	thcfg.t_stackSize = (1 << 9);
	thcfg._t_name = "Test";
	thcfg._t_routine = entry;
	thcfg.t_proior = Normal;

	tch_threadId th = tch_currentThread;
	tch_currentThread = ROOT_THREAD;
	*nproc = Thread->create(&thcfg,NULL);
	tch_currentThread = th;
	return tchOK;
}



void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_thread_header* cth = NULL;
	tch_thread_header* nth = NULL;
	tch_exc_stack* sp = NULL;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp = (tch_exc_stack*)tch_port_getThreadSP();
		sp++;
		tch_currentThread->t_tslot = 0;
		_impure_ptr = &tch_currentThread->t_reent;
		tch_port_setThreadSP((uint32_t)sp);
		if((arg1 = tch_threadIsValid(tch_currentThread)) == tchOK){
			tch_port_kernel_unlock();
		}else{
			tch_schedDestroy(tch_currentThread,arg1);
		}
		if(tch_currentThread == sysThread)
			tch_port_enable_privilegedThread();
		else
			tch_port_disable_privilegedThread();
		break;
	case SV_THREAD_START:              // start thread first time
		tch_schedStart((tch_threadId) arg1);
		break;
	case SV_THREAD_YIELD:
		tch_schedSleep(arg1,mSECOND,WAIT);    // put current thread in pending queue and will be waken up at given after given time duration is passed
		break;
	case SV_THREAD_SLEEP:
		tch_schedSleep(arg1,SECOND,SLEEP);
		break;
	case SV_THREAD_JOIN:
		if(((tch_thread_header*)arg1)->t_state != TERMINATED){                                 // check target if thread has terminated
			tch_schedSuspend((tch_thread_queue*)&((tch_thread_header*)arg1)->t_joinQ,arg2);    //if not, thread wait
			break;
		}
		tch_kernelSetResult(tch_currentThread,tchOK);                                           //..otherwise, it returns immediately
		break;
	case SV_THREAD_RESUME:
		tch_schedResumeM((tch_thread_queue*) arg1,1,arg2,TRUE);
		break;
	case SV_THREAD_RESUMEALL:
		tch_schedResumeM((tch_thread_queue*) arg1,SCHED_THREAD_ALL,arg2,TRUE);
		break;
	case SV_THREAD_SUSPEND:
		tch_schedSuspend((tch_thread_queue*)arg1,arg2);
		break;
	case SV_THREAD_TERMINATE:
		cth = (tch_thread_header*) arg1;
		tch_schedTerminate((tch_threadId) cth,arg2);
		break;
	case SV_THREAD_DESTROY:
		cth = (tch_thread_header*) arg1;
		tch_schedDestroy((tch_threadId) cth,arg2);
		break;
	case SV_EV_WAIT:
		cth = (tch_thread_header*) tch_currentThread;
		tch_kernelSetResult(cth,tch_event_kwait(arg1,arg2));
		break;
	case SV_EV_UPDATE:
		cth = (tch_thread_header*) tch_currentThread;
		tch_kernelSetResult(cth,tch_event_kupdate(arg1,arg2));
		break;
	case SV_EV_DESTROY:
		cth = (tch_thread_header*) tch_currentThread;
		tch_kernelSetResult(cth,tch_event_kdestroy(arg1));
		break;
	case SV_MSGQ_PUT:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kput((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_GET:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kget((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_DESTROY:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kdestroy((tch_msgqId) arg1));
		break;
	case SV_MAILQ_ALLOC:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kalloc((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_FREE:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kfree((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_DESTROY:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kdestroy((tch_mailqId) arg1,0));
		break;
	case SV_HAL_ENABLE_ISR:
		cth = tch_currentThread;
		NVIC_DisableIRQ(arg1);
		NVIC_SetPriority(arg1,arg2);
		NVIC_EnableIRQ(arg1);
		tch_kernelSetResult(cth,tchOK);
		break;
	case SV_HAL_DISABLE_ISR:
		cth = tch_currentThread;
		NVIC_DisableIRQ(arg1);
		tch_kernelSetResult(cth,tchOK);
		break;
	}
}

tchStatus tch_kernel_enableInterrupt(IRQn_Type irq,uint32_t priority){
	if(tch_port_isISR())
		return tchErrorISR;
	return tch_port_enterSv(SV_HAL_ENABLE_ISR,irq,priority);
}

tchStatus tch_kernel_disableInterrupt(IRQn_Type irq){
	if(tch_port_isISR())
		return tchErrorISR;
	return tch_port_enterSv(SV_HAL_DISABLE_ISR,irq,0);
}

tchStatus tch_kernel_postSysTask(int id,tch_sysTaskFn fn,void* arg){
	tchStatus result = tchOK;
	tch_sysTask* task = tch_rti->MailQ->alloc(sysTaskQ,tchWaitForever,&result);
	task->arg = arg;
	task->fn = fn;
	task->id = id;
	task->prior = tch_currentThread->t_prior;
	task->status = tchOK;
	if(result == tchOK)
		tch_rti->MailQ->put(sysTaskQ,task);
	return result;
}


void tch_kernelSetBusyMark(){
	if(RuntimeInterface.Mtx->lock(tch_busyMonitor.mtx,tchWaitForever) != tchOK)
		return;
	tch_busyMonitor.mval++;
	RuntimeInterface.Mtx->unlock(tch_busyMonitor.mtx);
}

void tch_kernelClrBusyMark(){
	if(RuntimeInterface.Mtx->lock(tch_busyMonitor.mtx,tchWaitForever) != tchOK)
		return;
	tch_busyMonitor.mval--;
	RuntimeInterface.Mtx->unlock(tch_busyMonitor.mtx);
}

BOOL tch_kernelIsBusy(){
	return !tch_busyMonitor.mval;
}

void tch_kernel_errorHandler(BOOL dump,tchStatus status){

	/**
	 *  optional register dump
	 */
	while(1){
		asm volatile("NOP");
	}
}

static DECLARE_THREADROUTINE(systhreadRoutine){
	// perform runtime initialization

	tchEvent evt;
	tch_sysTask* task = NULL;

	RuntimeInterface.Device = tch_kernel_initHAL(&RuntimeInterface);
	if(!RuntimeInterface.Device)
		tch_kernel_errorHandler(FALSE,tchErrorValue);

	RuntimeInterface.Time = tch_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);

	if(tch_kernel_initCrt0(&RuntimeInterface) != tchOK)
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	sysTaskQ = MailQ->create(sizeof(tch_sysTask),TCH_SYS_TASKQ_SZ);
	if(!sysTaskQ)
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	tch_busyMonitor.mval = 0;
	tch_busyMonitor.mtx = env->Mtx->create();

	tch_threadId th = tch_currentThread;
	tch_currentThread = ROOT_THREAD;      // create thread as root

	tch_threadCfg thcfg;
	thcfg._t_routine = (tch_thread_routine) main;
	thcfg.t_stackSize = (uint32_t) &Main_Stack_Top - (uint32_t) &Main_Stack_Limit;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";

	mainThread = Thread->create(&thcfg,&RuntimeInterface);
	tch_currentThread = th;

	thcfg._t_routine = (tch_thread_routine) idle;
	thcfg.t_stackSize = (uint32_t)&Idle_Stack_Top - (uint32_t)&Idle_Stack_Limit;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	idleThread = Thread->create(&thcfg,NULL);
	tch_threadId id;
	tch_kernel_exec(testimg,&id);




	if((!mainThread) || (!idleThread))
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	tch_boardInit(&RuntimeInterface);

	tch_port_disable_privilegedThread();


	Thread->start(idleThread);
	Thread->start(mainThread);
	Thread->start(id);

	uStdLib->string->memset(&evt,0,sizeof(tchEvent));



	// loop for handling system tasks (from ISR / from any user thread)
	while(TRUE){
		evt = MailQ->get(sysTaskQ,tchWaitForever);
		if(evt.status == tchEventMail){
			task = (tch_sysTask*) evt.value.p;
			task->fn(task->id,&RuntimeInterface,task->arg);
			MailQ->free(sysTaskQ,evt.value.p);
		}
	}
	return tchOK;    // unreachable point
}


static DECLARE_THREADROUTINE(idle){
	tch_rtcHandle* rtc_handle = env->Thread->getArg();

	while(TRUE){
		// some function entering sleep mode
		if((!tch_busyMonitor.mval) && (tch_currentThread->t_tslot > 5) && tch_schedIsEmpty()  && tch_systimeIsPendingEmpty())
			tch_kernel_postSysTask(SYSTSK_ID_SLEEP,kernelTaskHandler,rtc_handle);
		tch_hal_enterSleepMode();
		// some function waking up from sleep mode
 	}
	return 0;
}


static DECLARE_SYSTASK(kernelTaskHandler){
	struct tch_err_descriptor* err = NULL;
	struct tm lt;
	switch(id){
	case SYSTSK_ID_SLEEP:
		tch_hal_disableSystick();
		tch_hal_setSleepMode(LP_LEVEL1);
		tch_hal_suspendSysClock();
		tch_hal_enterSleepMode();
		tch_hal_resumeSysClock();
		tch_hal_setSleepMode(LP_LEVEL0);
		tch_hal_enableSystick();
		break;
	case SYSTSK_ID_ERR_HANDLE:
		// handle err
		err = arg;
		env->Time->getLocaltime(&lt);
		env->uStdLib->stdio->iprintf("\r\n===SYSTEM FAULT (SUBJECT : %s) (ERR TYPE : %d)===\n",getThreadHeader(err->subj)->t_name,err->errtype);
		env->uStdLib->stdio->iprintf("\r\n===Local Time : %d/%d/%d %d:%d:%d ===============\n",lt.tm_year + 1900,lt.tm_mon + 1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
		env->Thread->terminate(err->subj,err->errno);
		kMem->free(err);
		break;
	case SYSTSK_ID_RESET:
		tch_boardDeinit(env);
		tch_port_reset();
		break;
	}
}


void tch_kernelOnHardFault(int fault,int type){
	struct tch_err_descriptor* err = kMem->alloc(sizeof(struct tch_err_descriptor));
	err->errtype = fault;
	err->errno = type;
	err->subj = tch_currentThread;
	switch(fault){
	case HARDFAULT_UNRECOVERABLE:
		tch_kernel_postSysTask(SYSTSK_ID_RESET,kernelTaskHandler,err);
		tch_port_clearFault(type);
		return;
	case HARDFAULT_RECOVERABLE:
		tch_kernel_postSysTask(SYSTSK_ID_ERR_HANDLE,kernelTaskHandler,err);
		return;
	}
	while(TRUE){
		asm volatile("NOP");
	}
}



