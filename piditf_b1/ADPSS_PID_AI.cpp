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

//����һ���������������>=0��ʾ�ɹ�,�������Ӻ�ı���������<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵļ�����̺ţ���0��ʼ��ţ�
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���룬�忨������Ըñ����󹩸�����ϵͳ��
I32 CADPSS_PID_AI::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
                     I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
   I32 i;
   I32 nret;

   //���ұ����Ƿ��Ѿ�����
   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR &a=VarInf[i];
      if (a.idno==idno)
      {  //�ҵ�����Ӧ�ó����������
         break;   
      }
   }

   if (i<VarInf.size())
   {  //����
      nret = -1;
   }
   else
   {
      //����һ������
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

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
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

//ȷ��������Ŀ�󣬷��仺�������Slp��Itcp��data��Volt
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

//�ͷŻ�����
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

//ȷ�������󣬳�ʼ�����ݴ洢��
//0��ʾ�ɹ�����0��ʾʧ��
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
         if (slot_info[var.slotno] == CARD_INFO_AI)         // ���źŶ�Ӧ��λ�İ忨ΪAI��
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

            fprintf(fdbg, "CADPSS_PID_AI::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
            return -2;
         }
      }

      //��������ģʽPID_Slot_WriteConfig
      for (i=0; i<MAX_SLOTS; i++)
      {
         if (slot_flag[i] == 1)     // �ò�λΪAI��������ͨ��������
            PID_Slot_WriteConfig(DeviceId, i, 0x0f00 | CARD_CFG_AI_SIM);
      }
   
      for (i=0; i<n; i++)
      {
         fprintf(fdbg, "grp_cfg=0x%02x, Slp=%10.6f, Itcp=%10.6f\n", grp_cfg[i], Slp[i], Itcp[i]);
      }  
      //����AI��������PID_AI_GrpConfig
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

//�ӽӿ���ȡ�����ݣ��洢��data��Volt��
I32 CADPSS_PID_AI::getdatafromPID()
{
   I32 n=size();

   // ��AI����
   PID_AI_GrpRead(DeviceId, 0, data, n);

   // ����������ת��Ϊ��ѹֵ
   PID_AI_VoltScale(data, Slp, Itcp, Volt, n);

   return 0;
}

//����AI��������ָ�����̷�����������
//����ʵ�����õ�������Ŀ
I32 CADPSS_PID_AI::setSimuVal(I32 procno,F64 *Vin)
{
   I32 i, n=0;

   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      if (a.VSimIO[0].prono==procno)
      {  //�ҵ�
         Vin[a.VSimIO[0].idxIO]=a.VSimIO[0].kamplify*Volt[i];
         n++;
      }
   }
   return n;
}

//�ͷŻ������������ͷź��Զ�����
I32 CADPSS_PID_AI::finalize()
{
   releaseBuf();

   VarInf.clear();

   return 0;
}
