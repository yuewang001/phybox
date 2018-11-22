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

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڱ�������̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
//signType���ź����͡�AI=0,AO,DI,DO
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//nPIDNo���ӿ���ţ���0��ʼ��š�
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
I32 CADPSS_PID_ITF::addavar(I32 idxio, I32 idno, I32 ProcNo, I32 signType, I32 ValueType, 
							I32 VarNo, U16 nPIDNo, U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec)
{
	I32 i;

	if (!pid[nPIDNo])		// ��δʹ�ã���������
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
		if (pid[i])		// ��Чpid
			pid[i]->allocBuf();
	}

	return 0;
}

I32 CADPSS_PID_ITF::pushPID()
{
	for (int i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
	//	if (pid[i])		// ��Чpid
	//		VPID.push_back(*pid[i]);
	}

	return 0;
}


//ȷ�������󣬳�ʼ�����ݴ洢��
//0��ʾ�ɹ�����0��ʾʧ��
extern int gs_dev_no[MAX_DEVICES];
I32 CADPSS_PID_ITF::init(F64 SyncDT)
{
	I32 i, nret=-1;

	nret = pid_thread_create();

	if(nret<0) return -1;

	//nret = gpsTimer_thread_create();

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

	printf(" CADPSS_PID_ITF::init Done!\n");

	return  nret;
}

//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
void CADPSS_PID_ITF::initInstantPara(F64 dDT,F64 df0, I32 nkIT)
{
	I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
	{
		if (pid[i])	{
			pid[i]->initInstantPara(dDT,df0,nkIT);}
	}
}

//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
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

//����AI��������ָ�����̷�����������
//����ʵ�����õ�������Ŀ
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

//����ָ�����̷���������������AO����
//����ʵ�����õ�������Ŀ
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

//����˲ʱֵ��ֵ
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

//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
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

//�ͷŻ������������ͷź��Զ�����
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

//ȡ�����������Ŀ
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

//ȡ�����������Ŀ
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

//�Ƿ����˲ʱֵ��Ϣ����ʼ������Ч
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

//MarkInput��غ���
//��ѯ�Ƿ����
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

//���MarkInput���
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

//�ȴ�MarkInput�仯�󷵻�
I16 CADPSS_PID_ITF::MarkInput_Wait(U16 Timeout)
{
	I16 nret=0, cnt=0, err=0;
	I32 i;
	int SleepTime;

    // �ȴ�ÿһ��PID
    // �����ź�����ʽ��MarkInput_Wait�ȴ��ɹ�ʱ����0
    // ���ڲ�ѯ��ʽ��MarkInput_Wait���صײ��������ݸ��´���������Ϊ1
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

//ֹͣͬ���ź�
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

//������վ����
I16 CADPSS_PID_ITF::startSlaveSim()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
        if (pid[i])	{  
		if (i!=MasterNo)    //��վ
        {
            if (!pid[i]->startSim()) nret=1;
		}}
    }

    return nret;
}

//�������ط��棬��������Ӧ���������д�վ֮��
I16 CADPSS_PID_ITF::startMasterSim()
{
    I16 nret=0;
    I32 i;

	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
		if (pid[i])	{         
		if (i==MasterNo)    //����
        {
            if (!pid[i]->startSim()) nret=1;
		}}
    }

    return nret;
}

//ֹͣ����
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

//�������ؽӿ��䣬����С��Žӿ��䣬�����������ر��
//������С��Žӿ�����VPID�����е����
I16 CADPSS_PID_ITF::setMaster()
{
    I16 nret=0;
    I32 i, minPIDNo;

    //������С��Žӿ���
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

    //��ѯ��С��Žӿ�����VPID�����е����
	for (i=0; i<MAX_PID_NUM_PER_PROC; i++)
    {
		if (pid[i])	{         
		if (minPIDNo == pid[i]->getPIDNo())
        {
            MasterNo=i;
            break;
		}}
    }

    //���ýӿ������ر��
    pid[MasterNo]->setMaster();

    return nret;
}
