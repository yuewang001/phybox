//#include "StdAfx.h"
#include <math.h>
#include "ADPSS_PID_AO.h"
#include "rtftoc_intel64.h"

using namespace std;
extern FILE* fdbg;

CADPSS_PID_AO::CADPSS_PID_AO(void){
   Slp=NULL;
   Itcp=NULL;
   data=NULL;
   Volt=NULL;
   DeviceId=9999;
   NumInstant=0;
}

CADPSS_PID_AO::CADPSS_PID_AO(const CADPSS_PID_AO &a)
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

      // for (int i=0; i<n; i++) VarInf.push_back(a.VarInf[i]);
   }
   else
   {
      Slp=NULL;
      Itcp=NULL;
      data=NULL;
      Volt=NULL;
   }
   DeviceId=a.DeviceId;
   NumInstant=a.NumInstant;
   if (NumInstant>0) {
      memcpy(VinstInA,a.VinstInA,sizeof(F64)*NumInstant);
      memcpy(VinstInP,a.VinstInP,sizeof(F64)*NumInstant);
      memcpy(VinstInAL,a.VinstInAL,sizeof(F64)*NumInstant);
      memcpy(VinstInPL,a.VinstInPL,sizeof(F64)*NumInstant);
      kIT=a.kIT;
      DT=a.DT;
      f0=a.f0;
   }
}

CADPSS_PID_AO::~CADPSS_PID_AO(void){
// if (size()>0) {
//    finalize();
// }
}

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���������ϵͳ�����Ըñ����󹩸��忨���롣
I32 CADPSS_PID_AO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
                     I32 VarNo, U16 SlotNo, U16 nChanNo,  F64 KAmplify)
{
   I32 i;
   I32 nret;

   //���ұ����Ƿ��Ѿ�����
   for (i=0;i<VarInf.size();i++)
   {
      ADPSS_PID_IOVAR &a=VarInf[i];
      if (a.idno==idno)
      {  //�ҵ�������������������Ӧһ���ź�
         break;   
      }
   }

   ADPSS_Simu_IOVAR svar;
   svar.idxIO=idxio;
   svar.kamplify=KAmplify;
   svar.prono=ProcNo;
   svar.VarNo=VarNo;

   if (i<VarInf.size())
   {  //�ҵ�����ԭ�б���������һ���������
      VarInf[i].VSimIO.push_back(svar);
      nret=i;
   }
   else
   {
      //����һ������
      ADPSS_PID_IOVAR var;
      var.idno=idno;
      var.VSimIO.push_back(svar);

      var.Nsigntype=ADPSS_PID_IOVAR::AO;
      var.slotno=SlotNo;
      var.channelno=nChanNo;
      var.nValueType=ValueType;

      if (ADPSS_PID_IOVAR::Virt2InstVal==ValueType) {
         var.idxVal2=NumInstant;
         NumInstant++;
      }
      else
      {
         var.idxVal2=-1;
      }

      VarInf.push_back(var);
      nret=VarInf.size();
   }
      
   fprintf(fdbg, "CADPSS_PID_AO::idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d, NumInstant=%d\n", idxio, VarNo, SlotNo, nChanNo, ValueType, NumInstant);

   return nret;
}

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
I32 CADPSS_PID_AO::allocBuf()
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
         fprintf(fdbg, "CADPSS_PID_AO::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
I32 CADPSS_PID_AO::allocBuf(I32 nVar)
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
         fprintf(fdbg, "CADPSS_PID_AO::allocBuf(), fail to allocate buffer, len = %d", n);
         return 1;
      }
   }

   return 0;
}

//�ͷŻ�����
I32 CADPSS_PID_AO::releaseBuf()
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

//ȷ�������󣬳�ʼ�����ݴ洢�������ýӿ��������Ϣ
//0��ʾ�ɹ�����0��ʾʧ��
I32 CADPSS_PID_AO::init(U16 id)
{
   I32 i, n=size();
   U16 slot_info[MAX_SLOTS], slot_flag[MAX_SLOTS];
   F64 All_Slp[MAX_SLOTS][MAX_AO_CHANS], All_Itcp[MAX_SLOTS][MAX_AO_CHANS];
   U8* grp_cfg;

   if (n>0)
   {
#ifdef ZX_DBG
      datalog=new F64[10000*10];
#endif
      grp_cfg = new U8[n];
      memset(Volt, 0, n*sizeof(F64));
      DeviceId=id;
      fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, SigNum=%d\n", id, n);

      // ��۶�ȡ�忨��Ϣ
      for (i=0; i<MAX_SLOTS; i++)
      {
         PID_Slot_ReadInfo(DeviceId, i, &slot_info[i], &All_Slp[i][0], &All_Itcp[i][0]);
      }

      // ���ͨ��������λ�Ƿ�������������з������ã�����¼У׼����
      memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
      for (i=0; i<n; i++)
      {
         ADPSS_PID_IOVAR &var=VarInf[i];
         if (slot_info[var.slotno] == CARD_INFO_AO)         // ���źŶ�Ӧ��λ�İ忨ΪAO��
         {
            grp_cfg[i] = var.slotno<<4 | var.channelno;     // ��������
            // ��ȡУ׼����
            Slp[i] = All_Slp[var.slotno][var.channelno];
            Itcp[i] = All_Itcp[var.slotno][var.channelno];

            slot_flag[var.slotno] = 1;
         }
         else
         {
            delete[] grp_cfg;
#ifdef ZX_DBG
            delete[] datalog;
#endif

            fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
            return -2;
         }
      }

      //��������ģʽPID_Slot_WriteConfig
      for (i=0; i<MAX_SLOTS; i++)
      {
         if (slot_flag[i] == 1)     // �ò�λΪAO��������ͨ��������
            PID_Slot_WriteConfig(DeviceId, i, 0x0f00 | CARD_CFG_AO_SIM);
      }

      for (i=0; i<n; i++)
      {
         fprintf(fdbg, "grp_cfg=0x%02x, Slp=%10.6f, Itcp=%10.6f\n", grp_cfg[i], Slp[i], Itcp[i]);
      }
      //����AO��������PID_AO_GrpConfig
      PID_AO_GrpConfig(DeviceId, 0, grp_cfg, n);

      delete[] grp_cfg;
      fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, return 0\n", id);
      return 0;
   }
   else
   {
      fprintf(fdbg, "CADPSS_PID_AO::init(), PIDNo=%d, return -1\n", id);
      return -1;
   }
}

//��ʼ����Чֵת˲ʱֵ��ر�������������ת������ʱ����
//DT ������f0 Ƶ�ʣ�kIT��ÿ���󲽳��е�С������Ŀ
void CADPSS_PID_AO::initInstantPara(F64 dDT,F64 df0, I32 nkIT)
{
   DT=dDT;
   f0=df0;
   kIT=nkIT;
   if (NumInstant>0)
   {
      VinstInA=new F64[NumInstant];
      VinstInP=new F64[NumInstant];
      VinstInAL=new F64[NumInstant];
      VinstInPL=new F64[NumInstant];
      memset(VinstInA,0,NumInstant*sizeof(F64));
      memset(VinstInP,0,NumInstant*sizeof(F64));
      memset(VinstInAL,0,NumInstant*sizeof(F64));
      memset(VinstInPL,0,NumInstant*sizeof(F64));
   }
}

//����ָ�����̷���������������AO����
//����ʵ�����õ�������Ŀ
I32 CADPSS_PID_AO::GetSimuVal(I32 procno,F64 *Vin)
{
   I32 i,j,n=0;

   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      for (j=0; j<a.VSimIO.size(); j++)
      {
         ADPSS_Simu_IOVAR& b=a.VSimIO[j];
         if (b.prono==procno) //�ҵ�
         {
            if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)   //˲ʱֵ
            {
               if (1==j)   //�鲿ֵ
               {
                  F64 r,x;
                  r=Vin[a.VSimIO[0].idxIO];
                  x=Vin[b.idxIO];
                  I32 idx=a.idxVal2;
                  VinstInAL[idx]=VinstInA[idx];
                  VinstInPL[idx]=VinstInP[idx];
                  VinstInA[idx]=b.kamplify*sqrt((r*r+x*x)*2);  //ת��Ϊ��ֵ��A**0.5
                  VinstInP[idx]=atan2(x,r);
               }
            }
            else
            {  //��Чֵ����������ֵ�ۼ�
               if (0==j)
               {
                  Volt[i]=b.kamplify*Vin[b.idxIO];
               }
               else
               {
                  Volt[i]+=b.kamplify*Vin[b.idxIO];
               }
            }
            n++;
         }
      }
   }

   return n;
}

//����˲ʱֵ��ֵ
//nt��������̬���沽��
//knt������ӿ�����������󲽳��ڣ�
I32 CADPSS_PID_AO::updateInstantVal(I32 nt,I32 knt)
{
   I32 i, n=0;
   F64 wt, kperc;
   const F64 pi=3.1415926;

   kperc=knt/(kIT*1.0);
   wt=2*pi*f0*(nt+kperc)*DT;

   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      if (ADPSS_PID_IOVAR::Virt2InstVal==a.nValueType)   //˲ʱֵ
      {
         I32 idx=a.idxVal2;
         Volt[i]=VinstInAL[idx]*cos(wt+VinstInPL[idx])*(1-kperc)+VinstInA[idx]*cos(wt+VinstInP[idx])*kperc;
         n++;
      }
   }
#ifdef ZX_DBG
   datalog[nt*100+knt] = Volt[0];
#endif

   return n;
}

//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
I32 CADPSS_PID_AO::SenddataToPID()
{
   I32 i, n=size();

   PID_AO_VoltScale(Volt, Slp, Itcp, data, n);

   PID_AO_GrpWrite(DeviceId, 0, data, n);

   return 0;
}

I32 CADPSS_PID_AO::ClearOutput()
{
   I32 i, n=size();

   for (i=0; i<n; i++) Volt[i]=0;
   PID_AO_VoltScale(Volt, Slp, Itcp, data, n);
   PID_AO_GrpWrite(DeviceId, 0, data, n);

   return 0;
}

//�ͷŻ������������ͷź��Զ�����
I32 CADPSS_PID_AO::finalize(){
#ifdef ZX_DBG
   FILE* log;
   log = fopen("DATALOG.log","w");
   for (int i=0; i<10*10000; i++) fprintf(log, "%f\n", datalog[i]);
   fclose(log);
#endif

#ifdef ZX_DBG
   if (datalog) {
      delete[] datalog;
   }
#endif

   releaseBuf();

   VarInf.clear();

   if (NumInstant>0) {
      delete []VinstInA;
      delete []VinstInP;
      delete []VinstInAL;
      delete []VinstInPL;
      NumInstant=0;
   }
   return 0;
}
