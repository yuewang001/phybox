#ifndef _ADPSS_PID_BASE_H
#define _ADPSS_PID_BASE_H
#include "yjdnet.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define PID_SEM_MODE    0       // 1:信号量模式；0:查询模式
static char gs_card_str[5][10] = {"None", "DI", "DO", "AI", "AO"};


typedef unsigned char U8;
typedef short I16;
typedef unsigned short U16;
typedef long I32;
typedef unsigned long U32;
typedef double F64;
typedef void VOID;


//定义一个仿真输入输出变量
struct ADPSS_Simu_IOVAR{
	I32 prono;		//变量相关计算进程号
	I32 VarNo;		//变量内部编号，参照UD输入输出量的定义。
	F64 kamplify;	//放大系数
	U16 idxIO;		//在计算进程输入输出变量中的下标位置
};

//定义一个接口输入输出变量
struct ADPSS_PID_IOVAR{
	I32 idno;								//在输入/输出变量中的序号
	std::vector<ADPSS_Simu_IOVAR> VSimIO;	//根据数值类型，可能与一个或过个仿真变量对应
	enum SignalType {AI,AO,DI,DO};
	U16 Nsigntype;							//信号类型
	U16 slotno;								//板卡槽位号
	U16 channelno;							//板卡通道号
	U16 iospec;								//信号设置
	
	//DIRECTVAL，无需变换，一一对应的变量
	//Virt2InstVal, 仿真侧是有效值的实数和虚数两个值，接口箱侧是瞬时值的变量
	//D01VAL，仿真侧是逻辑值，接口箱侧是电平或开关量
	//D01TIMEVAL，仿真侧是逻辑值和时标两个值，接口箱侧是包含逻辑和时标信息的双字节变量
	enum ValueType {DIRECTVAL=1,Virt2InstVal,D01VAL,D01TIMEVAL};
	U16 nValueType;							//数值类型，取值ValueType
	I16 idxVal2;							//对于同时存在两种类型IO的情况，指向第二种IO变量信息的存储下标。如瞬时值的幅值和相角
};

// 输出文件
extern FILE * fdbg;

#endif
