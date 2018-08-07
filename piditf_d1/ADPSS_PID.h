#ifndef _ADPSS_PID_H
#define _ADPSS_PID_H
#include "ADPSS_PID_AI.h"
#include "ADPSS_PID_AO.h"
#include "ADPSS_PID_DI.h"
#include "ADPSS_PID_DO.h"

class CADPSS_PID
{
public:
	CADPSS_PID(U16 nPIDNo);
	CADPSS_PID(const CADPSS_PID &a);
	~CADPSS_PID(void);

	//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
	//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
	//Idno:��ţ�����/������ֱ��1��ʼ��š�
	//ProcNo�������źŵĽӿڽ��̺ţ���0��ʼ��ţ�
	//signType���ź����͡�AI=0,AO,DI,DO
	//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
	//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
	//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
	//nChanNo��ͨ����ţ���0��ʼ��š�
	//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
				I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//ȷ�������󣬳�ʼ�����ݴ洢��
	//0��ʾ�ɹ�����0��ʾʧ��
	I32 init(F64 SyncDT);

	//ȷ��������Ŀ�󣬷��仺����
	I32 allocBuf();

	//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
	void initInstantPara(F64 dDT,F64 df0, I32 nkIT);

	//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
	//inline I32 getdatafromPID();
	I32 getdatafromPID();

	//����AI��������ָ�����̷�����������
	//����ʵ�����õ�������Ŀ
	I32 setSimuVal(I32 procno,F64 *Vin);

	//����ָ�����̷���������������AO����
	//����ʵ�����õ�������Ŀ
	I32 GetSimuVal(I32 procno,F64 *Vin);

	//����˲ʱֵ��ֵ
	I32 updateInstantVal(I32 nt,I32 kit);

	//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
	I32 SenddataToPID(U16 DeviceId);

	I32 ClearOutput();

	//�ͷŻ������������ͷź��Զ�����
	I32 finalize();

	//ȡ�����������Ŀ
	I32 getNumIn();

	//ȡ�����������Ŀ
	I32 getNumOut();

	//�Ƿ����˲ʱֵ��Ϣ����ʼ������Ч
	bool ishaveInstantInput();

	//MarkInput��غ���
	//��ѯ�Ƿ����
	I16 MarkInput_IsUpdated();

	//���MarkInput���
	I16 MarkInput_Clear();

	//�ȴ�MarkInput�仯�󷵻�
	I16 MarkInput_Wait(U16 Timeout);

	//��ѯ�ӿ�����
	I32 getPIDNo() {return PIDNo;};

    //��������״̬
    bool setMaster() {isMaster=true;};

	//��ѯ����״̬
	bool getisMaster() {return isMaster;};

    //ֹͣͬ���ź�
    I16 stopSync();

	//��������
	I16 startSim();

	//ֹͣ����
	I16 stopSim();

private:
	//����/�������
	CADPSS_PID_AI vai;
	CADPSS_PID_AO vao;
	CADPSS_PID_DI vdi;
	CADPSS_PID_DO vdo;

	//�ӿ�����
	I32 PIDNo;

	//�Ƿ�����
	bool isMaster;

	//ͬ��Ƶ��
	U32 syncFreq;

	//ͬ������
	F64 syncDT;
};
#endif
