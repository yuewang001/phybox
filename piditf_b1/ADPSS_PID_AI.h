#ifndef _ADPSS_PID_AI_H
#define _ADPSS_PID_AI_H
#include <vector>
#include "ADPSS_PID_Base.h"

using namespace std;

//�ӿ���AI������
class CADPSS_PID_AI
{
public:
	CADPSS_PID_AI(void);
	CADPSS_PID_AI(const CADPSS_PID_AI &a);
	~CADPSS_PID_AI(void);

	//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
	//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
	//Idno:��ţ�����/������ֱ��1��ʼ��š�
	//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
	//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
	//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
	//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
	//nChanNo��ͨ����ţ���0��ʼ��š�
	//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType,
			    I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//ȷ�������󣬳�ʼ�����ݴ洢��
	//0��ʾ�ɹ�����0��ʾʧ��
	I32 init(U16 id);

	//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
	I32 allocBuf();

	//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
	I32 allocBuf(I32 nVar);

	//�ͷŻ�����
	I32 releaseBuf();

	//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
	I32 getdatafromPID();

	//����AI��������ָ�����̷�����������
	//����ʵ�����õ�������Ŀ
	I32 setSimuVal(I32 procno,double *Vin);

	//ȡ�ñ�����Ŀ
	I32 size(){return VarInf.size();};

	//�ͷŻ������������ͷź��Զ�����
	I32 finalize();

private:
	//ͨ����Ϣ
	std::vector<ADPSS_PID_IOVAR> VarInf;

	//�豸��ţ������豸��غ�������
	U16 DeviceId;

	//Slp��	ͨ��ϵ����������ÿһ��Ԫ�ر�ʾһ��AIͨ����ϵ����
	//Itcp��ͨ����ƫ��������ÿһ��Ԫ�ر�ʾһ��AIͨ������ƫ��
	F64 *Slp,*Itcp;

	//ԭʼ��ֵ
	U16 * data;

	//ת������ֵ
	F64 * Volt;
};
#endif
