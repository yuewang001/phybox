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
						   I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
	if(DEBUG)
	fprintf(fdbg,"CADPSS_PID_DI::addavar: idxio=%d,idno=%d\n",idxio,idno);
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
		//var.iospec = IoSpec;


		VarInf.push_back(var);
		nret=VarInf.size();

		if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_DI::new added value: idxio=%d,idno=%d\n",idxio,idno);
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

	DeviceId=id;
	if(DEBUG)
		fprintf(fdbg, "CADPSS_PID_DI::init(), PIDNo=%d, SigNum=%d\n", id, n);
	return 0;
	
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

	if(DEBUG)
		fprintf(fdbg,"CADPSS_PID_DI::setSimuVal:size VarInf: %d",VarInf.size());

	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];
		//����data��������Vin��
		//��ֵ��������Ҫ�ı任�����ﴦ��
		if (a.VSimIO[0].prono==procno)
		{	//�ҵ�
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
            Vin[a.VSimIO[0].idxIO]=((data[i]>>7)&0x1); //��Ҫ�޸�
/*DIʱ�깦��*/
			/*
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
	if(DEBUG)
	fprintf(fdbg,"CADPSS_PID_DI::setSimuVal: n=%d\n",n);
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
