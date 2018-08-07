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

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
//signType���ź����͡�AI=0,AO,DI,DO
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
I32 CADPSS_PID::addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
						I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
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
			nret=vdi.addavar(idxio,idno,ProcNo,ValueType,VarNo,SlotNo,nChanNo,KAmplify);
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

//��ȷ��������Ŀ�󣬷��仺�����ռ�
I32 CADPSS_PID::allocBuf()
{
	vai.allocBuf();
    vao.allocBuf();
	vdi.allocBuf();
	vdo.allocBuf();

	return 0;
}

//ȷ�������󣬳�ʼ�����ݴ洢��
//0��ʾ�ɹ�����0��ʾʧ��
I32 CADPSS_PID::init(F64 SyncDT)
{
	I32 nret;
	U32 f0;
	U16 Ism;

#if PID_SEM_MODE
	PID_Register(PIDNo, MODE_SEM);				//��ʼ��
#else
    PID_Register(PIDNo, MODE_QRY);				//��ʼ��
#endif
	PID_Unit_Cmd(PIDNo, CMD_STOP_SIM);			//���ͷ���ֹͣ����
	PID_Sync_ReadConfig(PIDNo, &f0, &Ism);		//��ͬ����Ϣ
	syncDT = SyncDT;
	syncFreq = (U32)(1.0/SyncDT);
	PID_Sync_WriteConfig(PIDNo, syncFreq, SM_SLAVE);

	//�忨��ʼ��
	nret=-1;
	if (0==vai.init(PIDNo)) nret=0;
	if (0==vao.init(PIDNo)) nret=0;
	if (0==vdi.init(PIDNo)) nret=0;
	if (0==vdo.init(PIDNo)) nret=0;

	vdi.setSyncDT(syncDT);

	fprintf(fdbg, "CADPSS_PID::init(), PID=%d, Master=%d, SyncFreq=%d, nret=%d\n", PIDNo, Ism, syncFreq, nret);
	return nret;
}

//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
void CADPSS_PID::initInstantPara(F64 dDT, F64 df0, I32 nkIT)
{
	vao.initInstantPara(dDT,df0,nkIT);
}

//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
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

//����AI��������ָ�����̷�����������
//����ʵ�����õ�������Ŀ
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

//����ָ�����̷���������������AO����
//����ʵ�����õ�������Ŀ
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

//����˲ʱֵ��ֵ
I32 CADPSS_PID::updateInstantVal(I32 nt,I32 kit){
	return vao.updateInstantVal(nt,kit);
}

//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
I32 CADPSS_PID::SenddataToPID()
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

//�ͷŻ������������ͷź��Զ�����
I32 CADPSS_PID::finalize()
{
	I32 nret;

	if (PIDNo<0) return -1;

	//�忨�ͷ�
	nret=-1;
	if (0==vai.finalize()) nret=0;
	if (0==vao.finalize()) nret=0;
	if (0==vdi.finalize()) nret=0;
	if (0==vdo.finalize()) nret=0;
	
	//�ӿ����ͷ�
	PID_Release(PIDNo);
	fprintf(fdbg, "CADPSS_PID::finalize(), PID=%d, nret=%d\n", PIDNo, nret);

	PIDNo=-1;
	return nret;
}

//ȡ������ӿ������������Ŀ
I32 CADPSS_PID::getNumIn()
{
	I32 n=0;
	n+=vao.size();
	n+=vdo.size();
	return n;
}

//ȡ������ӿ������������Ŀ
I32 CADPSS_PID::getNumOut()
{
	I32 n=0;
	n+=vai.size();
	n+=vdi.size();
	return n;
}

//�Ƿ����˲ʱֵ��Ϣ����ʼ������Ч
bool CADPSS_PID::ishaveInstantInput()
{
	return vao.ishaveInstantInput();
}

//��ѯ�Ƿ����
I16 CADPSS_PID::MarkInput_IsUpdated()
{
//	return PID_MarkInput_IsUpdated(PIDNo);
}

//���MarkInput���
I16 CADPSS_PID::MarkInput_Clear()
{
	return PID_MarkInput_Clear(PIDNo);
}

//�ȴ�MarkInput�仯�󷵻�
I16 CADPSS_PID::MarkInput_Wait(U16 Timeout)
{
	return PID_MarkInput_Wait(PIDNo,Timeout);
}

//ֹͣͬ���ź�
I16 CADPSS_PID::stopSync()
{
    return PID_Sync_WriteConfig(PIDNo, syncFreq, SM_SLAVE);
}

//��������
I16 CADPSS_PID::startSim()
{
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

//ֹͣ����
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
