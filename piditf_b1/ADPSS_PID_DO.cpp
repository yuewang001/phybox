//#include "StdAfx.h"
#include "ADPSS_PID_DO.h"
#include "rtftoc_intel64.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_DO::CADPSS_PID_DO(void){
   data=NULL;
   DeviceId=9999;
}

CADPSS_PID_DO::CADPSS_PID_DO(const CADPSS_PID_DO &a)
{
   I32 n=a.VarInf.size();
   if (n>0)
   {
      allocBuf(n);

      memcpy(data,a.data,sizeof(I16)*n);

      VarInf = a.VarInf;
   // for (int i=0; i<n; i++) VarInf.push_back(a.VarInf[i]);
   }
   else
   {
      data=NULL;
   }
   DeviceId=a.DeviceId;
}

CADPSS_PID_DO::~CADPSS_PID_DO(void){
// if (size()>0) {
//    finalize();
// }
}

//增加一个仿真变量，返回>=0表示成功,<0失败
//Idxio:IO索引，当前变量下标位置距离在当前进程的第一个输入输出变量下标位置的偏差值，从0开始编号。
//Idno:编号，输入/输出量分别从1开始编号。
//ProcNo：处理本信号的接口进程号，从0开始编号；
//ValueType：数值类型。1，有效值；2，瞬时值。
//VarNo： 变量内部编号，参照UD输入输出量的定义。
//SlotNo：通道所在槽位号，从0开始编号。
//nChanNo：通道编号，从0开始编号。
//KAmplify：信号放大倍数,对于仿真系统输出，仿真系统出乘以该倍数后供给板卡输入。
I32 CADPSS_PID_DO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
                     I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
   I32 i;
   I32 nret;

   //查找变量是否已经存在
   for (i=0;i<VarInf.size();i++)
   {
      ADPSS_PID_IOVAR &a=VarInf[i];
      if (a.idno==idno)
      {  
         break;   
      }
   }

   ADPSS_Simu_IOVAR svar;
   svar.idxIO=idxio;
   svar.kamplify=KAmplify;
   svar.prono=ProcNo;
   svar.VarNo=VarNo;

   if (i<VarInf.size())
   {  //找到，在原有变量中新增一个仿真变量
      VarInf[i].VSimIO.push_back(svar);
      nret=i;
   }
   else
   {
      //新增一个变量
      ADPSS_PID_IOVAR var;
      var.idno=idno;
      var.VSimIO.push_back(svar);

      var.Nsigntype=ADPSS_PID_IOVAR::DO;
      var.slotno=SlotNo;
      var.channelno=nChanNo;
      var.nValueType=ValueType;

      if (ADPSS_PID_IOVAR::D01TIMEVAL==ValueType)
      {//预留
         //以后新增处理
      }
      else
      {
         //以后新增处理
      }

      VarInf.push_back(var);
      nret=VarInf.size();
   }
   
   fprintf(fdbg, "CADPSS_PID_DO::idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d\n", idxio, VarNo, SlotNo, nChanNo, ValueType);

   return nret;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DO::allocBuf()
{
   I32 n=size();

   if (n>0) 
   {
      data = new U8[n];

      if (!data)
      {
         fprintf(fdbg, "CADPSS_PID_DO::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//确定变量数目后，分配缓冲区存放data
I32 CADPSS_PID_DO::allocBuf(I32 nVar)
{
   I32 n = nVar;

   if (n>0) 
   {
      data = new U8[n];

      if (!data)
      {
         fprintf(fdbg, "CADPSS_PID_DO::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//释放缓冲区
I32 CADPSS_PID_DO::releaseBuf()
{
   if (data)
   {
      delete[] data;
      data=NULL;
   }

   return 0;
}

//确定变量后，初始化数据存储区，设置接口箱分组信息
//0表示成功，非0表示失败
I32 CADPSS_PID_DO::init(U16 id)
{
   I32 i, n=size();
   U16 slot_info[MAX_SLOTS], slot_flag[MAX_SLOTS];
   U8* grp_cfg;

   if (n>0)
   {
      grp_cfg = new U8[n];
      DeviceId=id;
      fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, SigNum=%d\n", id, n);

      // 逐槽读取板卡信息
      for (i=0; i<MAX_SLOTS; i++)
      {
         PID_Slot_ReadInfo(DeviceId, i, &slot_info[i], NULL, NULL);
      }

      // 逐个通道分析槽位是否有误，无误则进行分组配置，并记录校准数据
      memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
      for (i=0; i<n; i++)
      {
         ADPSS_PID_IOVAR &var=VarInf[i];
         if (slot_info[var.slotno] == CARD_INFO_DO)         // 该信号对应槽位的板卡为DO卡
         {
            grp_cfg[i] = var.slotno<<4 | var.channelno;     // 分组配置

            slot_flag[var.slotno] = 1;
         }
         else
         {
            delete[] grp_cfg;

            fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
            return -2;
         }
      }

      //配置运行模式PID_Slot_WriteConfig
      for (i=0; i<MAX_SLOTS; i++)
      {
         if (slot_flag[i] == 1)     // 该槽位为DO卡，且有通道被调用
            PID_Slot_WriteConfig(DeviceId, i, 0x0f00 | CARD_CFG_DO_SIM);
      }
   
      for (i=0; i<n; i++)
      {
         fprintf(fdbg, "grp_cfg=0x%02x\n", grp_cfg[i]);
      }
      //进行DO编组配置PID_DO_GrpConfig
      PID_DO_GrpConfig(DeviceId, 0, grp_cfg, n);

      delete[] grp_cfg;
      fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, return 0\n", id);
      return 0;
   }
   else
   {
      fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, return -1\n", id);
      return -1;
   }     
}

//根据指定进程仿真输入数据设置DO数据
//返回实际设置的数据数目
I32 CADPSS_PID_DO::GetSimuVal(I32 procno,F64 *Vin)
{
   I32 i, n=0;
   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      //根据data含义设置，
      //数值类型所需要的变换在这里处理
      if (a.VSimIO[0].prono==procno)
      {  //找到
         data[i]=(U8)Vin[a.VSimIO[0].idxIO]; //需要修改
         data[i]=data[i]<<7;              // 0x80代表1
         n++;
      }
   }
   return n;
}

//将数据data送至接口箱，对所有进程调用GetSimuVal后执行。
I32 CADPSS_PID_DO::SenddataToPID()
{
   I32 n=size();

   PID_DO_GrpWrite(DeviceId, 0, data, n);
   
   return 0;
}

//释放缓冲区，对象释放后自动调用
I32 CADPSS_PID_DO::finalize()
{
   releaseBuf();

   VarInf.clear();

   return 0;
}
