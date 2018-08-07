#ifndef _ADPSS_PID_DO_H
#define _ADPSS_PID_DO_H
#include <vector>
#include "ADPSS_PID_Base.h"

//�ӿ���DO������
class CADPSS_PID_DO
{
public:
	CADPSS_PID_DO(void);
	CADPSS_PID_DO(const CADPSS_PID_DO &a);
	~CADPSS_PID_DO(void);

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

	//ȷ��������Ŀ�󣬷��仺�������data
	I32 allocBuf();

	//ȷ��������Ŀ�󣬷��仺�������data
	I32 allocBuf(I32 nVar);

	//�ͷŻ�����
	I32 releaseBuf();

	//����ָ�����̷���������������DO����
	//����ʵ�����õ�������Ŀ
	I32 GetSimuVal(I32 procno,double *Vin);

	//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
	I32 SenddataToPID();

	//ȡ�ñ�����Ŀ
	I32 size(){return VarInf.size();};

	//�ͷŻ������������ͷź��Զ�����
	I32 finalize();

private:
	//ͨ����Ϣ
	std::vector<ADPSS_PID_IOVAR> VarInf;

	//�豸��ţ������豸��غ�������
	U16 DeviceId;

	//ԭʼ��ֵ
	U8 * data;
};
#endif
