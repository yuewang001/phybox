#ifndef _ADPSS_PID_ITF_H
#define _ADPSS_PID_ITF_H
#include <vector>
#include "ADPSS_PID.h"

#define MAX_PID_NUM_PER_PROC	4		//每个接口进程最多支持的接口箱数目

//对外的接口类，提供对多个接口箱对外一致的管理
class CADPSS_PID_ITF
{
public:
	CADPSS_PID_ITF(void);
	~CADPSS_PID_ITF(void);

	//增加一个仿真变量，返回>=0表示成功,<0失败
	//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
	//Idno:编号，输入/输出量分别从1开始编号。
	//ProcNo：处理本信号的接口进程号，从0开始编号；
	//signType：信号类型。AI=0,AO,DI,DO
	//ValueType：数值类型。1，有效值；2，瞬时值。
	//VarNo： 变量内部编号，参照UD输入输出量的定义。
	//nPIDNo：接口箱号，从0开始编号。
	//SlotNo：通道所在槽位号，从0开始编号。
	//nChanNo：通道编号，从0开始编号。
	//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
				I32 VarNo,U16 nPIDNo,  U16 SlotNo, U16 nChanNo, F64 KAmplify, U16 IoSpec);

	//确定变量数目后，分配缓冲区空间
	I32 allocBuf();

	//CADPSS_PID对象必须先把内部信号push到VarInf中，然后才能被push到VPID中，否则std不支持
	//该函数将已经定义好的CADPSS_PID对象push到VPID中
	I32 pushPID();

	//确定变量后，初始化数据存储区
	//0表示成功，非0表示失败
	I32 init(F64 SyncDT);

	//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
	void initInstantPara(F64 dDT,F64 df0, I32 nkIT);

	//从接口箱取得数据，存储在data、Volt中。
	I32 getdatafromPID();

	//根据AI数据设置指定进程仿真输入数据
	//返回实际设置的数据数目
	I32 setSimuVal(I32 procno,F64 *Vin);

	//根据指定进程仿真输入数据设置AO数据
	//返回实际设置的数据数目
	I32 GetSimuVal(I32 procno,F64 *Vin);

	//更新瞬时值数值
	I32 updateInstantVal(I32 nt,I32 kit);

	//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
	I32 SenddataToPID();

	I32 ClearOutput();

	//释放缓冲区，对象释放后自动调用
	I32 finalize();

	//取得输入变量数目
	I32 getNumIn();

	//取得输出变量数目
	I32 getNumOut();

	//是否包含瞬时值信息，初始化后有效
	bool ishaveInstantInput();

	//MarkInput相关函数
	//查询是否更新
	I16 MarkInput_IsUpdated();

	//清除MarkInput标记
	I16 MarkInput_Clear();

	//等待MarkInput变化后返回
	I16 MarkInput_Wait(U16 Timeout);

	//停止同步信号
	I16 stopSync();

	//启动从站仿真
	I16 startSlaveSim();

	//启动主控仿真，启动主控应该启动所有从站之后
	I16 startMasterSim();

	//停止仿真
	I16 stopSim();

	//搜索主控接口箱，即最小序号接口箱，并设置其主控标记
	I16 setMaster();

private:
	//接口箱数组
	std::vector<CADPSS_PID> VPID;

	//主控接口箱在VPID中的编号
	I32 MasterNo;

	//CADPSS_PID对象指针的数组，根据使用需要进行分配
	CADPSS_PID *pid[MAX_PID_NUM_PER_PROC];
};
#endif
