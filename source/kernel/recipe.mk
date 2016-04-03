INC-y+=$(ROOT_DIR)/include/kernel
INC-y+=$(ROOT_DIR)/include/kernel/autogen
#SRC-y+=$(ROOT_DIR)/source/kernel
SRC-y+=./source/kernel

OBJ-y+=tch_bar.ko\
			 tch_err.ko\
			 tch_event.ko\
			 tch_fs.ko\
			 tch_idle.ko\
			 tch_init.ko\
			 tch_interrupt.ko\
			 tch_kdesc.ko\
			 tch_dbg.ko\
			 tch_kmod.ko\
			 tch_kobj.ko\
			 tch_loader.ko\
			 tch_lock.ko\
			 tch_lwtask.ko\
			 tch_mailq.ko\
			 tch_mpool.ko\
			 tch_msgq.ko\
			 tch_sched.ko\
			 tch_sem.ko\
			 tch_sys.ko\
			 tch_thread.ko\
			 tch_waitq.ko\
			 tch_time.ko\
			 string.ko\
			 time.ko
