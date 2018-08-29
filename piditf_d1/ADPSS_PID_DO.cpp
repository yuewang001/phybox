//#include "StdAfx.h"
#include "ADPSS_PID_DO.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_DO::CADPSS_PID_DO(void){
	data=NULL;
	DeviceId=9999;
}

CADPSS_PID_DO::CADPSS_PID_DO(const CADPSS_PID_DO &a)
{
	I32 n=a.VarInf.size();
	if (n>0)
	{
		allocBuf(n);

		memcpy(data,a.data,sizeof(I16)*n);

		VarInf = a.VarInf;
	//	for (int i=0; i<n; i++) VarInf.push_back(a.VarInf[i]);
	}
	else
	{
		data=NULL;
	}
	DeviceId=a.DeviceId;
}

CADPSS_PID_DO::~CADPSS_PID_DO(void){
//	if (size()>0) {
//		finalize();
//	}
}

//增加一个仿真变量，返回>=0表示成功,<0失败
//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的接口进程号，从0开始编号；
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输出，仿真系统出乘以该倍数后供给板卡输入。
I32 CADPSS_PID_DO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
						   I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
	I32 i;
	I32 nret;

	//查找变量是否已经存在
	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR &a=VarInf[i];
		if (a.idno==idno)
		{	
			break;	
		}
	}

	ADPSS_Simu_IOVAR svar;
	svar.idxIO=idxio;
	svar.kamplify=KAmplify;
	svar.prono=ProcNo;
	svar.VarNo=VarNo;

	if (i<VarInf.size())
	{	//找到，在原有变量中新增一个仿真变量
		VarInf[i].VSimIO.push_back(svar);
		nret=i;
	}
	else
	{
		//新增一个变量
		ADPSS_PID_IOVAR var;
		var.idno=idno;
		var.VSimIO.push_back(svar);

		var.Nsigntype=ADPSS_PID_IOVAR::DO;
		var.slotno=SlotNo;
		var.channelno=nChanNo;
		var.nValueType=ValueType;

		if (ADPSS_PID_IOVAR::D01TIMEVAL==ValueType)
		{//预留
			//以后新增处理
		}
		else
		{
			//以后新增处理
		}

		VarInf.push_back(var);
		nret=VarInf.size();
	}

	return nret;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DO::allocBuf()
{
	I32 n=size();

	if (n>0) 
	{
		data = new U16[n];

		if (!data)
		{
			fprintf(fdbg, "CADPSS_PID_DO::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DO::allocBuf(I32 nVar)
{
	I32 n = nVar;

	if (n>0) 
	{
		data = new U16[n];

		if (!data)
		{
			fprintf(fdbg, "CADPSS_PID_DO::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//释放缓冲区
I32 CADPSS_PID_DO::releaseBuf()
{
	if (data)
	{
		delete[] data;
		data=NULL;
	}

	return 0;
}

//确定变量后，初始化数据存储区，设置接口箱分组信息
//0表示成功，非0表示失败
I32 CADPSS_PID_DO::init(U16 id)
{
	I32 n=size();

	DeviceId=id;

	if(DEBUG)
	{
		fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, SigNum=%d\n", id, n);
	}
}

//根据指定进程仿真输入数据设置DO数据
//返回实际设置的数据数目
I32 CADPSS_PID_DO::GetSimuVal(I32 procno,F64 *Vin)
{
	I32 i, n=0;
	for (i=0; i<VarInf.size(); i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		//根据data含义设置，
		//数值类型所需要的变换在这里处理
		if (a.VSimIO[0].prono==procno)
		{	//找到

			int ind=a.idno-1;
			data[ind]=((U16)Vin[a.VSimIO[0].idxIO])<<15; //需要约定与emt通信的数值格式！！！

			data[ind] += (U16)((Vin[a.VSimIO[0].idxIO]-(U16)Vin[a.VSimIO[0].idxIO])*1000000);
	
			//DO，低15位，表示在下一同步信号多少us后输出，传送给物理接口箱，最高为代表分和合，低15位代表us数

			if(DEBUG)
			{
				fprintf(fdbg,"CADPSS_PID_DO::GetSimuVal: get DO data[%d]=%d",ind,data[ind]);
			}
			n++;
		}
	}
	if(INFO)
	{
		fprintf(fdbg,"CADPSS_PID_DO::GetSimuVal: get %d data\n",n);
	}
	return n;
}

//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
I32 CADPSS_PID_DO::SenddataToPID()
{
	I32 n=size();

	PID_DO_GrpWrite(DeviceId, 0, data, n);
	
	return 0;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_DO::finalize()
{
	releaseBuf();

	VarInf.clear();

	return 0;
}


I32 CADPSS_PID_DO::setSyncDT(F64 DT)
{
	syncDT = DT;
	return 0;
}
