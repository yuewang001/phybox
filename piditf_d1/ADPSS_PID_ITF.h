#ifndef _ADPSS_PID_ITF_H
#define _ADPSS_PID_ITF_H
#include <vector>
#include "ADPSS_PID.h"

#define MAX_PID_NUM_PER_PROC	4		//ÿ���ӿڽ������֧�ֵĽӿ�����Ŀ

//����Ľӿ��࣬�ṩ�Զ���ӿ������һ�µĹ���
class CADPSS_PID_ITF
{
public:
	CADPSS_PID_ITF(void);
	~CADPSS_PID_ITF(void);

	//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
	//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
	//Idno:��ţ�����/������ֱ��1��ʼ��š�
	//ProcNo�������źŵĽӿڽ��̺ţ���0��ʼ��ţ�
	//signType���ź����͡�AI=0,AO,DI,DO
	//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
	//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
	//nPIDNo���ӿ���ţ���0��ʼ��š�
	//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
	//nChanNo��ͨ����ţ���0��ʼ��š�
	//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
				I32 VarNo,U16 nPIDNo,  U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//ȷ��������Ŀ�󣬷��仺�����ռ�
	I32 allocBuf();

	//CADPSS_PID��������Ȱ��ڲ��ź�push��VarInf�У�Ȼ����ܱ�push��VPID�У�����std��֧��
	//�ú������Ѿ�����õ�CADPSS_PID����push��VPID��
	I32 pushPID();

	//ȷ�������󣬳�ʼ�����ݴ洢��
	//0��ʾ�ɹ�����0��ʾʧ��
	I32 init(F64 SyncDT);

	//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
	void initInstantPara(F64 dDT,F64 df0, I32 nkIT);

	//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
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
	I32 SenddataToPID();

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

	//ֹͣͬ���ź�
	I16 stopSync();

	//������վ����
	I16 startSlaveSim();

	//�������ط��棬��������Ӧ���������д�վ֮��
	I16 startMasterSim();

	//ֹͣ����
	I16 stopSim();

	//�������ؽӿ��䣬����С��Žӿ��䣬�����������ر��
	I16 setMaster();

private:
	//�ӿ�������
	std::vector<CADPSS_PID> VPID;

	//���ؽӿ�����VPID�еı��
	I32 MasterNo;

	//CADPSS_PID����ָ������飬����ʹ����Ҫ���з���
	CADPSS_PID *pid[MAX_PID_NUM_PER_PROC];
};
#endif
