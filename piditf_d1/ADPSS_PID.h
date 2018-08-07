#ifndef _ADPSS_PID_H
#define _ADPSS_PID_H
#include "ADPSS_PID_AI.h"
#include "ADPSS_PID_AO.h"
#include "ADPSS_PID_DI.h"
#include "ADPSS_PID_DO.h"

class CADPSS_PID
{
public:
	CADPSS_PID(U16 nPIDNo);
	CADPSS_PID(const CADPSS_PID &a);
	~CADPSS_PID(void);

	//增加一个仿真变量，返回>=0表示成功,<0失败
	//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
	//Idno:编号，输入/输出量分别从1开始编号。
	//ProcNo：处理本信号的接口进程号，从0开始编号；
	//signType：信号类型。AI=0,AO,DI,DO
	//ValueType：数值类型。1，有效值；2，瞬时值。
	//VarNo： 变量内部编号，参照UD输入输出量的定义。
	//SlotNo：通道所在槽位号，从0开始编号。
	//nChanNo：通道编号，从0开始编号。
	//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo,I32 signType, I32 ValueType, 
				I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//确定变量后，初始化数据存储区
	//0表示成功，非0表示失败
	I32 init(F64 SyncDT);

	//确定变量数目后，分配缓冲区
	I32 allocBuf();

	//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
	void initInstantPara(F64 dDT,F64 df0, I32 nkIT);

	//从接口箱取得数据，存储在data、Volt中。
	//inline I32 getdatafromPID();
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
	I32 SenddataToPID(U16 DeviceId);

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

	//查询接口箱编号
	I32 getPIDNo() {return PIDNo;};

    //设置主控状态
    bool setMaster() {isMaster=true;};

	//查询主控状态
	bool getisMaster() {return isMaster;};

    //停止同步信号
    I16 stopSync();

	//启动仿真
	I16 startSim();

	//停止仿真
	I16 stopSim();

private:
	//输入/输出变量
	CADPSS_PID_AI vai;
	CADPSS_PID_AO vao;
	CADPSS_PID_DI vdi;
	CADPSS_PID_DO vdo;

	//接口箱编号
	I32 PIDNo;

	//是否主控
	bool isMaster;

	//同步频率
	U32 syncFreq;

	//同步周期
	F64 syncDT;
};
#endif
