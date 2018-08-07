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

//����һ���������������>=0��ʾ�ɹ�,<0ʧ��
//Idxio:IO��������ǰ�����±�λ�þ����ڵ�ǰ���̵ĵ�һ��������������±�λ�õ�ƫ��ֵ����0��ʼ��š�
//Idno:��ţ�����/������ֱ��1��ʼ��š�
//ProcNo�������źŵĽӿڽ��̺ţ���0��ʼ��ţ�
//ValueType����ֵ���͡�1����Чֵ��2��˲ʱֵ��
//VarNo�� �����ڲ���ţ�����UD����������Ķ��塣
//SlotNo��ͨ�����ڲ�λ�ţ���0��ʼ��š�
//nChanNo��ͨ����ţ���0��ʼ��š�
//KAmplify���źŷŴ���,���ڷ���ϵͳ���������ϵͳ�����Ըñ����󹩸��忨���롣
I32 CADPSS_PID_DO::addavar(I32 idxio,I32 idno,I32 ProcNo, I32 ValueType, 
                     I32 VarNo, U16 SlotNo, U16 nChanNo, F64 KAmplify)
{
   I32 i;
   I32 nret;

   //���ұ����Ƿ��Ѿ�����
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

      var.Nsigntype=ADPSS_PID_IOVAR::DO;
      var.slotno=SlotNo;
      var.channelno=nChanNo;
      var.nValueType=ValueType;

      if (ADPSS_PID_IOVAR::D01TIMEVAL==ValueType)
      {//Ԥ��
         //�Ժ���������
      }
      else
      {
         //�Ժ���������
      }

      VarInf.push_back(var);
      nret=VarInf.size();
   }
   
   fprintf(fdbg, "CADPSS_PID_DO::idxio=%d, varno=%d, slotno=%d, ch=%d, valuetype=%d\n", idxio, VarNo, SlotNo, nChanNo, ValueType);

   return nret;
}

//ȷ��������Ŀ�󣬷��仺�������data
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

//ȷ��������Ŀ�󣬷��仺�������data
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

//�ͷŻ�����
I32 CADPSS_PID_DO::releaseBuf()
{
   if (data)
   {
      delete[] data;
      data=NULL;
   }

   return 0;
}

//ȷ�������󣬳�ʼ�����ݴ洢�������ýӿ��������Ϣ
//0��ʾ�ɹ�����0��ʾʧ��
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

      // ��۶�ȡ�忨��Ϣ
      for (i=0; i<MAX_SLOTS; i++)
      {
         PID_Slot_ReadInfo(DeviceId, i, &slot_info[i], NULL, NULL);
      }

      // ���ͨ��������λ�Ƿ�������������з������ã�����¼У׼����
      memset(slot_flag, 0, MAX_SLOTS*sizeof(U16));
      for (i=0; i<n; i++)
      {
         ADPSS_PID_IOVAR &var=VarInf[i];
         if (slot_info[var.slotno] == CARD_INFO_DO)         // ���źŶ�Ӧ��λ�İ忨ΪDO��
         {
            grp_cfg[i] = var.slotno<<4 | var.channelno;     // ��������

            slot_flag[var.slotno] = 1;
         }
         else
         {
            delete[] grp_cfg;

            fprintf(fdbg, "CADPSS_PID_DO::init(), PIDNo=%d, Slot=%d, CARDType=%d, return -2\n", id, var.slotno, slot_info[var.slotno]);
            return -2;
         }
      }

      //��������ģʽPID_Slot_WriteConfig
      for (i=0; i<MAX_SLOTS; i++)
      {
         if (slot_flag[i] == 1)     // �ò�λΪDO��������ͨ��������
            PID_Slot_WriteConfig(DeviceId, i, 0x0f00 | CARD_CFG_DO_SIM);
      }
   
      for (i=0; i<n; i++)
      {
         fprintf(fdbg, "grp_cfg=0x%02x\n", grp_cfg[i]);
      }
      //����DO��������PID_DO_GrpConfig
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

//����ָ�����̷���������������DO����
//����ʵ�����õ�������Ŀ
I32 CADPSS_PID_DO::GetSimuVal(I32 procno,F64 *Vin)
{
   I32 i, n=0;
   for (i=0; i<VarInf.size(); i++)
   {
      ADPSS_PID_IOVAR& a=VarInf[i];
      //����data�������ã�
      //��ֵ��������Ҫ�ı任�����ﴦ��
      if (a.VSimIO[0].prono==procno)
      {  //�ҵ�
         data[i]=(U8)Vin[a.VSimIO[0].idxIO]; //��Ҫ�޸�
         data[i]=data[i]<<7;              // 0x80����1
         n++;
      }
   }
   return n;
}

//������data�����ӿ��䣬�����н��̵���GetSimuVal��ִ�С�
I32 CADPSS_PID_DO::SenddataToPID()
{
   I32 n=size();

   PID_DO_GrpWrite(DeviceId, 0, data, n);
   
   return 0;
}

//�ͷŻ������������ͷź��Զ�����
I32 CADPSS_PID_DO::finalize()
{
   releaseBuf();

   VarInf.clear();

   return 0;
}
