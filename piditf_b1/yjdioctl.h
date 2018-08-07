#ifndef YJDIOCTL_H
#define YJDIOCTL_H

enum {
	PID_REGISTER,
	PID_RELEASE,
	PID_SYNC_READCONFIG,
	PID_SYNC_WRITECONFIG,
	PID_SLOT_READINFO,
	PID_SLOT_READCONFIG,
	PID_SLOT_WRITECONFIG,
	PID_DI_GRPCONFIG,
	PID_DO_GRPCONFIG,
	PID_AI_GRPCONFIG,
	PID_AO_GRPCONFIG,
	PID_DI_GRPREAD,
	PID_DO_GRPWRITE,
	PID_AI_GRPREAD,
	PID_AO_GRPWRITE,
	PID_AI_VOLTSCALE,
	PID_AO_VOLTSCALE,
	PID_MARKINPUT_WAIT,
	PID_MARKINPUT_CLEAR,
	PID_UNIT_CMD,
	PID_SEM_INIT
};

typedef struct pid_ioctl_t {
	U16 DeviceId;
	U32 *SyncFreq_rd;
	U16 *SM_rd;
	U32 SyncFreq_wr;
	U16 SM_wr;
	U8  SlotId;
	U16 *CardInfo;
	F64 *Slp;
	F64 *Itcp;
	U16 *CardConfig_rd;
	U16 CardConfig_wr;
	U16 GrpId;
	U8  *GrpConfig;
	U16 Count;
	U8  *Data_dig;
	U16 *Data_ana;	
	F64 *Volt;
	U16 Timeout;
	U8  Cmd;
	U8  Mode;
} pid_ioctl_t;

#endif /* YJDIOCTL_H */