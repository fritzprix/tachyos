/*
 * main.c
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"
#include "lld/fmo_adc.h"
#include "util/tch_lib.h"
#include "core/fmo_task.h"
#include "hld/atcmd_bt.h"

static uint16_t als_buffer[256];
static atcmd_bt_server_adapter btadapter;

static tchThread_t* adcReader;
volatile BOOL btActive;
static uint32_t adcReaderStack[1 << 9];

static BOOL onConnect(atcmd_bt_device* device);
static BOOL onDisconnect(atcmd_bt_device* device);
static THREAD_ROUTINE(adcReadLoop);

void* main(void* arg) {

	btadapter.onConnected = onConnect;
	btadapter.onDisconnected = onDisconnect;
	tch_hld_atcmdBt_openServer("STM32F_Dev", 115200, &btadapter, ActOnSleep);

	char reportcb[100];
	char convBuffer[100];
	int cnt = 0;

	while (1) {
		//	adc1->read(adc1,als_buffer,20,10);
		tchThread_sleep(1);
	}
	return 0;
}

BOOL onConnect(atcmd_bt_device* device) {
	char cbuf[100];
	adcReader = tchThread_create(adcReaderStack, 1 << 8, adcReadLoop,
			THREAD_PRIORITY_NORMAL, device, "adcreader");
	tchThread_start(adcReader);
	return TRUE;
}

BOOL onDisconnect(atcmd_bt_device* device) {
	btActive = FALSE;
//	tchThread_join(adcReader);
	return TRUE;
}

THREAD_ROUTINE(adcReadLoop) {
	struct _atcmd_bt_device_t* btdevice = (atcmd_bt_device*) arg;
	tch_adc_cfg acfg;
	btActive = TRUE;
	tch_lld_adc_initCfg(&acfg);
	acfg.ADC_SampleFreqInHz = 10000;
	acfg.ADC_ChCnt = 0;
	acfg.ADC_ChCfg_List = NULL;
	acfg.ADC_Resolution = ADC_Resolution_12b;
	acfg.ADC_SampleHold = ADC_SampleTime_3Cycles;
	adc1->open(adc1, &acfg, ActOnSleep);
	tch_printCstr("ADC Thread Start\n");

	if(btdevice->connect(btdevice)){
		tch_printCstr("Device Connected\n");
	}else{
		tch_printCstr("Device already Connected\n");
	}

	tch_ostream* btostream = btdevice->openOutputStream(btdevice);
	tchThread_sleep(10);
	char cbuf[20];
	while (btActive) {
		uint8_t idx = 0;
		int len = 0;
		adc1->read(adc1, als_buffer, 100, 10);
		while (idx < 100) {
			tch_strconcat(cbuf, "V : ", tch_itoa(als_buffer[idx++], cbuf + 5, 10));
			len = tch_strconcat(cbuf, cbuf, "\n\r");
			if(!btostream->write(btostream, cbuf, len, NULL)){
				tch_printCstr("write fail!\n");
				idx = 100;
				btActive = FALSE;
			}
		}
	}
	btostream->close(btostream);
	btdevice->disconnect(btdevice);
	adc1->close(adc1);
	tch_printCstr("ADC Looper Finished\n");
	return NULL ;
}
