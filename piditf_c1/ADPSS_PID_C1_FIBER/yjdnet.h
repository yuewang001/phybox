#ifndef YJDNET_H__
#define YJDNET_H__

#ifdef __cplusplus
extern   "C"   {
#endif

#if 1
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

//#include <linux/module.h>
#include <fcntl.h>
#include <semaphore.h>
#include <windriver.h>

#else

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <windriver.h>

#define _GNU_SOURCE

#ifdef _LINUX
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#else //_LINUX
#include <sys/timeb.h>
#endif //_LINUX

#endif

//#if YJDNET_DEBUG
#if 1
#  define yjdnet_debug(args...) printf(args)
#else
#  define yjdnet_debug(args...)
#endif

#undef rdtscll
#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A"(val))

#define MAX_DEVICES		4
#define MAX_SLOTS		10
#define MAX_AI_CHANS	24
#define MAX_AO_CHANS	24
#define MAX_DI_CHANS	40
#define MAX_DO_CHANS	40
#define MAX_GRP_CFGS	4


#define MAX_DEST 10


typedef unsigned char U8;
typedef unsigned char u8;
//typedef unsigned int uint8_t;
typedef short I16;
typedef unsigned short U16;
typedef unsigned short u16;
typedef long I32;
typedef unsigned long U32;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef float F32;
typedef double F64;

enum {
	NoError = 0,
	ErrorUnknownMacAddr = -1,
	ErrorReadSyncConfig = -2,
	ErrorWriteSyncConfig = -3,
	ErrorReadSlotInfo = -4,
	ErrorReadSlotConfig = -5,
	ErrorWriteSlotConfig = -6,
	ErrorReadSlotPara = -7,
	ErrorWriteSlotPara = -8,
	ErrorDIGrpSlot = -9,
	ErrorDIGrpConflict = -10,
	ErrorDOGrpSlot = -11,
	ErrorDOGrpConflict = -12,
	ErrorAIGrpSlot = -13,
	ErrorAIGrpConflict = -14,
	ErrorAOGrpSlot = -15,
	ErrorAOGrpConflict = -16,
	ErrorDIGrpRead = -17,
	ErrorDOGrpWrite = -18,
	ErrorAIGrpRead = -19,
	ErrorAOGrpWrite = -20,
	ErrorAIVoltScale = -21,
	ErrorAOVoltScale = -22,
	ErrorMarkInputClean = -23,
	ErrorMarkInputWaitTimeout = -24,
	ErrorMarkInputWait = -25,
	ErrorCmd = -26,

	CMD_START_SIM = 0x06,
	CMD_STOP_SIM = 0x07,
	CMD_START_REPLAY = 0x08,
	CMD_STOP_REPLAY = 0x09,

	SM_MASTER = 0x00,
	SM_SLAVE = 0x01,

	CARD_INFO_NO = 0x00,
	CARD_INFO_DI = 0x01,
	CARD_INFO_DO = 0x02,
	CARD_INFO_AI = 0x03,
	CARD_INFO_AO = 0x04,

	CARD_CFG_DI_NORMAL = 0x00,
	CARD_CFG_DI_SIM_ELEC = 0x01,
	CARD_CFG_DI_SIM_PULSE_UP = 0x02,
	CARD_CFG_DI_SIM_PULSE_DOWN = 0x03,
	CARD_CFG_DO_NORMAL = 0x00,
	CARD_CFG_DO_SIM = 0x01,
	CARD_CFG_AI_NORMAL = 0x00,
	CARD_CFG_AI_SIM =0x01,
	CARD_CFG_AO_NORMAL = 0x00,
	CARD_CFG_AO_SIM = 0x01,
	CARD_CFG_AO_REPLAY = 0x02,

	MODE_SEM = 0x00,
	MODE_QRY
};

I16 PID_Register(U16 DeviceId, U8 Mode);
I16 PID_Release(U16 DeviceId);
I16 PID_Sync_ReadConfig(U16 DeviceId, U32 *SyncFreq, U16 *SM);
I16 PID_Sync_WriteConfig(U16 DeviceId, U32 SyncFreq, U16 SM);
I16 PID_Slot_ReadInfo(U16 DeviceId, U8 SlotId, U16 *CardInfo, F64 *Slp, F64 *Itcp);
I16 PID_Slot_ReadConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig);
I16 PID_Slot_WriteConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig);
I16 PID_Slot_ReadPara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp);
I16 PID_Slot_WritePara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp);
I16 PID_DI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count);
I16 PID_DO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count);
I16 PID_AI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count);
I16 PID_AO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count);
I16 PID_DI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_DO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_AI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_AO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_AI_VoltScale(U16 *Data, F64 *Slp, F64 *Itcp, F64 *Volt, U16 Count);
I16 PID_AO_VoltScale(F64 *Volt, F64 *Slp, F64 *Itcp, U16 *Data, U16 Count);
I16 PID_MarkInput_IsUpdated(U16 DeviceId);
I16 PID_MarkInput_Clear(U16 DeviceId);
I16 PID_MarkInput_Wait(U16 DeviceId, U16 Timeout);
I16 PID_Update_NewData(U16 DeviceId);
I16 PID_Unit_Cmd(U16 DeviceId, U8 Cmd);
I16 PID_Sem_Init(U16 DeviceId);
I16 pid_thread_create();
I16 gpsTimer_thread_create();
I16 getSignal_NextDT(double *_global_us_t,double *_global_us_g,double DT);
I16 wait_PPS_signal();
I16 getEndTime();


#if 0
#else
struct rtnet_frame {
	unsigned char *payload;
	int len;
	unsigned int status;
};
struct enet_frame {
	uint8_t dst[6];
	uint8_t src[6];
	u16 type;
	uint8_t data[1500];	/* 46 to 1500 bytes */
};
typedef struct {
	void *ptr;
	int len;
	int bufsize;
} rtnet_mem_t;


struct _global_time{
	U16 gtime_y;
	U16 gtime_d;
	U16 gtime_h;
	U16 gtime_m;
	U16 gtime_s;
	U16 gtime_ms;
	U16 gtime_us;
};

#endif
#ifdef __cplusplus
}
#endif

#endif /* YJDNET_H */
