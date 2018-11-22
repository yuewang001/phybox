#ifndef _ADPSS_PID_BASE_H
#define _ADPSS_PID_BASE_H
#include "yjdnet.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define PID_SEM_MODE    0       // 1:�ź���ģʽ��0:��ѯģʽ
static char gs_card_str[5][10] = {"None", "DI", "DO", "AI", "AO"};


typedef unsigned char U8;
typedef short I16;
typedef unsigned short U16;
typedef long I32;
typedef unsigned long U32;
typedef double F64;
typedef void VOID;


//����һ�����������������
struct ADPSS_Simu_IOVAR{
	I32 prono;		//������ؼ�����̺�
	I32 VarNo;		//�����ڲ���ţ�����UD����������Ķ��塣
	F64 kamplify;	//�Ŵ�ϵ��
	U16 idxIO;		//�ڼ������������������е��±�λ��
};

//����һ���ӿ������������
struct ADPSS_PID_IOVAR{
	I32 idno;								//������/��������е����
	std::vector<ADPSS_Simu_IOVAR> VSimIO;	//������ֵ���ͣ�������һ����������������Ӧ
	enum SignalType {AI,AO,DI,DO};
	U16 Nsigntype;							//�ź�����
	U16 slotno;								//�忨��λ��
	U16 channelno;							//�忨ͨ����
	U16 iospec;								//�ź�����
	
	//DIRECTVAL������任��һһ��Ӧ�ı���
	//Virt2InstVal, ���������Чֵ��ʵ������������ֵ���ӿ������˲ʱֵ�ı���
	//D01VAL����������߼�ֵ���ӿ�����ǵ�ƽ�򿪹���
	//D01TIMEVAL����������߼�ֵ��ʱ������ֵ���ӿ�����ǰ����߼���ʱ����Ϣ��˫�ֽڱ���
	enum ValueType {DIRECTVAL=1,Virt2InstVal,D01VAL,D01TIMEVAL};
	U16 nValueType;							//��ֵ���ͣ�ȡֵValueType
	I16 idxVal2;							//����ͬʱ������������IO�������ָ��ڶ���IO������Ϣ�Ĵ洢�±ꡣ��˲ʱֵ�ķ�ֵ�����
};

// ����ļ�
extern FILE * fdbg;

#endif
