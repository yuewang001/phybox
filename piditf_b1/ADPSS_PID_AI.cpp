//#include "StdAfx.h"
#include "ADPSS_PID_AI.h"
#include "rtftoc_intel64.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_AI::CADPSS_PID_AI(void){
   Slp=NULL;
   Itcp=NULL;
   data=NULL;
   Volt=NULL;
   DeviceId=9999;
}

CADPSS_PID_AI::CADPSS_PID_AI(const CADPSS_PID_AI &a)
{
   I32 n=a.VarInf.size();
   if (n>0) 
   {
      allocBuf(n);

      memcpy(Slp,a.Slp,sizeof(F64)*n);
      memcpy(Itcp,a.Itcp,sizeof(F64)*n);
      memcpy(data,a.data,sizeof(I16)*n);
      memcpy(Volt,a.Volt,sizeof(F64)*n);

      VarInf = a.VarInf;
      
//    for (int i=0; i<n; i++) VarInf.push_back(a.VarInf[i]);
   }
   else
   {
      Slp=NULL;
      Itcp=NULL;
      data=NULL;
      Volt=NULL;
   }
   DeviceId=a.DeviceId;
}

CADPSS_PID_AI::~CADPSS_PID_AI(void){
// if (size()>0) {
//    finalize();
// }
}

//增加一个仿真变量，返回>=0表示成功,返回增加后的变量总数；<0失败
//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的计算进程号，从0开始编号；
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输入，板卡输入乘以该倍数后供给仿真系统。
I32 CADPSS_PID_AI::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
                     I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
   I32 i;
   I32 nret;

   //查找变量是否已经存在
   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR &a=VarInf[i];
      if (a.idno==idno)
      {  //找到，不应该出现这种情况
         break;   
      }
   }

   if (i<VarInf.size())
   {  //错误
      nret = -1;
   }
   else
   {
      //新增一个变量
      ADPSS_PID_IOVAR var;
      var.idno=idno;
      var.VSimIO.resize(1);
      var.VSimIO[0].prono=ProcNo;
      var.VSimIO[0].VarNo=VarNo;
      var.VSimIO[0].kamplify=KAmplify;
      var.VSimIO[0].idxIO=idxio;
      var.Nsigntype=ADPSS_PID_IOVAR::AI;
      var.slotno=SlotNo;
      var.channelno=nChanNo;
      var.nValueType=ValueType;
      var.idxVal2=-1;
      VarInf.push_back(var);
      nret = VarInf.size();
   }
      
   fprintf(fdbg, "CADPSS_PID_AI::idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d\n", idxio, VarNo, SlotNo, nChanNo, ValueType);

   return nret;
}

//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
I32 CADPSS_PID_AI::allocBuf()
{
   I32 n=size();

   if (n>0) 
   {
      Slp = new F64[n];
      Itcp = new F64[n];
      data = new U16[n];
      Volt = new F64[n];

      if (!Slp || !Itcp || !data || !Volt)
      {
         fprintf(fdbg, "CADPSS_PID_AI::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//确定变量数目后，分配缓冲区存放Slp、Itcp、data、Volt
I32 CADPSS_PID_AI::allocBuf(I32 nVar)
{
   I32 n = nVar;

   if (n>0) 
   {
      Slp = new F64[n];
      Itcp = new F64[n];
      data = new U16[n];
      Volt = new F64[n];

      if (!Slp || !Itcp || !data || !Volt)
      {
         fprintf(fdbg, "CADPSS_PID_AI::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//释放缓冲区
I32 CADPSS_PID_AI::releaseBuf()
{
   if (Slp)
   {
      delete[] Slp;
      Slp=NULL;
   }
   if (Itcp)
   {
      delete[] Itcp;
      Itcp=NULL;
   }
   if (data)
   {
      delete[] data;
      data=NULL;
   }
   if (Volt)
   {
      delete[] Volt;
      Volt=NULL;
   }

   return 0;
}

//确定变量后，初始化数据存储区
//0表示成功，非0表示失败
I32 CADPSS_PID_AI::init(U16 id)
{
   I32 i, j,  n=size();
   U16 slot_info[MAX_SLOTS], slot_flag[MAX_SLOTS];
   F64 All_Slp[MAX_SLOTS][MAX_AI_CHANS], All_Itcp[MAX_SLOTS][MAX_AI_CHANS];
   U8* grp_cfg;
   
   if (n>0)
   {
      grp_cfg = new U8[n];
      memset(Volt, 0, n*sizeof(F64));
      DeviceId=id;
      fprintf(fdbg, "CADPSS_PID_AI::init(), PIDNo=%d, SigNum=%d\n", id, n);

      // 逐槽读取板卡信息
      for (i=0; i<MAX_SLOTS; i++)
      {
         PID_Slot_ReadInfo(DeviceId, i, &slot_info[i], &All_Slp[i][0], &All_Itcp[i][0]);
      }

      // 逐个通道分析槽位是否有误，无误则进行分组配置，并记录校准数据
      memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
      for (i=0; i<n; i++)
      {
         ADPSS_PID_IOVAR &var=VarInf[i];
         if (slot_info[var.slotno] == CARD_INFO_AI)         // 该信号对应槽位的板卡为AI卡
         {
            grp_cfg[i] = var.slotno<<4 | var.channelno;     // 分组配置

            // 读取校准数据
            Slp[i] = All_Slp[var.slotno][var.channelno];
            Itcp[i] = All_Itcp[var.slotno][var.channelno];

            slot_flag[var.slotno] = 1;
         }
         else
         {
            delete[] grp_cfg;

            fprintf(fdbg, "CADPSS_PID_AI::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
            return -2;
         }
      }

      //配置运行模式PID_Slot_WriteConfig
      for (i=0; i<MAX_SLOTS; i++)
      {
         if (slot_flag[i] == 1)     // 该槽位为AI卡，且有通道被调用
            PID_Slot_WriteConfig(DeviceId, i, 0x0f00 | CARD_CFG_AI_SIM);
      }
   
      for (i=0; i<n; i++)
      {
         fprintf(fdbg, "grp_cfg=0x%02x, Slp=%10.6f, Itcp=%10.6f\n", grp_cfg[i], Slp[i], Itcp[i]);
      }  
      //进行AI编组配置PID_AI_GrpConfig
      PID_AI_GrpConfig(DeviceId, 0, grp_cfg, n);

      delete[] grp_cfg;
      fprintf(fdbg, "CADPSS_PID_AI::init(), PIDNo=%d, return 0\n", id);
      return 0;
   }
   else
   {
      fprintf(fdbg, "CADPSS_PID_AI::init(), PIDNo=%d, return -1\n", id);
      return -1;
   }
}

//从接口箱取得数据，存储在data、Volt中
I32 CADPSS_PID_AI::getdatafromPID()
{
   I32 n=size();

   // 读AI数据
   PID_AI_GrpRead(DeviceId, 0, data, n);

   // 将采样数据转换为电压值
   PID_AI_VoltScale(data, Slp, Itcp, Volt, n);

   return 0;
}

//根据AI数据设置指定进程仿真输入数据
//返回实际设置的数据数目
I32 CADPSS_PID_AI::setSimuVal(I32 procno,F64 *Vin)
{
   I32 i, n=0;

   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      if (a.VSimIO[0].prono==procno)
      {  //找到
         Vin[a.VSimIO[0].idxIO]=a.VSimIO[0].kamplify*Volt[i];
         n++;
      }
   }
   return n;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_AI::finalize()
{
   releaseBuf();

   VarInf.clear();

   return 0;
}
