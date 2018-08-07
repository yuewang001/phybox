//#include "StdAfx.h"
#include "ADPSS_PID.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID::CADPSS_PID(U16 nPIDNo){
	PIDNo=nPIDNo;
	isMaster=FALSE;
}

CADPSS_PID::CADPSS_PID(const CADPSS_PID &a):vai(a.vai),vao(a.vao),vdi(a.vdi),vdo(a.vdo)
{
	PIDNo=a.PIDNo;
	isMaster=a.isMaster;
}

CADPSS_PID::~CADPSS_PID(void){
//	finalize();
}

//增加一个仿真变量，返回>=0表示成功,<0失败
//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的计算进程号，从0开始编号；
//signType：信号类型。AI=0,AO,DI,DO
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
I32 CADPSS_PID::addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
						I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec)
{
	I32 nret;

	fprintf(fdbg, "CADPSS_PID::addavar(), idxio=%d, signType=%d, varno=%d, slotno=%d\n", idxio, signType, VarNo, SlotNo);

	switch(signType) {
		case ADPSS_PID_IOVAR::AI:
			nret=vai.addavar(idxio,idno,ProcNo,ValueType,VarNo,SlotNo,nChanNo,KAmplify);
			break;
		case ADPSS_PID_IOVAR::AO:
			nret=vao.addavar(idxio,idno,ProcNo,ValueType,VarNo,SlotNo,nChanNo,KAmplify);
			break;
		case ADPSS_PID_IOVAR::DI:
			nret=vdi.addavar(idxio,idno,ProcNo,ValueType,VarNo,SlotNo,nChanNo,KAmplify,IoSpec);
			break;
		case ADPSS_PID_IOVAR::DO:
			nret=vdo.addavar(idxio,idno,ProcNo,ValueType,VarNo,SlotNo,nChanNo,KAmplify);
			break;
	default:
		nret=-1;
		break;
	}

	return nret;
}

//在确定变量数目后，分配缓冲区空间
I32 CADPSS_PID::allocBuf()
{
	vai.allocBuf();
    vao.allocBuf();
	vdi.allocBuf();
	vdo.allocBuf();

	return 0;
}

//确定变量后，初始化数据存储区
//0表示成功，非0表示失败
I32 CADPSS_PID::init(F64 SyncDT)
{
	I32 nret;
	U32 f0;
	U16 Ism;

	I16 ret_test=0;

#if PID_SEM_MODE
	PID_Register(PIDNo, MODE_SEM);				//初始化
#else
    PID_Register(PIDNo, MODE_QRY);				//初始化
#endif
	PID_Unit_Cmd(PIDNo, CMD_STOP_SIM);			//发送仿真停止命令
	
	ret_test = PID_Sync_ReadConfig(PIDNo, &f0, &Ism);		//读同步信息
//	printf("ret_test =%d \n",ret_test);

	syncDT = SyncDT;
	syncFreq = (U32)(SyncDT*1000000);				//新物理接口注册同步信号频率传送参数单位为us
	ret_test = PID_Sync_WriteConfig(PIDNo, syncFreq, SM_SLAVE);

//	printf("ret_test =%d syncDT=%f syncFreq=%d \n",ret_test,syncDT,syncFreq);


	vao.setSyncDT(syncDT);
	vai.setSyncDT(syncDT);
	vdo.setSyncDT(syncDT);
	vdi.setSyncDT(syncDT);

	//板卡初始化
	nret=-1;
	if (0==vai.init(PIDNo)) nret=0;
	if (0==vao.init(PIDNo)) nret=0;
	if (0==vdi.init(PIDNo)) nret=0;
	if (0==vdo.init(PIDNo)) nret=0;

//	vdi.setSyncDT(syncDT);

	fprintf(fdbg, "CADPSS_PID::init(), PID=%d, Master=%d, SyncFreq=%d, nret=%d\n", PIDNo, Ism, syncFreq, nret);
	return nret;
}

//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
void CADPSS_PID::initInstantPara(F64 dDT, F64 df0, I32 nkIT)
{
	vao.initInstantPara(dDT,df0,nkIT);
}

//从接口箱取得数据，存储在data、Volt中。
I32 CADPSS_PID::getdatafromPID()
{
	I32 nret=0;

	if (vai.size()>0)
	{
		nret+=vai.getdatafromPID();
	}
	if (vdi.size()>0)
	{
		nret+=vdi.getdatafromPID();
	}

	return nret;
}

//根据AI数据设置指定进程仿真输入数据
//返回实际设置的数据数目
I32 CADPSS_PID::setSimuVal(I32 procno,F64 *Vin)
{
	I32 nret=0;
	
	if (vai.size()>0)
	{
		nret+=vai.setSimuVal(procno,Vin);
	}
	if (vdi.size()>0)
	{
		nret+=vdi.setSimuVal(procno,Vin);
	}
	
	return nret;
}

//根据指定进程仿真输入数据设置AO数据
//返回实际设置的数据数目
I32 CADPSS_PID::GetSimuVal(I32 procno,F64 *Vin)
{
	I32 nret=0;
	
	if (vao.size()>0)
	{
		nret+=vao.GetSimuVal(procno,Vin);
	}
	if (vdo.size()>0)
	{
		nret+=vdo.GetSimuVal(procno,Vin);
	}
	
	return nret;
}

//更新瞬时值数值
I32 CADPSS_PID::updateInstantVal(I32 nt,I32 kit){
	return vao.updateInstantVal(nt,kit);
}

//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
I32 CADPSS_PID::SenddataToPID(U16 DeviceId)
{
	I32 nret=0;
	if (vao.size()>0)
	{
		nret+=vao.SenddataToPID();
	}
	if (vdo.size()>0)
	{
		nret+=vdo.SenddataToPID();
	}

	PID_Update_NewData(DeviceId);

	return nret;
}

I32 CADPSS_PID::ClearOutput()
{
	I32 nret=0;
	if (vao.size()>0)
	{
		nret+=vao.ClearOutput();
	}
	return nret;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID::finalize()
{
	I32 nret;

	if (PIDNo<0) return -1;

	//板卡释放
	nret=-1;
	if (0==vai.finalize()) nret=0;
	if (0==vao.finalize()) nret=0;
	if (0==vdi.finalize()) nret=0;
	if (0==vdo.finalize()) nret=0;
	
	//接口箱释放
	PID_Release(PIDNo);
	fprintf(fdbg, "CADPSS_PID::finalize(), PID=%d, nret=%d\n", PIDNo, nret);

	PIDNo=-1;
	return nret;
}

//取得物理接口箱输入变量数目
I32 CADPSS_PID::getNumIn()
{
	I32 n=0;
	n+=vao.size();
	n+=vdo.size();
	return n;
}

//取得物理接口箱输出变量数目
I32 CADPSS_PID::getNumOut()
{
	I32 n=0;
	n+=vai.size();
	n+=vdi.size();
	return n;
}

//是否包含瞬时值信息，初始化后有效
bool CADPSS_PID::ishaveInstantInput()
{
	return vao.ishaveInstantInput();
}

//查询是否更新
I16 CADPSS_PID::MarkInput_IsUpdated()
{
//	return PID_MarkInput_IsUpdated(PIDNo);
}

//清除MarkInput标记
I16 CADPSS_PID::MarkInput_Clear()
{
	return PID_MarkInput_Clear(PIDNo);
}

//等待MarkInput变化后返回
I16 CADPSS_PID::MarkInput_Wait(U16 Timeout)
{
	return PID_MarkInput_Wait(PIDNo,Timeout);
}

//停止同步信号
I16 CADPSS_PID::stopSync()
{
    return PID_Sync_WriteConfig(PIDNo, syncFreq, SM_SLAVE);
}

//启动仿真
I16 CADPSS_PID::startSim()
{
    
	/*f发送启动仿真命令，一定要在整秒t过以后再发送，
	确保所有装置收到该命令在整秒t内，便于确保下一秒沿推送数据*/
	if (getisMaster())
    {
        PID_Sync_WriteConfig(PIDNo, syncFreq, SM_MASTER);
    }
    else
    {
        PID_Sync_WriteConfig(PIDNo, syncFreq, SM_SLAVE);
    }

	PID_MarkInput_Clear(PIDNo);

	return PID_Unit_Cmd(PIDNo, CMD_START_SIM);
}

//停止仿真
I16 CADPSS_PID::stopSim()
{
	I16 nret, cnt=10;
	do
	{
		nret = PID_Unit_Cmd((U16)PIDNo, CMD_STOP_SIM);
		cnt--;
	} while(nret<0 && cnt>0);
	fprintf(fdbg, "CADPSS_PID::stopSim(), PID=%d, nret=%d, cnt=%d\n", PIDNo, nret, cnt);
	return nret;
}
