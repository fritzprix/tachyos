/*
 * tch_sdio.c
 *
 *  Created on: 2016. 1. 27.
 *      Author: innocentevil
 */


#include "tch_sdio.h"
#include "tch_hal.h"
#include "tch_board.h"
#include "tch_dma.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_dbg.h"



#ifndef SDIO_CLASS_KEY
#define SDIO_CLASS_KEY				(uint16_t) 0x3F65
#endif

#define MAX_SDIO_DEVCNT				10
#define MAX_TIMEOUT_MILLS			1000
#define CMD_CLEAR_MASK				((uint32_t)0xFFFFF800)
#define SDIO_ACMD41_ARG_HCS			(1 << 30)
#define SDIO_ACMD41_ARG_XPC			(1 << 28)
#define SDIO_ACMD41_ARG_S18R		(1 << 24)

#define SDIO_VO_TO_VW_SHIFT			15
#define SDIO_OCR_VW27_28			(1 << 15)
#define SDIO_OCR_VW28_29			(1 << 16)
#define SDIO_OCR_VW29_30			(1 << 17)
#define SDIO_OCR_VW30_31			(1 << 18)
#define SDIO_OCR_VW31_32			(1 << 19)
#define SDIO_OCR_VW32_33			(1 << 20)
#define SDIO_OCR_VW33_34			(1 << 21)
#define SDIO_OCR_VW34_35			(1 << 22)
#define SDIO_OCR_VW35_36			(1 << 23)

#define SDIO_R3_READY				((uint32_t) 1 << 31)
#define SDIO_R3_CCS					((uint32_t) 1 << 30)
#define SDIO_R3_UHSII				((uint32_t) 1 << 29)
#define SDIO_R3_S18A				((uint32_t) 1 << 24)

/*
 * SDIO CMD index List
 * */
#define CMD_GO_IDLE_STATE			((uint8_t) 0)
#define CMD_ALL_SEND_CID			((uint8_t) 2)
#define CMD_SEND_RCA				((uint8_t) 3)
#define CMD_SET_DSR					((uint8_t) 4)		// program the DSR of all cards
#define CMD_SELECT					((uint8_t) 7)		// command toggle a card between stand-by and transfer state
#define CMD_SEND_IF_COND			((uint8_t) 8)		// send interface condition
#define CMD_SEND_CSD				((uint8_t) 9)		// query card specific data(CSD)
#define CMD_SEND_CID				((uint8_t) 10)		// query card identification(CID)
#define CMD_SIGVL_SW				((uint8_t) 11)		// switch to 1.8V bus signal level
#define CMD_STOP_TRANS				((uint8_t) 12)		// forces the card to stop transmission
#define CMD_SEND_STATUS				((uint8_t) 13)		// addressed card send its status
#define CMD_GO_INACTIVE				((uint8_t) 15)		// make an addressed card into the inactive state

#define CMD_SET_BLKLEN				((uint8_t) 16)
#define CMD_READ_SBLK				((uint8_t) 17)
#define CMD_READ_MBLK				((uint8_t) 18)
#define CMD_SEND_TUNE				((uint8_t) 19)
#define CMD_SPD_CLS_CTRL			((uint8_t) 20)
#define CMD_SET_BLKCNT				((uint8_t) 23)
#define CMD_WRITE_SBLK				((uint8_t) 24)
#define CMD_WRITE_MBLK				((uint8_t) 25)
#define CMD_PROG_CSD				((uint8_t) 27)
#define CMD_SET_WP					((uint8_t) 28)
#define CMD_CLR_WP					((uint8_t) 29)
#define CMD_SEND_WP					((uint8_t) 30)
#define CMD_ERASE_WBLK_START		((uint8_t) 32)
#define CMD_ERASE_WBLK_END			((uint8_t) 33)
#define CMD_ERASE					((uint8_t) 38)
#define CMD_DEF_DPS					((uint8_t) 40)
#define CMD_LOCK_UNLOCK				((uint8_t) 42)
#define CMD_APP_CMD					((uint8_t) 55)
#define CMD_GEN_CMD					((uint8_t) 56)

#define ACMD_SET_BUS_WIDTH			((uint8_t) 6)
#define ACMD_SD_STATUS				((uint8_t) 13)
#define ACMD_SEND_NUM_WR_BLK		((uint8_t) 22)
#define ACMD_SEND_WR_BLK_ERASE_CNT	((uint8_t) 23)
#define ACMD_SEND_OP_COND			((uint8_t) 41)
#define ACMD_SET_CLR_CARD_DETECT	((uint8_t) 42)
#define ACMD_SEND_SCR				((uint8_t) 51)


/*
 * Flags for SDIO Handle
 * */
#define SDIO_HANDLE_FLAG_DMA		((uint32_t) 0x10000)
#define SDIO_HANDLE_FLAG_BUSY		((uint32_t) 0x20000)
#define SDIO_HANDLE_FLAG_S18R		((uint32_t) 0x40000)


/*
 * Flags of SD Card Status Field @ R1 response
 * */
#define SDC_STATUS_OOR				((uint32_t) 1 << 31)
#define SDC_STATUS_ADDRESS_ERR		((uint32_t) 1 << 30)
#define SDC_STATUS_BLEN_ERR			((uint32_t) 1 << 29)
#define SDC_STATUS_ERASE_SEQ_ERR	((uint32_t) 1 << 28)
#define SDC_STATUS_ERASE_PARM_ERR	((uint32_t) 1 << 27)
#define SDC_STATUS_WP_VIOLATION_ERR	((uint32_t) 1 << 26)
#define SDC_STATUS_CARD_IS_LOCKED	((uint32_t) 1 << 25)
#define SDC_STATUS_LOCK_FAIL		((uint32_t) 1 << 24)
#define SDC_STATUS_CMDCRC_ERR		((uint32_t) 1 << 23)
#define SDC_STATUS_ILLCMD_ERR		((uint32_t) 1 << 22)
#define SDC_STATUS_CARDECC_ERR		((uint32_t) 1 << 21)
#define SDC_STATUS_CC_ERR			((uint32_t) 1 << 20)
#define SDC_STATUS_ERR				((uint32_t) 1 << 19)
#define SDC_STATUS_CSD_OVW_ERR		((uint32_t) 1 << 16)
#define SDC_STATUS_WP_ERASE_SKIP	((uint32_t) 1 << 15)
#define SDC_STATUS_CARD_ECC_DIS		((uint32_t) 1 << 14)
#define SDC_STATUS_ERASE_RESET		((uint32_t) 1 << 13)
#define SDC_STATUS_CUR_STATE_MSK	((uint32_t) 0xF << 9)
#define SDC_STATUS_DATA_RDY			((uint32_t) 1 << 8)
#define SDC_STATUS_APP_CMD			((uint32_t) 1 << 5)
#define SDC_STATUS_AKE_SEQ_ERR		((uint32_t) 1 << 3)



#define SDIO_VALIDATE(ins)	do {\
	((struct tch_sdio_handle_prototype*) ins)->status |= (0xffff & ((uint32_t) ins ^ SDIO_CLASS_KEY));\
}while(0)

#define SDIO_INVALIDATE(ins) do {\
	((struct tch_sdio_handle_prototype*) ins)->status &= ~0xffff;\
}while(0)


#define SDIO_ISVALID(ins)			((((struct tch_sdio_handle_prototype*) ins)->status & 0xffff) == (((uint32_t) ins ^ SDIO_CLASS_KEY) & 0xffff))



#define SET_BUSY(sdio) do {\
	set_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status |= SDIO_HANDLE_FLAG_BUSY;\
}while(0)

#define CLR_BUSY(sdio) do {\
	clear_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status &= ~SDIO_HANDLE_FLAG_BUSY;\
}while(0)

#define IS_BUSY(sdio) 		((struct tch_sdio_handle_prototype*) sdio)->status & SDIO_HANDLE_FLAG_BUSY

#define SET_DMA_MODE(sdio)		sdio->status |= SDIO_HANDLE_FLAG_DMA
#define CLR_DMA_MODE(sdio)		sdio->status &= ~SDIO_HANDLE_FLAG_DMA
#define IS_DMA_MODE(sdio)		(sdio->status & SDIO_HANDLE_FLAG_DMA)

#define MAX_BUS_WIDTH			((uint8_t) 4)

#define SDIO_D0_PIN				((uint8_t) 8)
#define SDIO_D1_PIN				((uint8_t) 9)
#define SDIO_D2_PIN				((uint8_t) 10)
#define SDIO_D3_PIN				((uint8_t) 11)
#define SDIO_CK_PIN				((uint8_t) 12)

#define SDIO_CMD_PIN			((uint8_t) 2)

#define SDIO_DETECT_PIN			((uint8_t) 8)

#define SDIO_DETECT_PORT		tch_gpio0
#define SDIO_DATA_PORT			tch_gpio2
#define SDIO_CMD_PORT			tch_gpio3


struct tch_sdio_device_info {
 	SdioDevType				type;
#define SDIO_DEV_FLAG_CAP_LARGE				((uint16_t) 1 << 1)
#define SDIO_DEV_FLAG_S18A_SUPPORT			((uint16_t) 1 << 2)
#define SDIO_DEV_FLAG_UHSII_SUPPORT			((uint16_t) 1 << 3)
#define SDIO_DEV_FLAG_READY					((uint16_t) 1 << 4)
 	uint32_t				flags;
 	uint16_t 				caddr;
 	uint8_t					wblen;
 	uint8_t					rblen;
 	BOOL					is_blk_erase_supported;
 	BOOL 					is_protected;
 	BOOL					is_locked;
};

struct tch_sdio_handle_prototype  {
	struct tch_sdio_handle 				pix;
	uint32_t 							status;
	uint16_t							vopt;
	uint8_t								dev_cnt;
	tch_sdioDevInfo 					dev_infos[MAX_SDIO_DEVCNT];
	tch_gpioHandle*						data_port;
	tch_gpioHandle*						cmd_port;
	tch_gpioHandle*						detect_port;
	tch_dmaHandle*						dma;
	tch_eventId							evId;
	const tch_core_api_t*				api;
	tch_mtxId							mtx;
	tch_condvId							condv;
};


typedef struct sdio_lock_blk_header {
	uint8_t		flag;
	uint8_t		pwdlen;
} sdio_lock_blk_header_t;

typedef struct sdio_lock_blk {
	sdio_lock_blk_header_t	header;
	char					pwd;
} sdio_lock_blk_t;



#define SDIO_RESP_SHORT		((sdio_resp_t) 1)
#define SDIO_RESP_LONG		((sdio_resp_t) 2)
typedef uint8_t		sdio_resp_t;



/*** private function declaration ***/
static int tch_sdio_init();
static void tch_sdio_exit();


static uint32_t sdio_mmc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);


static tchStatus sdio_send_cmd(struct tch_sdio_handle_prototype* ins, uint8_t cmdidx, uint32_t arg, BOOL waitresp, sdio_resp_t rtype, uint32_t* resp,uint32_t timeout);
static tchStatus sd_select_device(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device);
static tchStatus sd_deselect_device(struct tch_sdio_handle_prototype* ins);
static tchStatus sd_read_csd(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint32_t* csd);

__USER_API__ static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt);
__USER_API__ static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api,const tch_sdioCfg_t* config,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_release(tch_sdioHandle_t sdio);


__USER_API__ static tchStatus tch_sdio_handle_device_reset(tch_sdioHandle_t sdio);
__USER_API__ static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
//__USER_API__ static tchStatus tch_sdio_handle_deviceInfo(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info);
__USER_API__ static SdioDevType tch_sdio_handle_getDeviceType(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static BOOL tch_sdio_handle_isProtectedEnabled(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static uint64_t tch_sdio_handle_getMaxBitrate(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static uint64_t tch_sdio_handle_getCapacity(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock);
__USER_API__ static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd);
__USER_API__ static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd);

__USER_RODATA__ tch_hal_module_sdio_t SDIO_Ops = {
		.initCfg = tch_sdio_initCfg,
		.alloc = tch_sdio_alloc,
		.release = tch_sdio_release

};


static tch_mtxCb mtx;
static tch_condvCb condv;

static tch_hal_module_dma_t* dma;
static tch_hal_module_gpio_t* gpio;


static int tch_sdio_init()
{
	dma = NULL;
	gpio = NULL;
	tch_mutexInit(&mtx);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_SDIO, SDIO_CLASS_KEY, &SDIO_Ops, FALSE);
}

static void tch_sdio_exit()
{
	tch_kmod_unregister(MODULE_TYPE_SDIO, SDIO_CLASS_KEY);
	tch_condvDeinit(&condv);
	tch_mutexDeinit(&mtx);
}


MODULE_INIT(tch_sdio_init);
MODULE_EXIT(tch_sdio_exit);


static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt)
{
	cfg->bus_width = buswidth;
	cfg->lpopt = lpopt;
	cfg->v_opt = SDIO_VO31_32 | SDIO_VO32_33;
	cfg->s18r_enable = FALSE;
}

static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api, const tch_sdioCfg_t* config,uint32_t timeout)
{
	if(!config)
		return NULL;

	struct tch_sdio_handle_prototype* ins = NULL;
	const tch_sdio_bs_t* sdio_bs = &SDIO_BD_CFGs[0];
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];

	if(api->Mtx->lock(&mtx,timeout) != tchOK)
		goto INIT_FAIL;
	while(sdio_hw->_handle != NULL)
	{
		if(api->Condv->wait(&condv,&mtx,timeout) != tchOK)
		{
			api->Dbg->print(api->Dbg->Normal, 0, "SDIO Allocation Timeout\n\r");
			goto INIT_FAIL;
		}
	}
	ins = (struct tch_sdio_handle_prototype*) api->Mem->alloc(sizeof(struct tch_sdio_handle_prototype));
	sdio_hw->_handle = ins;
	if(!ins)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "Out of Memory\n\r");
		goto INIT_FAIL;
	}

	mset(ins,0, sizeof(struct tch_sdio_handle_prototype));

	dma = tch_kmod_request(MODULE_TYPE_DMA);
	if(!dma && sdio_bs->dma != DMA_NOT_USED)
	{

		api->Dbg->print(api->Dbg->Normal, 0, "DMA is Not available\n\r");
		goto INIT_FAIL;
	}

	gpio = api->Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "GPIO is Not available\n\r");
		goto INIT_FAIL;
	}

	gpio_config_t ioconfig;
	gpio->initCfg(&ioconfig);
	ioconfig.Mode = GPIO_Mode_AF;
	ioconfig.Otype = GPIO_Otype_PP;
	ioconfig.popt = config->lpopt;
	ioconfig.Speed = GPIO_OSpeed_100M;
	ioconfig.Af = sdio_bs->afv;
	ins->data_port = gpio->allocIo(api, SDIO_DATA_PORT, (1 << SDIO_D0_PIN | 1 << SDIO_D1_PIN |
			                                             1 << SDIO_D2_PIN | 1 << SDIO_D3_PIN | 
			                                             1 << SDIO_CK_PIN),
														&ioconfig, timeout);

	ins->cmd_port = gpio->allocIo(api, SDIO_CMD_PORT, 1 << SDIO_CMD_PIN, &ioconfig, timeout);

	if(!ins->data_port || !ins->cmd_port)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "GPIO(s) Not available\n\r");
		goto INIT_FAIL;
	}

	if(sdio_bs->dma != DMA_NOT_USED)
	{
		tch_DmaCfg dmacfg;
		dma->initCfg(&dmacfg);
		dmacfg.BufferType = DMA_BufferMode_Normal;
		dmacfg.Ch = sdio_bs->ch;
		dmacfg.Dir = DMA_Dir_MemToPeriph;
		dmacfg.FlowCtrl = DMA_FlowControl_Periph;
		dmacfg.Priority = DMA_Prior_High;
		dmacfg.mAlign = DMA_DataAlign_Word;
		dmacfg.mBurstSize = DMA_Burst_Inc4;
		dmacfg.mInc = TRUE;
		dmacfg.pAlign = DMA_DataAlign_Word;
		dmacfg.pBurstSize = DMA_Burst_Inc4;
		dmacfg.pInc = FALSE;
		ins->dma = dma->allocate(api, sdio_bs->dma, &dmacfg, timeout, config->lpopt);
		SET_DMA_MODE(ins);

		if(!ins->dma)
		{
			api->Dbg->print(api->Dbg->Normal, 0, "DMA is not abilable\n\r");
			goto INIT_FAIL;
		}
	}
	else
	{
		CLR_DMA_MODE(ins);
		//TODO : implement Non DMA mode
	}

	// set ops for SDIO handle
	mset(ins->dev_infos, 0, sizeof(struct tch_sdio_device_info) * MAX_SDIO_DEVCNT);

	ins->pix.deviceReset = tch_sdio_handle_device_reset;
	ins->pix.deviceId = tch_sdio_handle_device_id;
	ins->pix.getDeviceType = tch_sdio_handle_getDeviceType;
	ins->pix.isProtectEnabled = tch_sdio_handle_isProtectedEnabled;
	ins->pix.getMaxBitrate = tch_sdio_handle_getMaxBitrate;
	ins->pix.getCapacity = tch_sdio_handle_getCapacity;
	ins->pix.erase = tch_sdio_handle_erase;
	ins->pix.eraseForce = tch_sdio_handle_eraseForce;
	ins->pix.lock = tch_sdio_handle_lock;
	ins->pix.unlock = tch_sdio_handle_unlock;
	ins->pix.readBlock = tch_sdio_handle_readBlock;
	ins->pix.writeBlock = tch_sdio_handle_writeBlock;
	ins->pix.setPassword = tch_sdio_handle_setPassword;

	ins->mtx = api->Mtx->create();
	ins->condv = api->Condv->create();
	ins->evId = api->Event->create();
	ins->vopt = config->v_opt;
	ins->dev_cnt = 0;

	if(!ins->mtx || !ins->condv || !ins->evId)
	{
		goto INIT_FAIL;
	}

	api->Mtx->unlock(&mtx);

	*sdio_hw->_clkenr |= sdio_hw->clkmsk;			// power main clock enable
	if(config->lpopt == ActOnSleep)
		*sdio_hw->_lpclkenr |= sdio_hw->lpclkmsk;	// if sdio should be operational in idle mode, enable SDIO lp clock

	*sdio_hw->_rstr |= sdio_hw->rstmsk;				// reset sdio
	*sdio_hw->_rstr &= ~sdio_hw->rstmsk;

	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	sdio_reg->POWER |= SDIO_POWER_PWRCTRL;			// power up

	// set interrupts
	if(!ins->dma)
	{
		// DMA support is not available, enable interrupt for data handling
		sdio_reg->MASK |= (SDIO_MASK_SDIOITIE | SDIO_MASK_RXDAVLIE);
	}
	else
	{
		sdio_reg->CLKCR |= SDIO_CLKCR_HWFC_EN;		// hardware flow control enable
	}
	// enable error handling interrupt
	sdio_reg->MASK |= (SDIO_MASK_RXOVERRIE | SDIO_MASK_TXUNDERRIE | SDIO_MASK_CMDRENDIE | SDIO_MASK_CMDSENTIE | SDIO_MASK_CCRCFAILIE);
	sdio_reg->CLKCR |= SDIO_CLKCR_PWRSAV;			// power save mode enabled
	sdio_reg->CLKCR |= (SDIO_CLKCR_CLKDIV & 198);	// set clock divisor 48MHz / 200  because should be less than 400kHz
	sdio_reg->CLKCR |= SDIO_CLKCR_CLKEN;			// set sdio clock enable
	sdio_reg->DCTRL |= SDIO_DCTRL_DTEN;

	// set clock register for Identification phase

	if(config->s18r_enable)
	{
		ins->status |= SDIO_HANDLE_FLAG_S18R;
	}
	else
	{
		ins->status &= ~ SDIO_HANDLE_FLAG_S18R;
	}


	ins->api = api;
	tch_enableInterrupt(sdio_hw->irq, HANDLER_HIGH_PRIOR);
	SDIO_VALIDATE(ins);

	return (tch_sdioHandle_t) ins;

INIT_FAIL:
	if(ins)
	{
		if(ins->mtx)
			api->Mtx->destroy(ins->mtx);
		if(ins->condv)
			api->Condv->destroy(ins->condv);
		if(ins->evId)
			api->Event->destroy(ins->evId);
		if(ins->cmd_port)
			ins->cmd_port->close(ins->cmd_port);
		if(ins->data_port)
			ins->data_port->close(ins->data_port);
		api->Mem->free(ins);
	}

	sdio_hw->_handle = NULL;
	api->Condv->wakeAll(&condv);
	api->Mtx->unlock(&mtx);
	return NULL;
}

static tchStatus tch_sdio_release(tch_sdioHandle_t sdio)
{
	if(!sdio)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;

	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	const tch_core_api_t* api = ins->api;
	if(api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return tchErrorResource;
	while(IS_BUSY(sdio))
	{
		if(api->Condv->wait(ins->condv,ins->mtx, tchWaitForever) != tchOK)
			return tchErrorResource;
	}
	SET_BUSY(sdio);
	api->Mtx->unlock(ins->mtx);

	ins->cmd_port->close(ins->cmd_port);
	ins->data_port->close(ins->data_port);
	dma->freeDma(ins->dma);

	*sdio_hw->_rstr |= sdio_hw->rstmsk;
	*sdio_hw->_rstr &= ~sdio_hw->rstmsk;

	*sdio_hw->_lpclkenr &= ~sdio_hw->lpclkmsk;
	*sdio_hw->_clkenr &= ~sdio_hw->clkmsk;

	SDIO_INVALIDATE(ins);
	CLR_BUSY(ins);
	api->Mtx->destroy(ins->mtx);
	api->Condv->destroy(ins->condv);
	api->Event->destroy(ins->evId);
	api->Mem->free(ins);
	return tchOK;
}


static tchStatus tch_sdio_handle_device_reset(tch_sdioHandle_t sdio)
{
	if(!sdio)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	return sdio_send_cmd(ins, CMD_GO_IDLE_STATE, 0, FALSE, SDIO_RESP_SHORT, NULL, MAX_TIMEOUT_MILLS);			// send reset
}


static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt)
{
	if(!sdio || !devIds)
		return 0;
	if(!SDIO_ISVALID(sdio))
		return 0;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	if(ins->api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return 0;
	while(IS_BUSY(ins))
	{
		if(ins->api->Condv->wait(ins->condv, ins->mtx, tchWaitForever) != tchOK)
			return 0;
	}
	SET_BUSY(ins);
	if(ins->api->Mtx->unlock(ins->mtx) != tchOK)
		return 0;
	uint32_t resp[4];

	sdio_send_cmd(ins, CMD_SEND_IF_COND, 0x1FF, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);		// send op cond
	if(resp[0] != 0x1FF)
		return 0;


	switch(type){
	case MMC:
		resp[0] = sdio_mmc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	case SDC:
		resp[0] = sdio_sdc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	case SDIOC:
		resp[0] = sdio_sdioc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	default:
		resp[0] = 0;
	}

	ins->api->Mtx->lock(ins->mtx,tchWaitForever);
	CLR_BUSY(ins);
	ins->api->Condv->wakeAll(ins->condv);
	ins->api->Mtx->unlock(ins->mtx);
	return resp[0];
}

static SdioDevType tch_sdio_handle_getDeviceType(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}

static BOOL tch_sdio_handle_isProtectedEnabled(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}

static uint64_t tch_sdio_handle_getMaxBitrate(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}
static uint64_t tch_sdio_handle_getCapacity(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}


static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}

static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock)
{

}

static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd)
{

}

static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd)
{

}

static uint32_t sdio_mmc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{

}

static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{
	const tch_core_api_t* api = ins->api;

	//ensure max identification count less than MAX_SDIO_DEVCNT
	if(max_idcnt > MAX_SDIO_DEVCNT)
		max_idcnt = MAX_SDIO_DEVCNT;

	tchStatus res = tchOK;
	uint32_t pdevcnt = ins->dev_cnt;
	uint32_t arg = 0;
	uint32_t resp[4];


	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;



	while(res != tchEventTimeout && (ins->dev_cnt < max_idcnt))
	{
		// send card initialization command with OCR setting
		res = tchOK;
		sdio_send_cmd(ins, CMD_APP_CMD, 0, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
		mset(resp,0, 4 * sizeof(uint32_t));
		arg = SDIO_ACMD41_ARG_HCS;
		if(ins->status & SDIO_HANDLE_FLAG_S18R)
		{
			arg |= SDIO_ACMD41_ARG_S18R;
		}
		arg |= (ins->vopt << SDIO_VO_TO_VW_SHIFT);
		res = sdio_send_cmd(ins, ACMD_SEND_OP_COND, arg , TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);

		if((resp[0] & SDIO_R3_READY))
		{
			mset(&ins->dev_infos[ins->dev_cnt], 0 ,sizeof(struct tch_sdio_device_info));			// clear device info
			ins->dev_infos[ins->dev_cnt].type = SDC;

			if(resp[0] & SDIO_R3_UHSII)
				ins->dev_infos[ins->dev_cnt].flags |= SDIO_DEV_FLAG_UHSII_SUPPORT;
			if(resp[0] & SDIO_R3_CCS)
				ins->dev_infos[ins->dev_cnt].flags |= SDIO_DEV_FLAG_CAP_LARGE;

			resp[0] = 0;
			sdio_send_cmd(ins, CMD_ALL_SEND_CID, 0, TRUE, SDIO_RESP_LONG, resp, MAX_TIMEOUT_MILLS);		// request CID

			resp[0] = 0;
			sdio_send_cmd(ins, CMD_SEND_RCA, 0, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);		// request RCA Publish
			ins->dev_infos[ins->dev_cnt].caddr = (resp[0] >> 16);
			devIds[ins->dev_cnt] = &ins->dev_infos[ins->dev_cnt];


			// TODO : if UHS further initialization required
			if(ins->dev_infos[ins->dev_cnt].flags & SDIO_DEV_FLAG_UHSII_SUPPORT)
			{

			}


			sdio_send_cmd(ins, CMD_SELECT, (ins->dev_infos[ins->dev_cnt].caddr << 16), TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
			if(resp[0] & SDC_STATUS_CARD_IS_LOCKED)
			{
				// TODO : handle card is locked
			}

			sdio_send_cmd(ins, CMD_SEND_STATUS,(ins->dev_infos[ins->dev_cnt].caddr << 16), TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
			if(((resp[0] & SDC_STATUS_CUR_STATE_MSK) >> 9) != 4)
			{
			}



			sdio_send_cmd(ins, CMD_APP_CMD,0, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
			sdio_send_cmd(ins, ACMD_SET_BUS_WIDTH, 2, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
			//CMD7
			//CMD42
			//ACMD



			ins->dev_cnt++;
		}
	}
	return ins->dev_cnt - pdevcnt;
}

static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{

}

static tchStatus sdio_send_cmd(struct tch_sdio_handle_prototype* ins, uint8_t cmdidx, uint32_t arg, BOOL waitresp,sdio_resp_t rtype, uint32_t* resp, uint32_t timeout)
{
	if(!ins)
		return tchErrorParameter;
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	int32_t ev;

	sdio_reg->ARG = arg;
	tchStatus res;
	uint32_t cmd = (SDIO_CMD_CMDINDEX & cmdidx) | SDIO_CMD_CPSMEN;
	if(waitresp)
	{
		cmd |= ((rtype == SDIO_RESP_SHORT)? SDIO_CMD_WAITRESP_0 : SDIO_CMD_WAITRESP);
		sdio_reg->CMD = cmd;
		if((res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDREND, timeout)) != tchOK)
			return res;
		ev = ins->api->Event->clear(ins->evId, SDIO_STA_CMDREND);

		if(resp)
			mcpy(resp, &sdio_reg->RESP1, 4 * sizeof(uint32_t));

		if((ev & ~SDIO_STA_CMDREND))
		{
			return tchErrorIo;
		}
	}
	else
	{
		cmd |= SDIO_CMD_ENCMDCOMPL;
		sdio_reg->CMD = cmd;
		if((res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDSENT, timeout)) != tchOK)
			return res;
		ins->api->Event->clear(ins->evId, SDIO_STA_CMDSENT);
	}
	return res;
}

static tchStatus sd_select_device(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	struct tch_sdio_device_info* dinfo = (struct tch_sdio_device_info*) device;
	return sdio_send_cmd(ins,7,(dinfo->caddr << 16),TRUE,SDIO_RESP_SHORT,NULL,MAX_TIMEOUT_MILLS);
}

static tchStatus sd_deselect_device(struct tch_sdio_handle_prototype* ins)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	return sdio_send_cmd(ins,7,0,FALSE,SDIO_RESP_SHORT,NULL,MAX_TIMEOUT_MILLS);
}
static tchStatus sd_read_csd(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint32_t* csd)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	struct tch_sdio_device_info* dinfo = (struct tch_sdio_device_info*) device;
	return sdio_send_cmd(ins,9,(dinfo->caddr << 16), TRUE,SDIO_RESP_LONG, csd,MAX_TIMEOUT_MILLS);
}


// interrupt handler
void SDIO_IRQHandler()
{
	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	struct tch_sdio_handle_prototype* sdio_ins = sdio_hw->_handle;
	const tch_core_api_t* api = sdio_ins->api;
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	if(sdio_reg->STA & SDIO_STA_CMDSENT)
	{
		sdio_reg->ICR |= SDIO_ICR_CMDSENTC;
		api->Event->set(sdio_ins->evId,SDIO_STA_CMDSENT);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Sent\n\r",(sdio_reg->CMD & SDIO_CMD_CMDINDEX));
		return;
	}
	if(sdio_reg->STA & SDIO_STA_CMDREND)
	{
		sdio_reg->ICR |= SDIO_ICR_CMDRENDC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CMDREND);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Responded\n\r",(sdio_reg->RESPCMD & SDIO_RESPCMD_RESPCMD));
		return;
	}
	if(sdio_reg->STA & SDIO_STA_CCRCFAIL)
	{
		sdio_reg->ICR |= SDIO_ICR_CCRCFAILC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CMDREND | SDIO_STA_CCRCFAIL);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Responded\n\r wiht CRC Error", (sdio_reg->RESPCMD & SDIO_RESPCMD_RESPCMD));
		return;
	}
}

