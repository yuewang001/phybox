#ifndef _PSTPIDITF_H
#define _PSTPIDITF_H
#include "mpi.h"
#include <vector>
#include "ADPSS_PID_ITF.h"
//���ط��͵�IO������Ϣ�ṹ
const int MaxLen_Name=128;
struct PSTPIDIOVAR {
   int Valid; //��������, 1:Ϊ���룻2:Ϊ�����
   int Vidno;   //�������
   int ISST;   //������ʶ�� 1������ 2�����
   int SimProcNo; //�������̺�
   int CompType;  //Ԫ������
   int CompNo; //Ԫ���±��
   int VarNo;  //�����ڲ����
   int SigType;   //�ź����ͣ�AI=0,AO,DI,DO
   int ValueType; //��ֵ����
   int PidNo; //�������ڱ��
   int PidChNo; //ͨ����
   int Index; //��������
   char CompName[MaxLen_Name];   //Ԫ������
};

struct PSTNET_PIDIO {
   int MarkIO; //��������, 1:Ϊ���룻2:Ϊ�����
   int CompType;  //Ԫ������
   char CompName[MaxLen_Name];   //Ԫ������
   int VarNo;  //�����ڲ����
};

int FormComm();
void SubDealParaErr(int &Ierr);
int RecvSendPhyIO();
void DigitalSimulating();

void rtphyinit();

struct SMPIID {
   int myid,numprocs,minPIDProc;
};
const int MaxLeng_Buffer=4096;

//enum SIMUTYPE_ST{SIMUTYPE_STCTRL=1,SIMUTYPE_STSUB,SIMUTYPE_STUD,SIMUTYPE_STMAT,SIMUTYPE_STPHY};

//�����������
enum SIMUTYPE_ALL
{
   SIMUTYPE_STCTRL=1,
   SIMUTYPE_STSUB,
   SIMUTYPE_STUD,
   SIMUTYPE_STMAT,
   SIMUTYPE_STPHY,
   SIMUTYPE_STCom,
   SIMUTYPE_EMTSUB=101,
   SIMUTYPE_EMTUD,
   SIMUTYPE_EMTMAT,
   SIMUTYPE_EMTPHY,
   SIMUTYPE_STIO=201,
   SIMUTYPE_EMTIO
};

//������Ϣ
struct SPROCPHY {
   //PCMain�����ؽ��̺�
   //myidst,numprocst:��CommST�еĵ�ǰ���̺źͽ�����Ŀ
   int PCMain,myidst,numprocst;
   //�������
   std::vector<int> pcsubnet;
   //CommST ͨѶ��
   MPI_Comm CommST;
   //CommCal �������ͨ����
   MPI_Comm CommCal;
};
//IO������Ϣ
struct SNETIO {
   //������Ŀ����pcsubnet��Ӧ
   int numSubnet;
   //�洢�������������ֵ
   double * Vin,*Vout;
   //�洢��һ��������ֵ
   double * Vlast;
   //��pcsubnet��Ӧ������I��Vin��Vout�е��±���ʼλ��
   int * IdVin, *IdVout;
   //����
   double dt;
   //Ƶ��
   int fhz;
   //�󲽳��а�����˲ʱֵС������Ŀ
   int Ikt;
};

struct SPhyRTIO {
   // ��Ļ��ӡ
   int hdl_crtprint;       // ��Ļ��Ϣ���
   char dev_crtprint[32]; 
   // �������
   int hdl_FileError;   // ������Ϣ���
   char dev_FileError[32];
   // ������ɱ���
   // 0 ʵʱ����û�п�ʼ
   // 1 ʵʱ�������ڽ���
   // 2 ʵʱ�������
   enum { MRT_Initializing=0,MRT_Running=1,MRT_End=2};
   int MarkRTCalculation; 
};
#endif
