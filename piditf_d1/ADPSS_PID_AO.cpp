//#include "StdAfx.h"
#include <math.h>
#include "ADPSS_PID_AO.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_AO::CADPSS_PID_AO(void){
	Slp=NULL;
	Itcp=NULL;
	data=NULL;
	Volt=NULL;
	DeviceId=9999;
	NumInstant=0;
}

CADPSS_PID_AO::CADPSS_PID_AO(const CADPSS_PID_AO &a)
{
	I32 n=a.VarInf.size();
	if (n>0) 
	{
		allocBuf(n);

		memcpy(Slp,a.Slp,sizeof(F64)*n);
		memcpy(Itcp,a.Itcp,sizeof(F64)*n);
		memcpy(data,a.data,sizeof(float)*n);
		memcpy(Volt,a.Volt,sizeof(F64)*n);
		
		VarInf = a.VarInf;


	//	for (int i=0; i<n; i++) VarInf.push_back(a.VarInf[i]);
	}
	else
	{
		Slp=NULL;
		Itcp=NULL;
		data=NULL;
		Volt=NULL;
	}
	DeviceId=a.DeviceId;
	NumInstant=a.NumInstant;
	if (NumInstant>0) {
		memcpy(VinstInA,a.VinstInA,sizeof(F64)*NumInstant);
		memcpy(VinstInP,a.VinstInP,sizeof(F64)*NumInstant);
		memcpy(VinstInAL,a.VinstInAL,sizeof(F64)*NumInstant);
		memcpy(VinstInPL,a.VinstInPL,sizeof(F64)*NumInstant);
		kIT=a.kIT;
		DT=a.DT;
		f0=a.f0;
	}
}

CADPSS_PID_AO::~CADPSS_PID_AO(void){
//	if (size()>0) {
//		finalize();
//	}
}

//增加一个仿真变量，返回>=0表示成功,<0失败
//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的计算进程号，从0开始编号；
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输出，仿真系统出乘以该倍数后供给板卡输入。
I32 CADPSS_PID_AO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
						   I32 VarNo, U16 SlotNo, U16 nChanNo,  F64 KAmplify)
{
	I32 i;
	I32 nret;

	//查找变量是否已经存在
	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR &a=VarInf[i];
		if (a.idno==idno)
		{	//找到，可能是两个变量对应一个信号

			if(DEBUG)
			{
				fprintf(fdbg,"CADPSS_PID_AO::addavar: idno=%d already exist!\n",idno);
			}
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
		if(DEBUG)
		{
			fprintf(fdbg,"CADPSS_PID_AO::addavar: i:%d< VarINf.size()：%d\n",i,VarInf.size());
		}

	}
	else
	{
		//新增一个变量
		ADPSS_PID_IOVAR var;
		var.idno=idno;
		var.VSimIO.push_back(svar);

		var.Nsigntype=ADPSS_PID_IOVAR::AO;
		var.slotno=SlotNo;
		var.channelno=nChanNo;
		var.nValueType=ValueType;


		if (ADPSS_PID_IOVAR::Virt2InstVal==ValueType) {
			var.idxVal2=NumInstant;
			NumInstant++;
		}
		else
		{
			var.idxVal2=-1;
		}

		VarInf.push_back(var);
		nret=VarInf.size();
	}
	if(INFO)
	{
		fprintf(fdbg, "idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d, NumInstant=%d\n", idxio, VarNo, SlotNo, nChanNo, ValueType, NumInstant);
		fprintf(fdbg, "VarInf.size=%d\n", nret);
	}

	return nret;
}

//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
I32 CADPSS_PID_AO::allocBuf()
{
	I32 n=size();

	if (n>0) 
	{
		Slp = new F64[n];
		Itcp = new F64[n];
		data = new float[n];
		Volt = new F64[n];

		if (!Slp || !Itcp || !data || !Volt)
		{
			fprintf(fdbg, "CADPSS_PID_AO::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
I32 CADPSS_PID_AO::allocBuf(I32 nVar)
{
	I32 n = nVar;

	if (n>0) 
	{
		Slp = new F64[n];
		Itcp = new F64[n];
		data = new float[n];
		Volt = new F64[n];

		if (!Slp || !Itcp || !data || !Volt)
		{
			fprintf(fdbg, "CADPSS_PID_AO::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//释放缓冲区
I32 CADPSS_PID_AO::releaseBuf()
{
	if (Slp)
	{
		delete[] Slp;
		Slp=NULL;
	}
	if (Itcp)
	{
		delete[] Itcp;
		Itcp=NULL;
	}
	if (data)
	{
		delete[] data;
		data=NULL;
	}
	if (Volt)
	{
		delete[] Volt;
		Volt=NULL;
	}

	return 0;
}

//确定变量后，初始化数据存储区，设置接口箱分组信息
//0表示成功，非0表示失败
I32 CADPSS_PID_AO::init(U16 id)
{
	DeviceId=id;
	I32 n=size();
	if(INFO)
	fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, SigNum=%d\n", id, n);


	if (n>0)
	{

		memset(Volt, 0, n*sizeof(F64));
		if(DEBUG)
		{
			fprintf(fdbg,"CADPSS_PID_AO::init, set DeviceID to %d\n",DeviceId);
			fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, SigNum=%d\n", id, n);
		}

	}
	if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_AO::init done.......\n");
	return 0;
}



//初始化有效值转瞬时值相关变量，在有这种转换需求时调用
//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
void CADPSS_PID_AO::initInstantPara(F64 dDT,F64 df0, I32 nkIT)
{
	DT=dDT;
	f0=df0;
	kIT=nkIT;
	if (NumInstant>0)
	{
		VinstInA=new F64[NumInstant];
		VinstInP=new F64[NumInstant];
		VinstInAL=new F64[NumInstant];
		VinstInPL=new F64[NumInstant];
		memset(VinstInA,0,NumInstant*sizeof(F64));
		memset(VinstInP,0,NumInstant*sizeof(F64));
		memset(VinstInAL,0,NumInstant*sizeof(F64));
		memset(VinstInPL,0,NumInstant*sizeof(F64));
	}
}

//根据指定进程仿真输入数据设置AO数据
//返回实际设置的数据数目
I32 CADPSS_PID_AO::GetSimuVal(I32 procno,F64 *Vin)
{
	if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal start\n");
	I32 i,j,n=0;


	if(DEBUG)
	{
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal VarINf.size=%d\n",VarInf.size());
	}
	for (i=0; i<VarInf.size(); i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];

		if(DEBUG)
		{
			fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal VarInf[%d].VSimIO.size()=%d\n",i,a.VSimIO.size());
		}
		for (j=0; j<a.VSimIO.size(); j++)
		{
			ADPSS_Simu_IOVAR& b=a.VSimIO[j];
			if (b.prono==procno)	//找到
			{
				Volt[i]=Vin[b.idxIO];
				if(DEBUG)
					fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal Volt[%d]=%20.15f\n",i,Volt[i]);

				/*
				if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)	//瞬时值
				{
					//printf("CADPSS_PID_AO::GetSimuVal Virt2InstVal \n");

					if (1==j)	//虚部值
					{
						F64 r,x;
						r=Vin[a.VSimIO[0].idxIO];
						x=Vin[b.idxIO];
						I32 idx=a.idxVal2;
						VinstInAL[idx]=VinstInA[idx];
						VinstInPL[idx]=VinstInP[idx];
						VinstInA[idx]=b.kamplify*sqrt((r*r+x*x)*2);	//转换为幅值，A**0.5
						VinstInP[idx]=atan2(x,r);
					}
				}
				else
				{	//有效值，各变量数值累加
					//printf("CADPSS_PID_AO::GetSimuVal not Virt2InstVal--\n");
					if (0==j)
					{
						printf("j=0, idxIO=%d, b.kamplify=%f, Vin[%d]=%f\n",b.idxIO,b.kamplify,b.idxIO,Vin[b.idxIO]);
						Volt[i]=b.kamplify*Vin[b.idxIO];
					}
					else
					{
						Volt[i]+=b.kamplify*Vin[b.idxIO];
						//printf("j=%d, idxIO=%d, b.kamplify=%f, Vin[%d]=%f\n",j,b.idxIO,b.kamplify,b.idxIO,Vin[b.idxIO]);
					}

					//printf("CADPSS_PID_AO::GetSimuVal Volt(%d):%f\n",i,Volt[i]);
				}
				*/
				n++;
			}
		}
	}

	if(DEBUG)
	{
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal return ao size:%d\n",n);
	}

	return n;
}

//更新瞬时值数值
//nt：机电暂态仿真步数
//knt：物理接口输出步数（大步长内）
I32 CADPSS_PID_AO::updateInstantVal(I32 nt,I32 knt)
{
	I32 i, n=0;
	F64 wt, kperc;
	const F64 pi=3.1415926;

	kperc=knt/(kIT*1.0);
	wt=2*pi*f0*(nt+kperc)*DT;

	for (i=0; i<VarInf.size(); i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)	//瞬时值
		{
			I32 idx=a.idxVal2;
			Volt[i]=VinstInAL[idx]*cos(wt+VinstInPL[idx])*(1-kperc)+VinstInA[idx]*cos(wt+VinstInP[idx])*kperc;
			n++;
		}
	}
#ifdef ZX_DBG
	datalog[nt*100+knt] = Volt[0];
#endif

	return n;
}

//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
I32 CADPSS_PID_AO::SenddataToPID()
{
	I32 i, n=size();
	if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_AO::SenddataToPID: size of data:%d\n",n);

	//PID_AO_VoltScale(Volt, Slp, Itcp, data, n);

	//PID_AO_GrpWrite(DeviceId, 0, data, n);
	PID_AO_GrpWrite(DeviceId, 0, (float*) Volt, n);

	return 0;
}

I32 CADPSS_PID_AO::ClearOutput()
{
	I32 i, n=size();

	for (i=0; i<n; i++) Volt[i]=0;
	PID_AO_VoltScale(Volt, Slp, Itcp, data, n);
	PID_AO_GrpWrite(DeviceId, 0, data, n);



	return 0;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_AO::finalize(){
#ifdef ZX_DBG
	FILE* log;
	log = fopen("DATALOG.log","w");
	for (int i=0; i<10*10000; i++) fprintf(log, "%f\n", datalog[i]);
	fclose(log);
#endif

#ifdef ZX_DBG
	if (datalog) {
		delete[] datalog;
	}
#endif

	releaseBuf();

	VarInf.clear();

	if (NumInstant>0) {
		delete []VinstInA;
		delete []VinstInP;
		delete []VinstInAL;
		delete []VinstInPL;
		NumInstant=0;
	}
	return 0;
}

I32 CADPSS_PID_AO::setSyncDT(F64 DT)
{
	syncDT = DT;
	return 0;
}
