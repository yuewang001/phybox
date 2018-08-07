//C���Խӿڣ���C��FORTRAN���Խӿ�ʹ��
#pragma once
#include "ADPSS_PID_ITF.h"
//ȫ�ֿ��ƶ���
extern CADPSS_PID_ITF gPIDITF;

#ifdef __cplusplus   
extern   "C"   {   
#endif   
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
int PID_ITF_addavar(int idxio,int idno,int ProcNo,int signType, int ValueType, 
			int VarNo,U16 nPIDNo,  U16 SlotNo, U16 nChanNo, F64 KAmplify);
//ȷ�������󣬳�ʼ�����ݴ洢��
//0��ʾ�ɹ�����0��ʾʧ��
int PID_ITF_init(U32 SyncFreq);
//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
void PID_ITF_initInstantPara(double dDT,double df0, int nkIT);
//�ӽӿ���ȡ�����ݣ��洢��data��Volt�С�
int PID_ITF_getdatafromPID();
//����AI��������ָ�����̷�����������
//����ʵ�����õ�������Ŀ
int PID_ITF_setSimuVal(int procno,F64 *Vin);
//����ָ�����̷���������������AO����
//����ʵ�����õ�������Ŀ
int PID_ITF_GetSimuVal(int procno,F64 *Vin);
//����˲ʱֵ��ֵ
int PID_ITF_updateInstantVal(int nt,int kit);
//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
int PID_ITF_SenddataToPID();
//�ͷŻ������������ͷź��Զ�����
int PID_ITF_finalize();
//ȡ�����������Ŀ
int PID_ITF_getNumIn();
//ȡ�����������Ŀ
int PID_ITF_getNumOut();
//�Ƿ����˲ʱֵ��Ϣ����ʼ������Ч
bool PID_ITF_ishaveInstantInput();
//MarkInput��غ���
//��ѯ�Ƿ����
I16 PID_ITF_MarkInput_IsUpdated();
//���MarkInput���
I16 PID_ITF_MarkInput_Clear();
//�ȴ�MarkInput�仯�󷵻�
I16 PID_ITF_MarkInput_Wait(U16 Timeout);
#ifdef __cplusplus   
};
#endif   
