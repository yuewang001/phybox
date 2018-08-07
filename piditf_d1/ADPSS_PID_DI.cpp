//#include "StdAfx.h"
#include "ADPSS_PID_DI.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_DI::CADPSS_PID_DI(void){
	data=NULL;
	DeviceId=9999;
}

CADPSS_PID_DI::CADPSS_PID_DI(const CADPSS_PID_DI &a)
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

CADPSS_PID_DI::~CADPSS_PID_DI(void){
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
//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
I32 CADPSS_PID_DI::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
						   I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
	if(DEBUG)
	fprintf(fdbg,"CADPSS_PID_DI::addavar: idxio=%d,idno=%d\n",idxio,idno);
	I32 i;
	I32 nret;

	//查找变量是否已经存在
	for (i=0; i<VarInf.size(); i++)
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

		var.Nsigntype=ADPSS_PID_IOVAR::DI;
		var.slotno=SlotNo;
		var.channelno=nChanNo;
		var.nValueType=ValueType;
		//var.iospec = IoSpec;


		VarInf.push_back(var);
		nret=VarInf.size();

		if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_DI::new added value: idxio=%d,idno=%d\n",idxio,idno);
	}

	return nret;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DI::allocBuf()
{
	I32 n=size();

	if (n>0) 
	{
		data = new U16[n];

		if (!data)
		{
			fprintf(fdbg, "CADPSS_PID_DI::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DI::allocBuf(I32 nVar)
{
	I32 n = nVar;

	if (n>0) 
	{
		data = new U16[n];

		if (!data)
		{
			fprintf(fdbg, "CADPSS_PID_DI::allocBuf(), fail to allocate buffer, len = %d", n);
			return 1;
		}
	}

	return 0;
}

//释放缓冲区
I32 CADPSS_PID_DI::releaseBuf()
{
	if (data)
	{
		delete[] data;
		data=NULL;
	}

	return 0;
}

//确定变量后，初始化数据存储区
//0表示成功，非0表示失败
I32 CADPSS_PID_DI::init(U16 id)
{
	I32 i, n=size();

	DeviceId=id;
	if(DEBUG)
		fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, SigNum=%d\n", id, n);
	return 0;
	
}

//从接口箱取得数据，存储在data。
I32 CADPSS_PID_DI::getdatafromPID()
{
	I32 n=size();

	PID_DI_GrpRead(DeviceId, 0, data, n);

	return 0;
}

//根据DI数据设置指定进程仿真输入数据
//返回实际设置的数据数目
I32 CADPSS_PID_DI::setSimuVal(I32 procno, F64 *Vin)
{
	I32 i, n=0;

	if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_DI::setSimuVal:size VarInf: %d",VarInf.size());

	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		//根据data含义设置Vin，
		//数值类型所需要的变换在这里处理
		if (a.VSimIO[0].prono==procno)
		{	//找到
			//Vin[a.VSimIO[0].idxIO]=a.VSimIO[0].*Volt[i];
			/*
			Vin[a.VSimIO[0].idxIO]=a.VSimIO[0];
			if(DEBUG)
			{
				//printf("CADPSS_PID_DI::setSimuVal: Volt[%d]=%d---\n",i,Volt[i]);
				//printf("a.VSimIO[0].idxIO=%d  Vin[%d]=%d---\n",a.VSimIO[0].idxIO,a.VSimIO[0].idxIO,Vin[a.VSimIO[0].idxIO]);
				printf("CADPSS_PID_DI::setSimuVal: a.VSimIO[0]=%d---\n",a.VSimIO[0]);
			}
			n++;
			*/
			//Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1);
			//Vin[a.VSimIO[0].idxIO]=a.VSimIO[0].kamplify*Volt[i];
			//Vin[a.VSimIO[0].idxIO]=a.VSimIO[0];
			//Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1);
			Vin[a.VSimIO[0].idxIO]=data[i];
			if(DEBUG)
			{
				fprintf(fdbg,"CADPSS_PID_DI::setSimuVal:data[%d]=%f\n",i,data[i]);
				fprintf(fdbg,"CADPSS_PID_DI::setSimuVal: Vin[%d]=%f\n",a.VSimIO[0].idxIO,Vin[a.VSimIO[0].idxIO]);
			}

/*
            Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1); //需要修改
/*DI时标功能*/
			/*
			if (a.iospec == 1)				// 正脉冲
			{
				if ((data[i]>>15)&0x1)           //导通
				{
					Vin[a.VSimIO[0].idxIO] = 1.0 + 0.000001*(data[i]&0x7F)/syncDT;
				}
				else
				{
					Vin[a.VSimIO[0].idxIO] = 0.0;
				}
			}
			else if (a.iospec == 2)			// 负脉冲
			{
				if (!((data[i]>>15)&0x1))       //导通
				{
					Vin[a.VSimIO[0].idxIO] = 1.0 + 0.000001*(data[i]&0x7F)/syncDT;
				}
				else
				{
					Vin[a.VSimIO[0].idxIO] = 0.0;
				}
			}
			else							// 电平
			{
				if ((data[i]>>15)&0x1)           //导通
				{
					Vin[a.VSimIO[0].idxIO] = 1.0 + 0.000001*(data[i]&0x7F)/syncDT;
			//		printf("i=%d %02x %f ",i,data[i],Vin[a.VSimIO[0].idxIO]);
				}
				else
				{
					Vin[a.VSimIO[0].idxIO] = 0.0 + 0.000001*(data[i]&0x7F)/syncDT;
				}
			}

//*/
			n++;
		}
	}
	if(DEBUG)
	fprintf(fdbg,"CADPSS_PID_DI::setSimuVal: n=%d\n",n);
	return n;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_DI::finalize()
{
	releaseBuf();

	VarInf.clear();
	return 0;
}

I32 CADPSS_PID_DI::setSyncDT(F64 DT)
{
	syncDT = DT;
	return 0;
}
