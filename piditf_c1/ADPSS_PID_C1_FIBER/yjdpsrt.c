#include "yjdnet.h"
#include "yjdioctl.h"

static int fd = -1;
static int count;

I16 PID_Register(U16 DeviceId, U8 Mode)
{
	pid_ioctl_t p;
	
	//if (count)
		//return -1;
	//	fd = rt_open("/dev/yjdnet", RT_O_RDWR);
	//if (fd < 0) return -1;

	count++;
	p.DeviceId = DeviceId;
	p.Mode = Mode;
	return rt_ioctl(fd, PID_REGISTER, &p);
}

I16 PID_Release(U16 DeviceId)
{
	pid_ioctl_t p;
	int ret;

//	if (fd < 0) return 0;

	p.DeviceId = DeviceId;
	ret = rt_ioctl(fd, PID_RELEASE, &p);
	count--;
//	if (count <= 0)
//		rt_close(fd);
	return ret;
}

I16 PID_Sync_ReadConfig(U16 DeviceId, U32 *SyncFreq, U16 *SM)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SyncFreq_rd = SyncFreq;
	p.SM_rd = SM;
	return rt_ioctl(fd, PID_SYNC_READCONFIG, &p);
}

I16 PID_Sync_WriteConfig(U16 DeviceId, U32 SyncFreq, U16 SM)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SyncFreq_wr = SyncFreq;
	p.SM_wr = SM;
	return rt_ioctl(fd, PID_SYNC_WRITECONFIG, &p);
}

I16 PID_Slot_ReadInfo(U16 DeviceId, U8 SlotId, U16 *CardInfo, F64 *Slp, F64 *Itcp)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SlotId = SlotId;
	p.CardInfo = CardInfo;
	p.Slp = Slp;
	p.Itcp = Itcp;
	return rt_ioctl(fd, PID_SLOT_READINFO, &p);
}

I16 PID_Slot_ReadConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SlotId = SlotId;
	p.CardConfig_rd = CardConfig;
	return rt_ioctl(fd, PID_SLOT_READCONFIG, &p);
}

I16 PID_Slot_WriteConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SlotId = SlotId;
	p.CardConfig_wr = CardConfig;
	return rt_ioctl(fd, PID_SLOT_WRITECONFIG, &p);
}
	
I16 PID_Slot_ReadPara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SlotId = SlotId;
	p.Slp = Slp;
	p.Itcp = Itcp;
	return rt_ioctl(fd, PID_SLOT_READPARA, &p);
}

I16 PID_Slot_WritePara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.SlotId = SlotId;
	p.Slp = Slp;
	p.Itcp = Itcp;
	return rt_ioctl(fd, PID_SLOT_WRITEPARA, &p);
}

I16 PID_DI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.GrpConfig = GrpConfig;
	p.Count = Count;
	return rt_ioctl(fd, PID_DI_GRPCONFIG, &p);
}

I16 PID_DO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.GrpConfig = GrpConfig;
	p.Count = Count;
	return rt_ioctl(fd, PID_DO_GRPCONFIG, &p);
}

I16 PID_AI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.GrpConfig = GrpConfig;
	p.Count = Count;
	return rt_ioctl(fd, PID_AI_GRPCONFIG, &p);
}

I16 PID_AO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.GrpConfig = GrpConfig;
	p.Count = Count;
	return rt_ioctl(fd, PID_AO_GRPCONFIG, &p);
}

I16 PID_DI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.Data_dig = Data;
	p.Count = Count;
	return rt_ioctl(fd, PID_DI_GRPREAD, &p);
}

I16 PID_DO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.Data_dig = Data;
	p.Count = Count;
	return rt_ioctl(fd, PID_DO_GRPWRITE, &p);
}

I16 PID_AI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.Data_ana = Data;
	p.Count = Count;
	return rt_ioctl(fd, PID_AI_GRPREAD, &p);
}

I16 PID_AO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.GrpId = GrpId;
	p.Data_ana = Data;
	p.Count = Count;
	return rt_ioctl(fd, PID_AO_GRPWRITE, &p);
}

I16 PID_AI_VoltScale(U16 *Data, F64 *Slp, F64 *Itcp, F64 *Volt, U16 Count)
{
	pid_ioctl_t p;

	p.Data_ana = Data;
	p.Slp = Slp;
	p.Itcp = Itcp;
	p.Volt = Volt;
	p.Count = Count;
	return rt_ioctl(fd, PID_AI_VOLTSCALE, &p);
}

I16 PID_AO_VoltScale(F64 *Volt, F64 *Slp, F64 *Itcp, U16 *Data, U16 Count)
{
	pid_ioctl_t p;

	p.Volt = Volt;
	p.Slp = Slp;
	p.Itcp = Itcp;
	p.Data_ana = Data;
	p.Count = Count;
	return rt_ioctl(fd, PID_AO_VOLTSCALE, &p);
}

I16 PID_MarkInput_Wait(U16 DeviceId, U16 Timeout)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.Timeout = Timeout;
	return rt_ioctl(fd, PID_MARKINPUT_WAIT, &p);
}


I16 PID_Update_NewData(U16 DeviceId)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	return rt_ioctl(fd, PID_UPDATE_NEWDATA, &p);
}


I16 PID_MarkInput_Clear(U16 DeviceId)
{
        pid_ioctl_t p;
        p.DeviceId = DeviceId;
        return rt_ioctl(fd, PID_MARKINPUT_CLEAR, &p);
}


I16 PID_Unit_Cmd(U16 DeviceId, U8 Cmd)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	p.Cmd = Cmd;
	return rt_ioctl(fd, PID_UNIT_CMD, &p);
}

I16 PID_Sem_Init(U16 DeviceId)
{
	pid_ioctl_t p;

	p.DeviceId = DeviceId;
	return rt_ioctl(fd, PID_SEM_INIT, &p);
}

