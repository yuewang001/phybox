// PstPIDItf.cpp : �������̨Ӧ�ó������ڵ㡣
//

//#include "stdafx.h"
#include "mpi.h"
#include <stdio.h>
#include <algorithm>
#include "PstPIDITF.h"
#include "math.h"
#include <sys/rt_types.h>
#include <sys/rt_stat.h>
#include <rt_fcntl.h>
#include <rt_stdio.h>
#include "rtftoc_intel64.h"

//MPI��ʶ
struct SMPIID mpiid;
//����װ����ؽ���
struct SPROCPHY procphy;
// ����ļ�
FILE * fdbg;
//ʵʱ���
struct SPhyRTIO phyrtio;
// ����ϵͳ�����������������
struct SNETIO simio;
//ȫ�ֽӿڶ���
CADPSS_PID_ITF gPIDITF;
//˲ʱֵ����
const double DTINST=(1.0e-4*2);   //100us
//�����շ�����  xdc added 2013.4.2
double *DataSendRecv=NULL;

#ifdef ZX_DBG2
   double *timestamp_init;
   double *timestamp;
   #define TS_LEN 1000*100
#endif

//��ʼ���Ĵ���
enum {PhyInit_Start,PhyInit_Ok,PhyInit_Stop,PhyInit_Over} MarkPhyInit;
int IsBalance=0;
double BarrierT;

int main(int argc,char * argv[])
{
   int NT;   
   int k,kNT;      
   int i,Nin,Nout;
   MPI_Status mpistat;

   //1.��ʼ������װ�ý���ʵʱ����
   rtphyinit();
   
   MPI_Init(&argc,&argv);
   MPI_Comm_size(MPI_COMM_WORLD,&mpiid.numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&mpiid.myid);
   //ȡ���Ľ��̺�,Ϊ�����С����ӿڽ��̺���׼��
   mpiid.minPIDProc=mpiid.numprocs;
   
   char fname[32];
   char str_dbg[256];
   sprintf(fname,"PhyERR%d.LIS",mpiid.myid+1);
   fdbg=fopen(fname,"w");
   if (NULL==fdbg) {
      printf("Fail to open file %s",fname);
      return 1;
   }
   sprintf(str_dbg,"PhyST Process %d of %d start to run...\n",
               mpiid.myid, mpiid.numprocs);
   fputs(str_dbg,fdbg);
   fputs(str_dbg,stdout);
   fflush(fdbg);
   
   //�γ�һЩ����ͨ����
   int Ierr=0;  
   Ierr=FormComm();
  
   //�����ؽ���IO��Ϣ�����ýӿ����ݣ���������������ص�IO����
   Ierr=RecvSendPhyIO();
   fprintf(fdbg,"After RecvSendPhyIO, return %d .\n",Ierr);
   fflush(fdbg);
   
   //ΪCADPSS_PID������仺�����ռ䣬������push��gPID.VPID��
   gPIDITF.allocBuf();
   gPIDITF.pushPID();
   
   //������СPID���̵���С��Žӿ���Ϊ����
   fprintf(fdbg, "minPIDProc = %d\n", mpiid.minPIDProc);
   fflush(fdbg);
   
   //������СPID���̵���С��Žӿ���Ϊ����
   if(mpiid.myid==mpiid.minPIDProc) 
      gPIDITF.setMaster();
   
   //����ӿڳ�ʼ��
   if (gPIDITF.ishaveInstantInput())
   {
      fprintf(fdbg,"ishaveInstantInput() return TRUE.\n");
      //�������������ӿ���ķ��沽����������
      Ierr=gPIDITF.init(simio.dt/simio.Ikt);
      gPIDITF.initInstantPara(simio.dt,simio.fhz,simio.Ikt);
   }
   else
   {
      fprintf(fdbg,"ishaveInstantInput() return FALSE.\n");
      //�������������ӿ���ķ��沽����������
      Ierr=gPIDITF.init(simio.dt);
   }
   
   if (Ierr!=0) 
      fprintf(fdbg,"Fail to initialize Physical device,ErrorNo= %d.\n",Ierr);
   else
      fprintf(fdbg,"Succeed to initialize Physical device.\n");
   fflush(fdbg);
   
   if (Ierr<0) Ierr=-Ierr;
   SubDealParaErr(Ierr);
   fprintf(fdbg,"After SubDealParaErr,Ierr=%d \n ",Ierr);
   fflush(fdbg);
   
   if (Ierr!=0) goto LabelEND;
   
   //������ʼ��ʵʱ����
   RTDevOpenClose(1);
    
   //��ʼ����
   //ֹͣ���нӿ����ͬ���ź�
   gPIDITF.stopSync();
   
   //���д�վת��������״̬
   gPIDITF.startSlaveSim(); 
   
   fprintf(fdbg,"Before create rtl_thread RTSTPhyInit.\n");
   fflush(fdbg);
   
   MarkPhyInit=PhyInit_Start;
   
   printf("Before create_rt_thread:MarkPhyInit=%d\n",MarkPhyInit); 
   create_rt_thread_((void*) RTSTPhySim);
   printf("After create_rt_thread:MarkPhyInit=%d\n",MarkPhyInit);
   
   while(MarkPhyInit!=PhyInit_Ok)
   {
      usleep(2000);
      DealFIFO();
   }
   DealFIFO();
   
   printf("After PST::SendRecvPhyV!\n");  
   if(1!=IsBalance) Ierr=1;
   SubDealParaErr(Ierr);
   printf("After PSTInit2 Ierr=%d\n",Ierr);
   
   if (0!=Ierr) 
   {
      printf("Fail to initialization in Physical Device!");
      fprintf(fdbg,"Fail to initialization in Physical Device!");
      fflush(fdbg);
      
      RTDevOpenClose(0);
      goto LabelEND;
   }
   else
   {     
#ifndef _INFINIBAND      
      printf("Before First MPI_Barrier...\n");
      MPI_Barrier(procphy.CommCal);
      printf("After First MPI_Barrier...\n");
      
      fortran_rtclock_gettime_(&BarrierT);
      BarrierT += 2;
#endif
      
      MarkPhyInit=PhyInit_Over;
              
      //�ȴ����������
      wait_for_end();
      //�ļ��ر�
      RTDevOpenClose(0);
      
      printf("End of Physical Device Interface!\n");
      fprintf(fdbg,"End of Physical Device Interface!\n");
      fflush(fdbg);
   }
   //����MPI
LabelEND: 
   delete []simio.Vin;
   delete []simio.Vout;
   delete []simio.Vlast;
   delete []DataSendRecv;
   
   MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();
   fclose(fdbg);   
   return 0;
}

//ʵʱ����װ�ýӿڳ�ʼ��
void rtphyinit()
{
   lock_pages_();
   phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Initializing;
}

//�γɻ��硢���ͨѶ��
int FormComm()
{
   int i;   
   int *PCbuffer;   
   int *IDataSend,*IDataRecv,*Ncount,*NPos;

   MPI_Group GroupAll,GrouPCMain_Sub;
   MPI_Group GroupST,GroupEMT,GroupCAL;
   MPI_Comm CommMain_Sub,CommEMT;
   IDataSend=new int[mpiid.numprocs];
   IDataRecv=new int[mpiid.numprocs];
   Ncount=new int[mpiid.numprocs];
   NPos=new int[mpiid.numprocs+1];
   PCbuffer=new int[mpiid.numprocs];
   
   MPI_Comm_group( MPI_COMM_WORLD,&GroupAll);
   //----- �γɻ�����̬�������̬�����ͨ����
   // ÿ�����̽��Լ������ͺŷ�������
   int Ntype; 
   Ntype=SIMUTYPE_STPHY;
   MPI_Allgather( &Ntype, 1, MPI_INTEGER,IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);
   
   //���м������ͨ����
   int NumProcCal=0;
   for (i=0;i<mpiid.numprocs;i++){
      if (IDataRecv[i]<=200) {
         NumProcCal=NumProcCal+1;
         PCbuffer[NumProcCal-1]=i;
      }
      
      if(IDataRecv[i]==SIMUTYPE_STPHY)
      {
         if(mpiid.minPIDProc>i) mpiid.minPIDProc=i;
      }
   }
   
   if(NumProcCal>0) {
      MPI_Group_incl(GroupAll,NumProcCal,PCbuffer,&GroupCAL);
      MPI_Comm_create(MPI_COMM_WORLD,GroupCAL,&procphy.CommCal);
   }else{
      procphy.CommCal=MPI_COMM_NULL;
   }
   
   // ������̬ͨ����
   int NumProcST=0;
   for (i=0;i<mpiid.numprocs;i++){
      if (IDataRecv[i]<=100) {
         NumProcST=NumProcST+1;
         PCbuffer[NumProcST-1]=i;
      }
   }
   if (NumProcST>0) {
      MPI_Group_incl(GroupCAL,NumProcST,PCbuffer,&GroupST);
      MPI_Comm_create(procphy.CommCal,GroupST,&procphy.CommST);
      procphy.PCMain=0;
      MPI_Comm_size(procphy.CommST,&procphy.numprocst);
      MPI_Comm_rank(procphy.CommST,&procphy.myidst);
   }else{
      procphy.CommST=MPI_COMM_NULL;
   }
   // �����̬ͨ����
   int NumProcEMT=0;
   for (i=0;i<mpiid.numprocs;i++){
      if (IDataRecv[i]>100&&IDataRecv[i]<=200) {
         NumProcEMT=NumProcEMT+1;
         PCbuffer[NumProcEMT-1]=i;
      }
   }
   if (NumProcEMT>0) {
      MPI_Group_incl(GroupCAL,NumProcEMT,PCbuffer,&GroupEMT);
      MPI_Comm_create(procphy.CommCal,GroupEMT,&CommEMT);
   }
   
   MPI_Group_free( &GroupAll);
   MPI_Group_free( &GroupCAL);
   
   if  (NumProcST==0) return 1;
   
   //----- Form Main_Sub communicator -------------------------------------
   int NPCSub=1;
   PCbuffer[0]=procphy.PCMain;
   int NPCMat1,NPCPhy1,NPCUD1;
   NPCMat1=0;
   NPCPhy1=0;
   NPCUD1=0;
   for (i=0;i<NumProcST;i++) {
      if (IDataRecv[i]==SIMUTYPE_STSUB) {
         NPCSub=NPCSub+1;
         PCbuffer[NPCSub-1]=i;
      }
      if (IDataRecv[i]==SIMUTYPE_STMAT) NPCMat1=NPCMat1+1;
      if (IDataRecv[i]==SIMUTYPE_STUD) NPCUD1=NPCUD1+1;
      if (IDataRecv[i]==SIMUTYPE_STPHY) NPCPhy1=NPCPhy1+1;
   }
   // form GrouPCMain_Sub and CommMain_Sub
   MPI_Group_incl(GroupST,NPCSub,PCbuffer,&GrouPCMain_Sub);
   MPI_Comm_create(procphy.CommST,GrouPCMain_Sub,&CommMain_Sub);
   MPI_Group_free( &GroupST);
   delete []IDataSend;
   delete []IDataRecv;
   delete []Ncount;
   delete []NPos;
   delete []PCbuffer;
   return 0;
}

//�����ؽ���IO��Ϣ�����ýӿ����ݣ���������������ص�IO����
int RecvSendPhyIO()
{
   int NumVar,i,n,Nin,Nout,k;
   MPI_Status mpistat;
   
   MPI_Recv(&NumVar, 1, MPI_INTEGER, procphy.PCMain,101, procphy.CommST, &mpistat);
   int nlen=sizeof(PSTPIDIOVAR)*NumVar+2*sizeof(double);
   char *databuff= new char[nlen];
   MPI_Recv(databuff, nlen, MPI_CHAR, procphy.PCMain,102, procphy.CommST, &mpistat);
   PSTPIDIOVAR * iovars=(PSTPIDIOVAR *)databuff;
   //��ӡ
   fputs("MarkIO,PHY_Name,NetProcNo,CompType,CompName,ValueType,VarNo,PIBNo,SlotNo,SignNo,KAmplify \n",fdbg);
   for (i=0;i<NumVar;i++)
   {
      iovars[i].CompName[MaxLen_Name-1]=0;
      k=MaxLen_Name-2;
      while(iovars[i].CompName[k]==' ')
      {
         iovars[i].CompName[k]=0;
         k--;
      }
      
      iovars[i].PHY_Name[MaxLen_Name-1]=0;
      k=MaxLen_Name-2;
      while(iovars[i].PHY_Name[k]==' ')
      {
         iovars[i].PHY_Name[k]=0;
         k--;
      }     
 
      fprintf(fdbg,"%d,%s,%d,%d,%s,%d,%d,%d,%d,%d,%f \n",
         iovars[i].MarkIO,iovars[i].PHY_Name,iovars[i].NetProcNo,iovars[i].CompType,iovars[i].CompName,
         iovars[i].ValueType,iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
   }
   fflush(fdbg);
   double *pdouble;
   pdouble=(double*) (databuff+sizeof(PSTPIDIOVAR)*NumVar);
   simio.dt=*pdouble;
   pdouble++;
   simio.fhz=int(*pdouble);
   //ͳ������������̺�
   for (i=0;i<NumVar;i++)
   {
      //�������н���
      if (std::find(procphy.pcsubnet.begin(),procphy.pcsubnet.end(),iovars[i].NetProcNo)==procphy.pcsubnet.end()) {
         procphy.pcsubnet.push_back(iovars[i].NetProcNo);
      }
   }
   simio.numSubnet=procphy.pcsubnet.size();
   fprintf(fdbg,"NumVar=%d,simio.numSubnet=%d\n",NumVar,simio.numSubnet);
   PSTNET_PIDIO * stnetios=new PSTNET_PIDIO[NumVar];
   simio.IdVin=new int[simio.numSubnet+1];
   simio.IdVout=new int[simio.numSubnet+1];
   simio.IdVin[0]=0;
   simio.IdVout[0]=0;
   //��������ӿڵ������������
   for (n=0;n<simio.numSubnet;n++) {
      Nin=0;
      Nout=0;
      for (i=0;i<NumVar;i++) {
         if (iovars[i].NetProcNo==procphy.pcsubnet[n]) { //��ͬ����
            if (1==iovars[i].MarkIO) { //����
               gPIDITF.addavar(Nin,iovars[i].idno,iovars[i].NetProcNo,iovars[i].SignalType,iovars[i].ValueType,
                  iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
               Nin++;
            }else{   //���
               gPIDITF.addavar(Nout,iovars[i].idno,iovars[i].NetProcNo,iovars[i].SignalType,iovars[i].ValueType,
                  iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
               Nout++;
            }
            k=Nin+Nout-1;
            stnetios[k].MarkIO=iovars[i].MarkIO;
            stnetios[k].CompType=iovars[i].CompType;
            memcpy(stnetios[k].CompName,iovars[i].CompName,MaxLen_Name);
            stnetios[k].VarNo=iovars[i].VarNo;
         }
      }
      simio.IdVin[n+1]=simio.IdVin[n]+Nin;
      simio.IdVout[n+1]=simio.IdVout[n]+Nout;
      fprintf(fdbg,"simio.IdVin[%d]=%d,simio.IdVout[%d]=%d\n",n+1,simio.IdVin[n+1],n+1,simio.IdVout[n+1]);
      //���������������������
      k=Nin+Nout;
      //��ӡ
      fprintf(fdbg,"subnet %d:  k=%d, packlen=%d, stnetios=\n",procphy.pcsubnet[n],k,k*sizeof(PSTNET_PIDIO));
      for (i=0;i<k;i++)
      {
         fprintf(fdbg,"%d,%d,%s,%d\n",stnetios[i].MarkIO,stnetios[i].CompType,stnetios[i].CompName,stnetios[i].VarNo);
      }
      fflush(fdbg);
      MPI_Send(&k, 1, MPI_INT,procphy.pcsubnet[n], 101, procphy.CommST);
      MPI_Send(stnetios, k*sizeof(PSTNET_PIDIO), MPI_CHAR,procphy.pcsubnet[n], 102, procphy.CommST);
   }
    
   simio.Vin=new double[simio.IdVin[n]];
   simio.Vout=new double[simio.IdVout[n]];
   simio.Vlast=new double[simio.IdVout[n]];
   DataSendRecv=new double[simio.IdVin[n]+simio.IdVout[n]+1];
   
   if (simio.dt>DTINST) 
      simio.Ikt=int(simio.dt/DTINST+0.5);
   else
      simio.Ikt=1;

   fprintf(fdbg,"simio.Ikt=%d\n",simio.Ikt);
   delete []databuff;
   delete []stnetios;

   return 0;
}
  
//���ݷ�������ʼ������װ�� 
void *RTSTPhySim(void *arg)
{
   int Ius;
   int Ierr;   
   int k,kNT;
   int IsContinue;
   int i,Nin,Nout;
   char temp[256];
   MPI_Status mpistat;
   double dd,curTime;
   
   //˯�ߵȴ�ʱ��
   Ius=30; 
   IsBalance=0;

   // Receive input values
   //sprintf(temp,"NT=-1,Recv from ST:\n");
   //fortran_rtprints_(temp);
   
   for (i=0;i<simio.numSubnet;i++)
   {
      Nin=simio.IdVin[i+1]-simio.IdVin[i];
      
      MPI_Recv(DataSendRecv, Nin+1, MPI_DOUBLE_PRECISION, 
               procphy.pcsubnet[i],101, procphy.CommST, &mpistat);
           
      if(Nin>0)
      {         
         memcpy(&simio.Vin[simio.IdVin[i]],DataSendRecv,Nin*sizeof(double));
         gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);       
      }
      IsContinue=int(DataSendRecv[Nin]+0.5);
      
      //sprintf(temp,"pcsubnet No.=%d, Nin=%d, values:\n",procphy.pcsubnet[i],Nin);
      //fortran_rtprints_(temp);
      
      //for(k=0;k<Nin+1;k++)
      //{
      //   if(k==Nin)
      //      sprintf(temp,"%d \n",IsContinue);
      //   else
      //      sprintf(temp,"%f,",DataSendRecv[k]);
      //   fortran_rtprints_(temp);
      //}
   }
   
   //Receive input values finished!
   if(IsContinue==1)  return NULL;
   
   //�����ӿ���ͨ��
   gPIDITF.startMasterSim();

   //k�ĵ�λ��Ӧ����ܲ�
   for (k=0;;k++) 
   {     
      if(MarkPhyInit==PhyInit_Start)
      {
         if (gPIDITF.ishaveInstantInput())
         {
            for (kNT=0;kNT<simio.Ikt;kNT++)  //С����
            {
               //���
               gPIDITF.updateInstantVal(k,kNT);
               gPIDITF.SenddataToPID();

               //�ȴ����뵽��
               gPIDITF.MarkInput_Wait(0);
            }
         }
         else
         {
            gPIDITF.SenddataToPID();

            //�ȴ����뵽��
            gPIDITF.MarkInput_Wait(0);
         }
         
         Ierr=gPIDITF.getdatafromPID();
         
         for (i=0;i<simio.numSubnet;i++)
         {
            Nout=simio.IdVout[i+1]-simio.IdVout[i];
            gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);    
         }
         Nout=simio.IdVout[simio.numSubnet];
                 
         dd=0;
         for(i=0;i<Nout;i++) 
         {
            if (fabs(simio.Vlast[i]-simio.Vout[i])>dd) 
            {
               dd=fabs(simio.Vlast[i]-simio.Vout[i]);
            }
            simio.Vlast[i]=simio.Vout[i];
         }
         
         //sprintf(temp,"MarkPhyInit=%d,dd=%f\n",MarkPhyInit,dd);
         //fortran_rtprints_(temp);
         
         if(dd<0.01)
         {    
            if (gPIDITF.ishaveInstantInput()) 
            {
               double tt=simio.fhz*simio.dt*(k+1);
               //why? ���������ó�ʼ�������������˳�ѭ��
               if (fabs(tt-int(tt+0.4))>0.01)  
               {
                  fortran_clock_sleep_(&Ius);    
                  continue;
               }
            }
            
            //sprintf(temp,"Initial Values Error is %f!\n",dd);
            //fortran_rtprints_(temp);
            MarkPhyInit=PhyInit_Ok;
            IsBalance=1;
            break;
         }
         else if(k>1000)
         {
            MarkPhyInit=PhyInit_Ok;
            IsBalance=0;
            break;
         }
         fortran_clock_sleep_(&Ius);    
      }
      else
      {
         break;
      }
   }
   
   if(IsBalance==0) 
   {
      sprintf(temp,"Initial Values is not balance!\n");
      fortran_rtprints_(temp);
      Ierr=gPIDITF.stopSim();
      return NULL;
   }
     
   if(MarkPhyInit==PhyInit_Ok)
   {     
      //Send output values
      //sprintf(temp,"NT=-1,Send to ST:\n");
      //fortran_rtprints_(temp);
      
      for (i=0;i<simio.numSubnet;i++)
      {
         Nout=simio.IdVout[i+1]-simio.IdVout[i];
         if (Nout>0)
            memcpy(DataSendRecv,&simio.Vout[simio.IdVout[i]],Nout*sizeof(double));              
         DataSendRecv[Nout]=int(phyrtio.MarkRTCalculation+0.5);
         
         MPI_Send(DataSendRecv,Nout+1,MPI_DOUBLE_PRECISION,
                  procphy.pcsubnet[i],102,procphy.CommST);
         
         //sprintf(temp,"pcsubnet No.=%d, Nout=%d, values:\n",procphy.pcsubnet[i],Nout);
         //fortran_rtprints_(temp);
         
         //for(k=0;k<Nout+1;k++)
         //{
         //   if(k==Nout)
         //      sprintf(temp,"%d \n",(int)phyrtio.MarkRTCalculation);
         //   else
         //      sprintf(temp,"%f,",DataSendRecv[k]);
         //   fortran_rtprints_(temp);
         //}
      }
      
      //�ȴ����߳���Init_over�󷵻�
      for (k=0;;k++) 
      {
         if (gPIDITF.ishaveInstantInput())
         {
            for (kNT=0;kNT<simio.Ikt;kNT++)  //С����
            {
               //���
               gPIDITF.updateInstantVal(k,kNT);
               gPIDITF.SenddataToPID();
         
               //�ȴ����뵽��
               gPIDITF.MarkInput_Wait(0);
            }
         }
         else
         {
            gPIDITF.SenddataToPID();
         
            //�ȴ����뵽��
            gPIDITF.MarkInput_Wait(0);
         }
         
         Ierr=gPIDITF.getdatafromPID();
         
         for (i=0;i<simio.numSubnet;i++)
         {
            Nout=simio.IdVout[i+1]-simio.IdVout[i];
            gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);
         }
         Nout=simio.IdVout[simio.numSubnet];
         
         dd=0;
         for (i=0;i<Nout;i++) 
         {
            if (fabs(simio.Vlast[i]-simio.Vout[i])>dd) 
            {
               dd=fabs(simio.Vlast[i]-simio.Vout[i]);
            }
            simio.Vlast[i]=simio.Vout[i];
         }
   
         if(MarkPhyInit==PhyInit_Over)
         {
#ifndef _INFINIBAND
            fortran_rtclock_gettime_(&curTime);
            if(curTime<=BarrierT-2*simio.dt)
            {
               fortran_clock_sleep_(&Ius);  
               continue;
            }              
#endif            
            if(dd<0.01||k>1000)
            {
               if (gPIDITF.ishaveInstantInput()) 
               {
                  double tt=simio.fhz*simio.dt*(k+1);
                  if (fabs(tt-int(tt+0.4))>0.01) 
                  {
                     fortran_clock_sleep_(&Ius);    
                     continue;
                  }
               }
               
               //sprintf(temp,"Initial Values Error is %f!\n",dd);
               //fortran_rtprints_(temp);   
               break;
            }
            
            fortran_clock_sleep_(&Ius);  
            continue;
         }
         
         fortran_clock_sleep_(&Ius);     
      } 
  
      RTSTPhySimulating();
   }
     
   phyrtio.MarkRTCalculation=SPhyRTIO::MRT_End;
   return NULL;
}

//ʵʱ����ӿں���
void RTSTPhySimulating()
{
   int NT;
   int Ierr;
   int k,kNT;
   int IsContinue;
   int SleepTime;
   int i,Nin,Nout;
   MPI_Status mpistat;
   char temp[256];

#ifndef _INFINIBAND
   fortran_clock_sleepto_(&BarrierT);  
   MPI_Barrier(procphy.CommCal);
#else
   MPI_Barrier_np(procphy.CommCal);
   MPI_Barrier(procphy.CommCal);
#endif

   //�յ�������̷��͵Ŀ�ʼ�����������
   for (i=0;i<simio.numSubnet;i++)
   {
      MPI_Recv(&k, 1, MPI_INTEGER, procphy.pcsubnet[i],1, procphy.CommST, &mpistat);
   }
     
   // ��ֵƽ�����,��ʼ�������
   //----- Integral part -------------------------------
      
   NT=0;
   sprintf(temp,"Starting simulating...\n");
   fortran_rtprints_(temp);
   
   phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Running;
   
   while (TRUE)
   {    
      kNT=0;
      IsContinue=0;
      
      // Receive input values and IsContinue
      // Val_in(),KIT, IsCOntinue 
      for (i=0;i<simio.numSubnet;i++)
      {
         Nin=simio.IdVin[i+1]-simio.IdVin[i];
         
         MPI_Recv(DataSendRecv, Nin+1, MPI_DOUBLE_PRECISION, 
                  procphy.pcsubnet[i],101, procphy.CommST, &mpistat);
                  
         if (Nin>0)
         {
            memcpy(&simio.Vin[simio.IdVin[i]],DataSendRecv,Nin*sizeof(double)); 
            gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);
         }
         IsContinue=int(DataSendRecv[Nin]+0.5);
         
         //if((NT-1)%100==0) 
         //{
         //   sprintf(temp,"pcsubnet No.=%d, NT=%d, Nin=%d, Input:\n",procphy.pcsubnet[i],NT,Nin);
         //   fortran_rtprints_(temp);
         //   
         //   for(k=0;k<Nin+1;k++)
         //   {
         //      if(k==Nin)
         //         sprintf(temp,"%d \n",IsContinue);
         //      else
         //         sprintf(temp,"%f,",DataSendRecv[k]);
         //      fortran_rtprints_(temp);
         //   } 
         //}
      }
      
      //sprintf(temp,"IsContinue=%d\n",IsContinue);
      //fortran_rtprints_(temp);      
      
      if (1==IsContinue) break;
      
      Ierr=gPIDITF.getdatafromPID();
      
      //Send output values
      for (i=0;i<simio.numSubnet;i++)
      {
         Nout=simio.IdVout[i+1]-simio.IdVout[i];
         if (Nout>0)
         {
            gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);
            memcpy(DataSendRecv,&simio.Vout[simio.IdVout[i]],Nout*sizeof(double));     
         }
         DataSendRecv[Nout]=int(phyrtio.MarkRTCalculation+0.5);
                  
         MPI_Send(DataSendRecv,Nout+1,MPI_DOUBLE_PRECISION,
                  procphy.pcsubnet[i],102,procphy.CommST);
         
         //if((NT-1)%100==0) 
         //{
         //   sprintf(temp,"pcsubnet No.=%d, NT=%d, Nout=%d, Output:\n",procphy.pcsubnet[i],NT,Nout);
         //   fortran_rtprints_(temp);
         //   
         //   for(k=0;k<Nout+1;k++)
         //   {
         //      if(k==Nout)
         //         sprintf(temp,"%d \n",int(phyrtio.MarkRTCalculation));
         //      else
         //         sprintf(temp,"%f,",DataSendRecv[k]);
         //      fortran_rtprints_(temp);
         //   }
         //}
      }
      
      if (gPIDITF.ishaveInstantInput())
      {
         for (kNT=0;kNT<simio.Ikt;kNT++)  //С����
         {
            //���
            gPIDITF.updateInstantVal(NT,kNT);
            gPIDITF.SenddataToPID();
            //�ȴ����뵽��
            gPIDITF.MarkInput_Wait(0);
         }
      }
      else
      {
         //���
         gPIDITF.SenddataToPID();
         //�ȴ����뵽��
         gPIDITF.MarkInput_Wait(0);
      }
      NT++;
   }
   
   sprintf(temp,"End of simulating!\n");
   fortran_rtprints_(temp);
   
   gPIDITF.ClearOutput();
   SleepTime = 10*1000;
   fortran_clock_sleep_(&SleepTime);
   
   gPIDITF.stopSim();
   
   sprintf(temp,"RTSTPhySimulating Returned!\n");
   fortran_rtprints_(temp);
 
   return;
}

//��������Ϣ�ڲ��г���䷢��
void SubDealParaErr(int &ErrorNo){
   int *IDataRecv=new int[mpiid.numprocs];
   //flog<<"Before MPI_Gather ErrorNo="<<ErrorNo<<endl;
   // send ErrorNo to Mainctrl
   
   //���н��̴�����ǰͬ��������������ȴ�
   int Ierr=MPI_Barrier(procphy.CommST); //added by xdc 2013.4.18
   
   MPI_Gather(&ErrorNo, 1, MPI_INTEGER,IDataRecv, mpiid.numprocs, MPI_INTEGER,procphy.PCMain, procphy.CommST);
   //flog<<"After MPI_Gather ErrorNo="<<ErrorNo<<endl;
   // wait for ErrorNo from Mainctrl
   MPI_Bcast(&ErrorNo,1,MPI_INTEGER,procphy.PCMain,procphy.CommST);
   //flog<<"After MPI_Bcast ErrorNo="<<ErrorNo<<endl;
   delete []IDataRecv;
}

//-----------------------------------------------------------------
// �ȴ�ʵʱ���̼�������������ļ������ͨ��
//-----------------------------------------------------------------
void wait_for_end()
{
   while(SPhyRTIO::MRT_End!=phyrtio.MarkRTCalculation)
   {
      usleep(2000);
      DealFIFO();
   }
   //������Ϻ������һ��
   usleep(2000);
   DealFIFO();
}

//-----------------------------------------------------------------
// ���¼�ļ���д���ַ���
//-----------------------------------------------------------------
void Writestr2FIFO(int hdl, char *str)
{
   int nlen,nret;
   nlen=strlen(str);
   if (SPhyRTIO::MRT_Running==phyrtio.MarkRTCalculation) { //ʵʱ
      nret=rt_write(hdl,str,nlen);
   }
   else
   fwrite(str,sizeof(char),nlen,fdbg);
}
//-----------------------------------------------------------------
// �ȴ�ʵʱ���̼�������������ļ������ͨ��
//-----------------------------------------------------------------
void DealFIFO()
{
   char sline[MaxLeng_Buffer];
   int nret;
   // ��Ļ��Ϣ���
   nret=rt_read(phyrtio.hdl_crtprint,sline,MaxLeng_Buffer);
   if (nret>0) {
      sline[nret]='\0';
      printf(sline);
   }
   //������Ϣ���
   do {
      nret=rt_read(phyrtio.hdl_FileError,sline,MaxLeng_Buffer);
      if (nret>0) {
         sline[nret]='\0';
         fputs(sline,fdbg);
      }
   }while(0<nret);
}

//-----------------------------------------------------------------
// ʵʱ�豸�Ĵ򿪺͹رգ������ļ���ͨ�Ŷ˿�
//-----------------------------------------------------------------
void RTDevOpenClose(int Isopen){
   char Sno1[3],sname[30];
   bool isok;
   int nret;
   //���豸�ļ�
   if (1==Isopen) {
      phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Running;
      sprintf(Sno1,"%d",mpiid.myid+1);
      // ��Ļ��ӡ��Ϣ
      sprintf(sname,"crtprint%d",mpiid.myid+1);
      sprintf(phyrtio.dev_crtprint,"FIFO%s",sname);
      fortran_fifo_open_(phyrtio.dev_crtprint, &phyrtio.hdl_crtprint);   
      // ������Ϣ�ļ�
      sprintf(sname,"PhyERR%d.LIS",mpiid.myid+1);
      sprintf(phyrtio.dev_FileError,"FIFO%s",sname);
      fortran_fifo_open_(phyrtio.dev_FileError, &phyrtio.hdl_FileError);
   }else{
      fortran_fifo_close_(phyrtio.dev_crtprint, &phyrtio.hdl_crtprint,&nret);
      fortran_fifo_close_(phyrtio.dev_FileError,&phyrtio.hdl_FileError,&nret);
   }
}

