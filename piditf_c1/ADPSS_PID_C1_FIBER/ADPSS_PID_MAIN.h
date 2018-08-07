#ifndef _PHYMAINRT_H
#define _PHYMAINRT_H
#include <vector>
#include <iostream>
#include <fstream>
#include "ADPSS_PID_ITF.h"

using namespace std;

const int MaxRecv=1024;                 // MPI buffer size
const int MaxLongLineLen=1024;			// Maximum line length

int FormComm();                         //�γ�ͨѶ��
string ReadAllocin();                   //��alloc_in.pth�ļ�����������ӿ�����·��
int REmtParal();                        //��EMTPARAL.DAT�ļ�
int ReadDvc(string filename);           //��*.DVC�ļ�
void *RtEmtPhy(void *t);                //ʵʱ����
void Wait_For_End();
void DealFIFO();
void WriteStr2FIFO(int hdl, char *str);
void RTDevOpenClose(int);
void RtPhyInit();
void AllocRcdBuf();
void ReleaseRcdBuf();

//MPI��ʶ
struct SMPIID {
	int MyId;                           //������id
	int NumProcALL;                     //��������
} MpiId;

//�����������
enum SIMUTYPE_ALL
{
    SIMUTYPE_STCTRL=1,
    SIMUTYPE_STSUB,
    SIMUTYPE_STUD,
    SIMUTYPE_STMAT,
    SIMUTYPE_STPHY,
    SIMUTYPE_EMTSUB=101,
	SIMUTYPE_EMTUD,
    SIMUTYPE_EMTMAT,
	SIMUTYPE_EMTPHY,
    SIMUTYPE_STIO=201,
	SIMUTYPE_EMTIO
};

//����ṹ���¼�뱾����ӿڽ���ͨѶ�ļ��������Ϣ
struct SSIMPROCINFO {
	int Id;
	int nIn;
	int nOut;
	double Data[MaxRecv];
};

//MPI���̺�ͨ�����ʶ
struct SPROCPHY {
	vector<SSIMPROCINFO> SimProc;		//�ͱ�����ͨ�ŵĵ���������̱�ʶ��
	MPI_Comm CommEMT;                   //�����̬ͨ����
	MPI_Comm CommEMTCAL;               //�����̬����ͨ����
	MPI_Comm CommST;                    //������̬ͨ����
	MPI_Comm CommCAL;
} ProcPhy;

//����ṹ���¼EMTPARA.DAT�ļ���Ϣ
struct SEMTPARAINFO {
	int NPhyProc;						//����ӿڽ�����
	int NMatProc;						//Matlab�ӿڽ�����
	double T, DT;						//����ʱ��ͷ��沽��
	double TStart;						//��������ӿڵ�ʱ��
} EmtParaInfo;

//ͨ����Ϣ
struct SCHINFO {
	int InOut;							//����������͡�1��װ�����룻2��װ�����
	int IdNo;							//���������š���������������IdNo��ͬ�������ValueType�����������ϳ�
	string PhyName;						//װ������
	int SimProc;						//�ӿ�����
	int CompType;						//Ԫ������
	int CompNo;							//Ԫ�����
	int SigType;						//�ź�����
	int ValueType;						//��ֵ����
	int VarNo;							//�������
	double VarRatio;					//�������
	int PIDProc;						//�ӿڽ��̺�
	int PIDNo;							//�ӿ����
	int SlotNo;							//�忨��λ��
	int ChNo;							//ͨ����
	double Gain;						//����ϵ��
	int IoSpec;							//�ź�����
};

//����ṹ���¼ͨ����Ϣ
struct SCARDINFO {
	int NChnl;							//ͨ����
	int NChIn;							//����ͨ����
	int NChOut;							//���ͨ����
	vector<SCHINFO> ch;					//ͨ������
} PIDChInfo;

struct SPhyRtIO {
   int hdl_crtprint;       				// ��Ļ��Ϣ������
   char dev_crtprint[32]; 				// ��Ļ��Ϣ����豸
   int hdl_FileError;   				// �ļ���Ϣ������
   char dev_FileError[32];				// �ļ���Ϣ����豸
   enum { MRT_Initializing=0,MRT_Running=1,MRT_End=2};			// ���б�־��0: û�п�ʼ��1: �������У�2: �������
   int RTCalcMark;
} PhyRtIO;

//��������װ�ã���Ӧһ���ӿڽ��
CADPSS_PID_ITF gPID;

#endif
