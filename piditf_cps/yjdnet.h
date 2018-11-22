#ifndef YJDNET_H
#define YJDNET_H

#ifdef __cplusplus
extern   "C"   {
#endif

//#include <linux/module.h>
//#include <stdio.h>
//#include <rt_unistd.h>
#include <rt_stdlib.h>
#include <rt_fcntl.h>
#include <rt_semaphore.h>
#include <sys/rt_ioctl.h>
#include <rtnet.h>

#if YJDNET_DEBUG
#  define yjdnet_debug(args...) rt_printf(args)
#else
#  define yjdnet_debug(args...)
#endif

#undef rdtscll
#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A"(val))

#define MAX_DEVICES	4
#define MAX_SLOTS	8
#define MAX_AI_CHANS	8
#define MAX_AO_CHANS	8
#define MAX_DI_CHANS	16
#define MAX_DO_CHANS	16
#define MAX_GRP_CFGS	4

typedef unsigned char U8;
typedef short I16;
typedef unsigned short U16;
typedef long I32;
typedef unsigned long U32;
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
	ErrorDIGrpSlot = -7,
	ErrorDIGrpConflict = -8,
	ErrorDOGrpSlot = -9,
	ErrorDOGrpConflict = -10,
	ErrorAIGrpSlot = -11,
	ErrorAIGrpConflict = -12,
	ErrorAOGrpSlot = -13,
	ErrorAOGrpConflict = -14,
	ErrorDIGrpRead = -15,
	ErrorDOGrpWrite = -16,
	ErrorAIGrpRead = -17,
	ErrorAOGrpWrite = -18,
	ErrorAIVoltScale = -19,
	ErrorAOVoltScale = -20,
	ErrorMarkInputClean = -21,
	ErrorMarkInputWaitTimeout = -22,
	ErrorMarkInputWait = -23,
	ErrorCmd = -24,

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
I16 PID_Slot_WriteConfig(U16 DeviceId, U8 SlotId, U16 CardConfig);
I16 PID_DI_GrpConfig(U16 DeviceId, U16 GrpId, U8 *GrpConfig, U16 Count);
I16 PID_DO_GrpConfig(U16 DeviceId, U16 GrpId, U8 *GrpConfig, U16 Count);
I16 PID_AI_GrpConfig(U16 DeviceId, U16 GrpId, U8 *GrpConfig, U16 Count);
I16 PID_AO_GrpConfig(U16 DeviceId, U16 GrpId, U8 *GrpConfig, U16 Count);
I16 PID_DI_GrpRead(U16 DeviceId, U16 GrpId, U8 *Data, U16 Count);
I16 PID_DO_GrpWrite(U16 DeviceId, U16 GrpId, U8 *Data, U16 Count);
I16 PID_AI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_AO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count);
I16 PID_AI_VoltScale(U16 *Data, F64 *Slp, F64 *Itcp, F64 *Volt, U16 Count);
I16 PID_AO_VoltScale(F64 *Volt, F64 *Slp, F64 *Itcp, U16 *Data, U16 Count);
I16 PID_MarkInput_IsUpdated(U16 DeviceId);
I16 PID_MarkInput_Clear(U16 DeviceId);
I16 PID_MarkInput_Wait(U16 DeviceId, U16 Timeout);
I16 PID_Unit_Cmd(U16 DeviceId, U8 Cmd);
I16 PID_Sem_Init(U16 DeviceId);
#ifdef __cplusplus
}
#endif

#endif /* YJDNET_H */
