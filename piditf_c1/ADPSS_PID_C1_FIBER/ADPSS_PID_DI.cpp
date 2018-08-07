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

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵĽӿڽ��̺ţ���0��ʼ��ţ�
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
I32 CADPSS_PID_DI::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
						   I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec)
{
	I32 i;
	I32 nret;

	//���ұ����Ƿ��Ѿ�����
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
	{	//�ҵ�����ԭ�б���������һ���������
		VarInf[i].VSimIO.push_back(svar);
		nret=i;
	}
	else
	{
		//����һ������
		ADPSS_PID_IOVAR var;
		var.idno=idno;
		var.VSimIO.push_back(svar);

		var.Nsigntype=ADPSS_PID_IOVAR::DI;
		var.slotno=SlotNo;
		var.channelno=nChanNo;
		var.nValueType=ValueType;
		var.iospec = IoSpec;

		if (ADPSS_PID_IOVAR::D01TIMEVAL==ValueType)
		{//Ԥ��
			//�Ժ���������
		}
		else
		{
			//�Ժ���������
		}
		VarInf.push_back(var);
		nret=VarInf.size();
	}

	return nret;
}

//ȷ��������Ŀ�󣬷��仺�������data
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

//ȷ��������Ŀ�󣬷��仺�������data
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

//�ͷŻ�����
I32 CADPSS_PID_DI::releaseBuf()
{
	if (data)
	{
		delete[] data;
		data=NULL;
	}

	return 0;
}

//ȷ�������󣬳�ʼ�����ݴ洢��
//0��ʾ�ɹ�����0��ʾʧ��
I32 CADPSS_PID_DI::init(U16 id)
{
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

		// ��۶�ȡ�忨��Ϣ
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

		// ���ͨ��������λ�Ƿ�������������з������ã�����¼У׼����
		memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
		for (i=0; i<n; i++)
		{
			ADPSS_PID_IOVAR &var=VarInf[i];
			if (slot_info[var.slotno][1] == CARD_INFO_DI)			// ���źŶ�Ӧ��λ�İ忨ΪDI��
			{
				grp_cfg[i] = var.slotno<<8 | var.channelno;		// ��������

				slot_flag[var.slotno] = 1;
				slot_iospec[var.slotno] = var.iospec;			// �Ըð忨���һ���źŵ�iospecΪ׼
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

		//��������ģʽPID_Slot_WriteConfig
		for (i=0; i<MAX_SLOTS; i++)
		{
			if (slot_flag[i] == 1)		// �ò�λΪDI��������ͨ��������
			{
				//config[0] = CARD_CFG_DI_SIM_ELEC;
				//PID_Slot_WriteConfig(DeviceId, i, config);
				if(slot_iospec[i] == 1)				// ������ģʽ
				{
					config[0] = 0x0f00 | CARD_CFG_DI_SIM_PULSE_UP;
					PID_Slot_WriteConfig(DeviceId, i, config);
					fprintf(fdbg, "CADPSS_PID_DI::init(), Slot %d, IoSpec::PULSE_UP\n", i);
				}
				else if (slot_iospec[i] == 2)		// ������ģʽ
				{
					config[0] = 0x0f00 | CARD_CFG_DI_SIM_PULSE_DOWN;
					PID_Slot_WriteConfig(DeviceId, i, config);
					fprintf(fdbg, "CADPSS_PID_DI::init(), Slot %d, IoSpec::PULSE_DOWN\n", i);
				}
				else								// Ĭ��Ϊ��ƽģʽ
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
		//����DI��������PID_DI_GrpConfig
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
}

//�ӽӿ���ȡ�����ݣ��洢��data��
I32 CADPSS_PID_DI::getdatafromPID()
{
	I32 n=size();

	PID_DI_GrpRead(DeviceId, 0, data, n);

	return 0;
}

//����DI��������ָ�����̷�����������
//����ʵ�����õ�������Ŀ
I32 CADPSS_PID_DI::setSimuVal(I32 procno, F64 *Vin)
{
	I32 i, n=0;

	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		//����data��������Vin��
		//��ֵ��������Ҫ�ı任�����ﴦ��
		if (a.VSimIO[0].prono==procno)
		{	//�ҵ�
/*
            Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1); //��Ҫ�޸�
/*DIʱ�깦��*/
			if (a.iospec == 1)				// ������
			{
				if ((data[i]>>15)&0x1)           //��ͨ
				{
					Vin[a.VSimIO[0].idxIO] = 1.0 + 0.000001*(data[i]&0x7F)/syncDT;
				}
				else
				{
					Vin[a.VSimIO[0].idxIO] = 0.0;
				}
			}
			else if (a.iospec == 2)			// ������
			{
				if (!((data[i]>>15)&0x1))       //��ͨ
				{
					Vin[a.VSimIO[0].idxIO] = 1.0 + 0.000001*(data[i]&0x7F)/syncDT;
				}
				else
				{
					Vin[a.VSimIO[0].idxIO] = 0.0;
				}
			}
			else							// ��ƽ
			{
				if ((data[i]>>15)&0x1)           //��ͨ
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

//�ͷŻ������������ͷź��Զ�����
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
