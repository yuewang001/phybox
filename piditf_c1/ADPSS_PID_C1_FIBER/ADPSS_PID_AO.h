#ifndef _ADPSS_PID_AO_H
#define _ADPSS_PID_AO_H
#include <vector>
#include "ADPSS_PID_Base.h"

//�ӿ���AO������
class CADPSS_PID_AO
{
public:
	CADPSS_PID_AO(void);
	CADPSS_PID_AO(const CADPSS_PID_AO &a);
	~CADPSS_PID_AO(void);

	//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
	//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
	//Idno:��ţ�����/������ֱ��1��ʼ��š�
	//ProcNo�������źŵĽӿڽ��̺ţ���0��ʼ��ţ�
	//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
	//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
	//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
	//nChanNo��ͨ����ţ���0��ʼ��š�
	//KAmplify���źŷŴ���,���ڷ���ϵͳ���������ϵͳ�����Ըñ����󹩸��忨���롣
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
		        I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//ȷ�������󣬳�ʼ�����ݴ洢�������ýӿ��������Ϣ
	//0��ʾ�ɹ�����0��ʾʧ��
	I32 init(U16 id);

	//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
	I32 allocBuf();

	//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
	I32 allocBuf(I32 nVar);

	//�ͷŻ�����
	I32 releaseBuf();

	//��ʼ����Чֵת˲ʱֵ��ر�������������ת������ʱ����
	//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
	void initInstantPara(F64 dDT, F64 df0, I32 nkIT);

	//����ָ�����̷���������������AO����
	//����ʵ�����õ�������Ŀ
	I32 GetSimuVal(I32 procno,F64 *Vin);

	//����˲ʱֵ��ֵ
	I32 updateInstantVal(I32 nt,I32 kit);

	//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
	I32 SenddataToPID();

	I32 ClearOutput();

	//ȡ�ñ�����Ŀ
	I32 size(){return VarInf.size();};

	//�ͷŻ������������ͷź��Զ�����
	I32 finalize();

	//�Ƿ����˲ʱֵ��Ϣ����ʼ������Ч
	bool ishaveInstantInput(){return (NumInstant>0);};

	I32 setSyncDT(F64 DT);

private:
	//�豸��ţ������豸��غ�������
	U16 DeviceId;

	//ͨ����Ϣ
	std::vector<ADPSS_PID_IOVAR> VarInf;

	//Slp��	ͨ��ϵ����������ÿһ��Ԫ�ر�ʾһ��AOͨ����ϵ����
	//Itcp��ͨ����ƫ��������ÿһ��Ԫ�ر�ʾһ��AOͨ������ƫ��
	F64 *Slp,*Itcp;

	//ԭʼ��ֵ
	U16 * data;

	//ת������ֵ
	F64 * Volt;

#ifdef ZX_DBG
	F64 * datalog;
#endif

	//˲ʱֵ�任�����Ϣ���ڳ�ʼ��ʱ����
	I32 NumInstant;	//˲ʱֵ��Ŀ

	//���κ��ϴε�˲ʱֵ�������ķ�ֵ�ͽǶ�
	F64 *VinstInA,*VinstInP,*VinstInAL,*VinstInPL;

	//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
	I32 kIT;
	F64 DT,f0;

	F64 syncDT;
};
#endif
