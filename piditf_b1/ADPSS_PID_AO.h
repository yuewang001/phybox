#ifndef _ADPSS_PID_AO_H
#define _ADPSS_PID_AO_H
#include <vector>
#include "ADPSS_PID_Base.h"

//接口箱AO管理类
class CADPSS_PID_AO
{
public:
	CADPSS_PID_AO(void);
	CADPSS_PID_AO(const CADPSS_PID_AO &a);
	~CADPSS_PID_AO(void);

	//增加一个仿真变量，返回>=0表示成功,<0失败
	//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
	//Idno:编号，输入/输出量分别从1开始编号。
	//ProcNo：处理本信号的接口进程号，从0开始编号；
	//ValueType：数值类型。1，有效值；2，瞬时值。
	//VarNo： 变量内部编号，参照UD输入输出量的定义。
	//SlotNo：通道所在槽位号，从0开始编号。
	//nChanNo：通道编号，从0开始编号。
	//KAmplify：信号放大倍数,对于仿真系统输出，仿真系统出乘以该倍数后供给板卡输入。
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
		        I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//确定变量后，初始化数据存储区，设置接口箱分组信息
	//0表示成功，非0表示失败
	I32 init(U16 id);

	//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
	I32 allocBuf();

	//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
	I32 allocBuf(I32 nVar);

	//释放缓冲区
	I32 releaseBuf();

	//初始化有效值转瞬时值相关变量，在有这种转换需求时调用
	//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
	void initInstantPara(F64 dDT, F64 df0, I32 nkIT);

	//根据指定进程仿真输入数据设置AO数据
	//返回实际设置的数据数目
	I32 GetSimuVal(I32 procno,F64 *Vin);

	//更新瞬时值数值
	I32 updateInstantVal(I32 nt,I32 kit);

	//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
	I32 SenddataToPID();

	I32 ClearOutput();

	//取得变量数目
	I32 size(){return VarInf.size();};

	//释放缓冲区，对象释放后自动调用
	I32 finalize();

	//是否包含瞬时值信息，初始化后有效
	bool ishaveInstantInput(){return (NumInstant>0);};

private:
	//设备编号，用于设备相关函数调用
	U16 DeviceId;

	//通道信息
	std::vector<ADPSS_PID_IOVAR> VarInf;

	//Slp：	通道系数，数组中每一个元素表示一个AO通道的系数。
	//Itcp：通道零偏，数组中每一个元素表示一个AO通道的零偏。
	F64 *Slp,*Itcp;

	//原始数值
	U16 * data;

	//转换后数值
	F64 * Volt;

#ifdef ZX_DBG
	F64 * datalog;
#endif

	//瞬时值变换相关信息，在初始化时设置
	I32 NumInstant;	//瞬时值数目

	//本次和上次的瞬时值输入量的幅值和角度
	F64 *VinstInA,*VinstInP,*VinstInAL,*VinstInPL;

	//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
	I32 kIT;
	F64 DT,f0;
};
#endif
