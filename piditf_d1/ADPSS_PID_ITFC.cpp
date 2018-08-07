#include "stdafx.h"
#include "ADPSS_PID_ITFC.h"
CADPSS_PID_ITF gPIDITF;
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
int PID_ITF_addavar(int idxio,int idno,int ProcNo,int signType, int ValueType, 
					int VarNo,U16 nPIDNo,  U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
	return gPIDITF.addavar(idxio,idno,ProcNo,signType,ValueType,
		VarNo,nPIDNo,SlotNo,nChanNo,KAmplify);
}
//确定变量后，初始化数据存储区
//0表示成功，非0表示失败
int PID_ITF_init(U32 SyncFreq){
	return gPIDITF.init(SyncFreq);
}
//DT 步长；f0 频率；kIT，每个大步长中的小步长数目
void PID_ITF_initInstantPara(double dDT,double df0, int nkIT){
	return gPIDITF.initInstantPara(dDT,df0,nkIT);
}
//从接口箱取得数据，存储在data、Volt中。
int PID_ITF_getdatafromPID(){
	return gPIDITF.getdatafromPID();
}
//根据AI数据设置指定进程仿真输入数据
//返回实际设置的数据数目
int PID_ITF_setSimuVal(int procno,F64 *Vin){
	return gPIDITF.setSimuVal(procno,Vin);
}
//根据指定进程仿真输入数据设置AO数据
//返回实际设置的数据数目
int PID_ITF_GetSimuVal(int procno,F64 *Vin){
	return gPIDITF.GetSimuVal(procno,Vin);
}
//更新瞬时值数值
int PID_ITF_updateInstantVal(int nt,int kit){
	return gPIDITF.updateInstantVal(nt,kit);
}
//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
int PID_ITF_SenddataToPID(){
	return gPIDITF.SenddataToPID();
}
//释放缓冲区，对象释放后自动调用
int PID_ITF_finalize(){
	return gPIDITF.finalize();
}
//取得输入变量数目
int PID_ITF_getNumIn(){
	return gPIDITF.getNumIn();
}
//取得输出变量数目
int PID_ITF_getNumOut(){
	return gPIDITF.getNumOut();
}
//是否包含瞬时值信息，初始化后有效
bool PID_ITF_ishaveInstantInput(){
	return gPIDITF.ishaveInstantInput();
}
//MarkInput相关函数
//查询是否更新
I16 PID_ITF_MarkInput_IsUpdated(){
	return gPIDITF.MarkInput_IsUpdated();
}
//清除MarkInput标记
I16 PID_ITF_MarkInput_Clear(){
	return gPIDITF.MarkInput_Clear();
}
//等待MarkInput变化后返回
I16 PID_ITF_MarkInput_Wait(U16 Timeout){
	return gPIDITF.MarkInput_Wait(Timeout);
}
