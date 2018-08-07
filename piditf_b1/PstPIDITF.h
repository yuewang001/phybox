#ifndef _PSTPIDITF_H
#define _PSTPIDITF_H
#include "mpi.h"
#include <vector>
#include "ADPSS_PID_ITF.h"
//主控发送的IO变量信息结构
const int MaxLen_Name=24;
struct PSTPIDIOVAR {
   int MarkIO; //数据类型, 1:为输入；2:为输出；
   int idno;   //变量序号
   char PHY_Name[MaxLen_Name];   //物理装置名称
   int NetProcNo; //子网进程号
   int CompType;  //元件类型
   char CompName[MaxLen_Name];   //元件名称
   int SignalType;   //信号类型，AI=0,AO,DI,DO
   int ValueType; //数值类型
   int VarNo;  //变量内部编号
   int PIBNo;  //接口箱编号
   int SlotNo; //板卡槽位号
   int SignNo; //信号编号
   double KAmplify;  //放大倍数
};

struct PSTNET_PIDIO {
   int MarkIO; //数据类型, 1:为输入；2:为输出；
   int CompType;  //元件类型
   char CompName[MaxLen_Name];   //元件名称
   int VarNo;  //变量内部编号
};

int FormComm();
void SubDealParaErr(int &Ierr);
int RecvSendPhyIO();
void RTSTPhySimulating();
void *RTSTPhySim(void *arg);
void wait_for_end();
void DealFIFO();
void RTDevOpenClose(int);
void Writestr2FIFO(int hdl, char *str);

void rtphyinit();

struct SMPIID {
   int myid,numprocs,minPIDProc;
};
const int MaxLeng_Buffer=4096;

//enum SIMUTYPE_ST{SIMUTYPE_STCTRL=1,SIMUTYPE_STSUB,SIMUTYPE_STUD,SIMUTYPE_STMAT,SIMUTYPE_STPHY};

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

//进程信息
struct SPROCPHY {
   //PCMain：主控进程号
   //myidst,numprocst:在CommST中的当前进程号和进程数目
   int PCMain,myidst,numprocst;
   //相关子网
   std::vector<int> pcsubnet;
   //CommST 通讯组
   MPI_Comm CommST;
   //CommCal 计算进程通信组
   MPI_Comm CommCal;
};
//IO变量信息
struct SNETIO {
   //子网数目，与pcsubnet对应
   int numSubnet;
   //存储输入输出变量数值
   double * Vin,*Vout;
   //存储上一步变量数值
   double * Vlast;
   //与pcsubnet对应的子网I在Vin、Vout中的下标起始位置
   int * IdVin, *IdVout;
   //步长
   double dt;
   //频率
   int fhz;
   //大步长中包含的瞬时值小步长数目
   int Ikt;
};

struct SPhyRTIO {
   // 屏幕打印
   int hdl_crtprint;       // 屏幕信息输出
   char dev_crtprint[32]; 
   // 暂稳输出
   int hdl_FileError;   // 错误信息输出
   char dev_FileError[32];
   // 计算完成标帜
   // 0 实时计算没有开始
   // 1 实时计算正在进行
   // 2 实时计算完成
   enum { MRT_Initializing=0,MRT_Running=1,MRT_End=2};
   int MarkRTCalculation; 
};
#endif
