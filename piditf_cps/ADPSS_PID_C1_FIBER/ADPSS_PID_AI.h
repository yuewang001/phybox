#ifndef _ADPSS_PID_AI_H
#define _ADPSS_PID_AI_H
#include <vector>
#include "ADPSS_PID_Base.h"

using namespace std;

//接口箱AI管理类
class CADPSS_PID_AI
{
public:
	CADPSS_PID_AI(void);
	CADPSS_PID_AI(const CADPSS_PID_AI &a);
	~CADPSS_PID_AI(void);

	//增加一个仿真变量，返回>=0表示成功,<0失败
	//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
	//Idno:编号，输入/输出量分别从1开始编号。
	//ProcNo：处理本信号的计算进程号，从0开始编号；
	//ValueType：数值类型。1，有效值；2，瞬时值。
	//VarNo： 变量内部编号，参照UD输入输出量的定义。
	//SlotNo：通道所在槽位号，从0开始编号。
	//nChanNo：通道编号，从0开始编号。
	//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
	I32 addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType,
			    I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify);

	//确定变量后，初始化数据存储区
	//0表示成功，非0表示失败
	I32 init(U16 id);

	//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
	I32 allocBuf();

	//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
	I32 allocBuf(I32 nVar);

	//释放缓冲区
	I32 releaseBuf();

	//从接口箱取得数据，存储在data、Volt中。
	I32 getdatafromPID();

	//根据AI数据设置指定进程仿真输入数据
	//返回实际设置的数据数目
	I32 setSimuVal(I32 procno,double *Vin);

	//取得变量数目
	I32 size(){return VarInf.size();};

	//释放缓冲区，对象释放后自动调用
	I32 finalize();

	I32 setSyncDT(F64 DT);

private:
	//通道信息
	std::vector<ADPSS_PID_IOVAR> VarInf;

	//设备编号，用于设备相关函数调用
	U16 DeviceId;

	//Slp：	通道系数，数组中每一个元素表示一个AI通道的系数。
	//Itcp：通道零偏，数组中每一个元素表示一个AI通道的零偏。
	F64 *Slp,*Itcp;

	//原始数值
	U16 * data;

	//转换后数值
	F64 * Volt;

	F64 syncDT;
};
#endif
