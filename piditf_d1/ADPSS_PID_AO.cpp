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

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���������ϵͳ�����Ըñ����󹩸��忨���롣
I32 CADPSS_PID_AO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
						   I32 VarNo, U16 SlotNo, U16 nChanNo,  F64 KAmplify)
{
	I32 i;
	I32 nret;

	//���ұ����Ƿ��Ѿ�����
	for (i=0;i<VarInf.size();i++)
	{
		ADPSS_PID_IOVAR &a=VarInf[i];
		if (a.idno==idno)
		{	//�ҵ�������������������Ӧһ���ź�

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
	{	//�ҵ�����ԭ�б���������һ���������
		VarInf[i].VSimIO.push_back(svar);
		nret=i;
		if(DEBUG)
		{
			fprintf(fdbg,"CADPSS_PID_AO::addavar: i:%d< VarINf.size()��%d\n",i,VarInf.size());
		}

	}
	else
	{
		//����һ������
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
		fprintf(fdbg, "procno=%d  idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d, NumInstant=%d\n",ProcNo, idxio, VarNo, SlotNo, nChanNo, ValueType, NumInstant);
		fprintf(fdbg, "VarInf.size=%d\n", nret);
	}

	return nret;
}

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
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

	if(DEBUG)
	{
		fprintf(fdbg, "CADPSS_PID_AO::allocBuf(), allocat buf size :%d", n);
	}

	return 0;
}

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
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

//�ͷŻ�����
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

//ȷ�������󣬳�ʼ�����ݴ洢�������ýӿ��������Ϣ
//0��ʾ�ɹ�����0��ʾʧ��
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



//��ʼ����Чֵת˲ʱֵ��ر�������������ת������ʱ����
//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
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

//����ָ�����̷���������������AO����
//����ʵ�����õ�������Ŀ
I32 CADPSS_PID_AO::GetSimuVal(I32 procno,F64 *Vin)
{
	if(DEBUG)
	{
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal start for procno: %d \n",procno);
	}

	I32 i,j,n=0;


	if(DEBUG)
	{
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal VarINf.size=%d\n",VarInf.size());
	}


	if(DEBUG)
	{
		fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal print VarInf value------\n");
		for (i=0; i<VarInf.size(); i++)
		{
			ADPSS_PID_IOVAR& b=VarInf[i];
			for (j=0; j<b.VSimIO.size(); j++)
			{
				ADPSS_Simu_IOVAR& a=b.VSimIO[j];
			   fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal print VarInf value No:%d   prono=%d   VarNo=%d   IdxIO=%d  idno=%d------\n",i,a.prono,a.VarNo,a.idxIO,b.idno);
			}

		}
	}
	for (i=0; i<VarInf.size(); i++)
	{
		ADPSS_PID_IOVAR& a=VarInf[i];

		if(DEBUG)
		{
			//fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal VarInf[%d].VSimIO.size()=%d\n",i,a.VSimIO.size());
		}
		for (j=0; j<a.VSimIO.size(); j++)
		{
			ADPSS_Simu_IOVAR& b=a.VSimIO[j];
			//Volt[a.idno]=Vin[b.idxIO];

			if(DEBUG)
			{
				fprintf(fdbg, "print data: idNo=%d\n",a.idno);
				fprintf(fdbg, "print data: idxIO=%d\n",b.idxIO);
				fprintf(fdbg, "print data: varNo=%d\n",b.VarNo);
				fprintf(fdbg, "print data: prono=%d\n",b.prono);

			}
			if (b.prono==procno)	//�ҵ�
			{
				Volt[a.idno-1]=Vin[b.idxIO];
				//Volt[i]=Vin[b.idxIO];
				if(DEBUG)
				{
					fprintf(fdbg, "print data: Volt[%d]=%20.15f\n",a.idno-1,Volt[a.idno-1]);
					//fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal idno:%d   %20.15f\n",a.idno-1.idno,Vin[b.idxIO]);
					//fprintf(fdbg,"CADPSS_PID_AO::GetSimuVal Volt[%d]=%20.15f\n",i,Volt[a.idno-1]);

				}

				/*
				if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)	//˲ʱֵ
				{
					//printf("CADPSS_PID_AO::GetSimuVal Virt2InstVal \n");

					if (1==j)	//�鲿ֵ
					{
						F64 r,x;
						r=Vin[a.VSimIO[0].idxIO];
						x=Vin[b.idxIO];
						I32 idx=a.idxVal2;
						VinstInAL[idx]=VinstInA[idx];
						VinstInPL[idx]=VinstInP[idx];
						VinstInA[idx]=b.kamplify*sqrt((r*r+x*x)*2);	//ת��Ϊ��ֵ��A**0.5
						VinstInP[idx]=atan2(x,r);
					}
				}
				else
				{	//��Чֵ����������ֵ�ۼ�
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

//����˲ʱֵ��ֵ
//nt��������̬���沽��
//knt������ӿ�����������󲽳��ڣ�
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
		if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)	//˲ʱֵ
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

//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
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

//�ͷŻ������������ͷź��Զ�����
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
