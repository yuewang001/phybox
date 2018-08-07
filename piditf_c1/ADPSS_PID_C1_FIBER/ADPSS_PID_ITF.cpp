//#include "StdAfx.h"
#include "ADPSS_PID_ITF.h"
#include "rtftoc_intel.h"
#include "yjdnet.h"

CADPSS_PID_ITF::CADPSS_PID_ITF(void)
{
	MasterNo=-1;

	for (int i=0; i<MAX_PID_NUM_PER_PROC; i++) pid[i] = NULL;
}

CADPSS_PID_ITF::~CADPSS_PID_ITF(void)
{
	for (int i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i]) delete pid[i];
	}
//	finalize();
}

//增加一个仿真变量，返回>=0表示成功,<0失败
//Idxio:IO索引，当前变量下标位置距离在本计算进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的计算进程号，从0开始编号；
//signType：信号类型。AI=0,AO,DI,DO
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//nPIDNo：接口箱号，从0开始编号。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
I32 CADPSS_PID_ITF::addavar(I32 idxio, I32 idno, I32 ProcNo, I32 signType, I32 ValueType, 
							I32 VarNo, U16 nPIDNo, U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec)
{
	I32 i;

	if (!pid[nPIDNo])		// 尚未使用，创建对象
	{
		pid[nPIDNo] = new CADPSS_PID(nPIDNo);
	}

	return pid[nPIDNo]->addavar(idxio,idno,ProcNo,signType,ValueType,VarNo,
		SlotNo,nChanNo,KAmplify,IoSpec);
}

I32 CADPSS_PID_ITF::allocBuf()
{
	for (int i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])		// 有效pid
			pid[i]->allocBuf();
	}

	return 0;
}

I32 CADPSS_PID_ITF::pushPID()
{
	for (int i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
	//	if (pid[i])		// 有效pid
	//		VPID.push_back(*pid[i]);
	}

	return 0;
}


//确定变量后，初始化数据存储区
//0表示成功，非0表示失败
extern int gs_dev_no[MAX_DEVICES];
I32 CADPSS_PID_ITF::init(F64 SyncDT)
{
	I32 i, nret=-1;

	nret = pid_thread_create();

	if(nret<0) return -1;

	nret = gpsTimer_thread_create();

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	
		{
			gs_dev_no[i]=i;

			if (0==pid[i]->init(SyncDT))
			{
				nret=0;
			}
		}
	}

	return  nret;
}

//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
void CADPSS_PID_ITF::initInstantPara(F64 dDT,F64 df0, I32 nkIT)
{
	I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			pid[i]->initInstantPara(dDT,df0,nkIT);}
	}
}

//从接口箱取得数据，存储在data、Volt中。
I32 CADPSS_PID_ITF::getdatafromPID()
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			nret+=pid[i]->getdatafromPID();}
	}

	return nret;
}

//根据AI数据设置指定进程仿真输入数据
//返回实际设置的数据数目
I32 CADPSS_PID_ITF::setSimuVal(I32 procno,F64 *Vin)
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			nret+=pid[i]->setSimuVal(procno,Vin);}
	}

	return nret;
}

//根据指定进程仿真输入数据设置AO数据
//返回实际设置的数据数目
I32 CADPSS_PID_ITF::GetSimuVal(I32 procno,F64 *Vin)
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			nret+=pid[i]->GetSimuVal(procno,Vin);}
	}

	return nret;
}

//更新瞬时值数值
I32 CADPSS_PID_ITF::updateInstantVal(I32 nt,I32 kit)
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			nret+=pid[i]->updateInstantVal(nt,kit);}
	}

	return nret;
}

//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
I32 CADPSS_PID_ITF::SenddataToPID()
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			nret+=pid[i]->SenddataToPID(i);}
	}

	return nret;
}

I32 CADPSS_PID_ITF::ClearOutput()
{
        I32 i,nret=0;

		for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
        {
			if (pid[i])	{               
				nret+=pid[i]->ClearOutput();}
        }

        return nret;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_ITF::finalize()
{
	I32 i,nret=-1;
/*
	if (MasterNo<0)
	{
		return -1;
	}
*/
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			if (0==pid[i]->finalize()) nret=0;}
	}
	MasterNo=-1;

	return nret;
}

//取得输入变量数目
I32 CADPSS_PID_ITF::getNumIn()
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			nret+=pid[i]->getNumIn();}
	}

	return nret;
}

//取得输出变量数目
I32 CADPSS_PID_ITF::getNumOut()
{
	I32 i,nret=0;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			nret+=pid[i]->getNumOut();}
	}

	return nret;
}

//是否包含瞬时值信息，初始化后有效
bool CADPSS_PID_ITF::ishaveInstantInput()
{
	I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			if (pid[i]->ishaveInstantInput()) return TRUE;}
	}

	return FALSE;
}

//MarkInput相关函数
//查询是否更新
I16 CADPSS_PID_ITF::MarkInput_IsUpdated()
{
	I16 nret=1;
	I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			if (!pid[i]->MarkInput_IsUpdated()) nret=0;}
	}

	return nret;
}

//清除MarkInput标记
I16 CADPSS_PID_ITF::MarkInput_Clear()
{
	I16 nret=0;
	I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
			if (!pid[i]->MarkInput_Clear()) nret=1;}
	}

	return nret;
}

//等待MarkInput变化后返回
I16 CADPSS_PID_ITF::MarkInput_Wait(U16 Timeout)
{
	I16 nret=0, cnt=0, err=0;
	I32 i;
	int SleepTime;

    // 等待每一个PID
    // 对于信号量方式，MarkInput_Wait等待成功时返回0
    // 对于查询方式，MarkInput_Wait返回底层驱动数据更新次数，正常为1
#if PID_SEM_MODE
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
		if (pid[i])	{         
		nret = pid[i]->MarkInput_Wait(Timeout);
		if (!nret) cnt++;}
    }
#else
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{  
		while (1)
		{
			nret = pid[i]->MarkInput_Wait(Timeout);

			if (nret>0)
			{
				cnt+=nret;
				break;
			}
			else
			{
				SleepTime = 5;
        		//	fortran_clock_sleep_(&SleepTime);

			}
		}}
    }
#endif

    return cnt;
}

//停止同步信号
I16 CADPSS_PID_ITF::stopSync()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
        if (pid[i])	{  
			pid[i]->stopSync();}
    }

    return nret;
}

//启动从站仿真
I16 CADPSS_PID_ITF::startSlaveSim()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
        if (pid[i])	{  
		if (i!=MasterNo)    //从站
        {
            if (!pid[i]->startSim()) nret=1;
		}}
    }

    return nret;
}

//启动主控仿真，启动主控应该启动所有从站之后
I16 CADPSS_PID_ITF::startMasterSim()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
		if (pid[i])	{         
		if (i==MasterNo)    //主控
        {
            if (!pid[i]->startSim()) nret=1;
		}}
    }

    return nret;
}

//停止仿真
I16 CADPSS_PID_ITF::stopSim()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
        if (pid[i])	{  
			if (!pid[i]->stopSim()) nret=1;}
    }

    return nret;
}

//搜索主控接口箱，即最小序号接口箱，并设置其主控标记
//返回最小序号接口箱在VPID数组中的序号
I16 CADPSS_PID_ITF::setMaster()
{
    I16 nret=0;
    I32 i, minPIDNo;

    //搜索最小序号接口箱
	//minPIDNo = pid[0]->getPIDNo();
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if(pid[i])
		{
			minPIDNo=pid[i]->getPIDNo();
			break;
		}
	}
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
        if (pid[i])	{  
			if (minPIDNo > pid[i]->getPIDNo()) minPIDNo=pid[i]->getPIDNo();}
    }

    //查询最小序号接口箱在VPID数组中的序号
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
		if (pid[i])	{         
		if (minPIDNo == pid[i]->getPIDNo())
        {
            MasterNo=i;
            break;
		}}
    }

    //设置接口箱主控标记
    pid[MasterNo]->setMaster();

    return nret;
}
