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
						   I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec)
{
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
		var.iospec = IoSpec;

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
	/*
	I32 i, n=size();
	U16 slot_info[MAX_SLOTS][7], slot_flag[MAX_SLOTS], slot_iospec[MAX_SLOTS];
	U16* grp_cfg;
	U8 ver1,ver2;
	U16 ver3;
	U16 config[3];

	if (n>0)
	{
		grp_cfg = new U16[n];
		DeviceId=id;
		fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, SigNum=%d\n", id, n);

		// 逐槽读取板卡信息
		for (i=0; i<MAX_SLOTS; i++)
		{
			PID_Slot_ReadInfo(DeviceId, i, &slot_info[i][0], NULL, NULL);
		}

		
		for (i = 0; i < MAX_SLOTS; i++) 
		{
			ver1 = (U8)(slot_info[i][0] >> 12);
			ver2 = (U8)(slot_info[i][0] >> 8) & 0x0F;
			ver3 = (U8)(slot_info[i][0]);
			fprintf(fdbg,"%d - %s  v%d.%d.%d\n", i, gs_card_str[slot_info[i][1]], ver1, ver2, ver3);
		}

		// 逐个通道分析槽位是否有误，无误则进行分组配置，并记录校准数据
		memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
		for (i=0; i<n; i++)
		{
			ADPSS_PID_IOVAR &var=VarInf[i];
			if (slot_info[var.slotno][1] == CARD_INFO_DI)			// 该信号对应槽位的板卡为DI卡
			{
				grp_cfg[i] = var.slotno<<8 | var.channelno;		// 分组配置

				slot_flag[var.slotno] = 1;
				slot_iospec[var.slotno] = var.iospec;			// 以该板卡最后一个信号的iospec为准
			}
			else
			{
				delete[] grp_cfg;

				fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
				return -2;
			}
		}

		//*(U32 *)&config[1] = 50;
		
		*(U32 *)&config[1] =(U32)(syncDT*1000000);

		//配置运行模式PID_Slot_WriteConfig
		for (i=0; i<MAX_SLOTS; i++)
		{
			if (slot_flag[i] == 1)		// 该槽位为DI卡，且有通道被调用
			{
				//config[0] = CARD_CFG_DI_SIM_ELEC;
				//PID_Slot_WriteConfig(DeviceId, i, config);
				if(slot_iospec[i] == 1)				// 正脉冲模式
				{
					config[0] = 0x0f00 | CARD_CFG_DI_SIM_PULSE_UP;
					PID_Slot_WriteConfig(DeviceId, i, config);
					fprintf(fdbg, "CADPSS_PID_DI::init(), Slot %d, IoSpec::PULSE_UP\n", i);
				}
				else if (slot_iospec[i] == 2)		// 负脉冲模式
				{
					config[0] = 0x0f00 | CARD_CFG_DI_SIM_PULSE_DOWN;
					PID_Slot_WriteConfig(DeviceId, i, config);
					fprintf(fdbg, "CADPSS_PID_DI::init(), Slot %d, IoSpec::PULSE_DOWN\n", i);
				}
				else								// 默认为电平模式
				{
					config[0] = 0x0f00 | CARD_CFG_DI_SIM_ELEC;
					PID_Slot_WriteConfig(DeviceId, i, config);
					fprintf(fdbg, "CADPSS_PID_DI::init(), Slot %d, IoSpec::SIM_ELEC\n", i);
				}
			}
		}
	
		for (i=0; i<n; i++)
		{
                        fprintf(fdbg, "grp_cfg=0x%02x\n", grp_cfg[i]);
		}	
		//进行DI编组配置PID_DI_GrpConfig
		PID_DI_GrpConfig(DeviceId, 0, grp_cfg, n);

		delete[] grp_cfg;
		fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, return 0\n", id);
		return 0;
	}
	else
	{
		fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, return -1\n", id);
		return -1;
	}
	*/
	fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, return 0\n", id);
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

	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		//根据data含义设置Vin，
		//数值类型所需要的变换在这里处理
		if (a.VSimIO[0].prono==procno)
		{	//找到
/*
            Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1); //需要修改
/*DI时标功能*/
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
