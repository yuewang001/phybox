#ifndef _PHYMAINRT_H
#define _PHYMAINRT_H
#include <vector>
#include <iostream>
#include <fstream>
#include "ADPSS_PID_ITF.h"

using namespace std;

const int MaxRecv=1024;                 // MPI buffer size
const int MaxLongLineLen=1024;			// Maximum line length

int FormComm();                         //形成通讯体
string ReadAllocin();                   //读alloc_in.pth文件，返回物理接口数据路径
int REmtParal();                        //读EMTPARAL.DAT文件
int ReadDvc(string filename);           //读*.DVC文件
void *RtEmtPhy(void *t);                //实时进程
void Wait_For_End();
void DealFIFO();
void WriteStr2FIFO(int hdl, char *str);
void RTDevOpenClose(int);
void RtPhyInit();
void AllocRcdBuf();
void ReleaseRcdBuf();

//MPI标识
struct SMPIID {
	int MyId;                           //本进程id
	int NumProcALL;                     //进程总数
} MpiId;

//仿真进程类型
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

//定义结构体记录与本物理接口进程通讯的计算进程信息
struct SSIMPROCINFO {
	int Id;
	int nIn;
	int nOut;
	double Data[MaxRecv];
};

//MPI进程和通信域标识
struct SPROCPHY {
	vector<SSIMPROCINFO> SimProc;		//和本进程通信的电磁子网进程标识号
	MPI_Comm CommEMT;                   //电磁暂态通信域
	MPI_Comm CommEMTCAL;               //电磁暂态子网通信域
	MPI_Comm CommST;                    //机电暂态通信域
	MPI_Comm CommCAL;
} ProcPhy;

//定义结构体记录EMTPARA.DAT文件信息
struct SEMTPARAINFO {
	int NPhyProc;						//物理接口进程数
	int NMatProc;						//Matlab接口进程数
	double T, DT;						//仿真时间和仿真步长
	double TStart;						//启动输出接口的时间
} EmtParaInfo;

//通道信息
struct SCHINFO {
	int InOut;							//输入输出类型。1：装置输入；2：装置输出
	int IdNo;							//输入输出序号。若多个仿真变量的IdNo相同，则根据ValueType将这多个变量合成
	string PhyName;						//装置类型
	int SimProc;						//接口类型
	int CompType;						//元件类型
	int CompNo;							//元件序号
	int SigType;						//信号类型
	int ValueType;						//数值类型
	int VarNo;							//变量编号
	double VarRatio;					//变量变比
	int PIDProc;						//接口进程号
	int PIDNo;							//接口箱号
	int SlotNo;							//板卡槽位号
	int ChNo;							//通道号
	double Gain;						//功放系数
	int IoSpec;							//信号设置
};

//定义结构体记录通道信息
struct SCARDINFO {
	int NChnl;							//通道数
	int NChIn;							//输入通道数
	int NChOut;							//输出通道数
	vector<SCHINFO> ch;					//通道数据
} PIDChInfo;

struct SPhyRtIO {
   int hdl_crtprint;       				// 屏幕信息输出句柄
   char dev_crtprint[32]; 				// 屏幕信息输出设备
   int hdl_FileError;   				// 文件信息输出句柄
   char dev_FileError[32];				// 文件信息输出设备
   enum { MRT_Initializing=0,MRT_Running=1,MRT_End=2};			// 运行标志。0: 没有开始；1: 正在运行；2: 计算完成
   int RTCalcMark;
} PhyRtIO;

//定义物理装置，对应一个接口结点
CADPSS_PID_ITF gPID;

#endif
