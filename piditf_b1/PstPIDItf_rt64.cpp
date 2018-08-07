// PstPIDItf.cpp : 定义控制台应用程序的入口点。
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

//MPI标识
struct SMPIID mpiid;
//物理装置相关进程
struct SPROCPHY procphy;
// 输出文件
FILE * fdbg;
//实时相关
struct SPhyRTIO phyrtio;
// 仿真系统子网的输入输出变量
struct SNETIO simio;
//全局接口对象
CADPSS_PID_ITF gPIDITF;
//瞬时值步长
const double DTINST=(1.0e-4*2);   //100us
//数据收发缓存  xdc added 2013.4.2
double *DataSendRecv=NULL;

#ifdef ZX_DBG2
   double *timestamp_init;
   double *timestamp;
   #define TS_LEN 1000*100
#endif

//初始化的处理
enum {PhyInit_Start,PhyInit_Ok,PhyInit_Stop,PhyInit_Over} MarkPhyInit;
int IsBalance=0;
double BarrierT;

int main(int argc,char * argv[])
{
   int NT;   
   int k,kNT;      
   int i,Nin,Nout;
   MPI_Status mpistat;

   //1.初始化物理装置进程实时环境
   rtphyinit();
   
   MPI_Init(&argc,&argv);
   MPI_Comm_size(MPI_COMM_WORLD,&mpiid.numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&mpiid.myid);
   //取最大的进程号,为获得最小物理接口进程号做准备
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
   
   //形成一些公用通信体
   int Ierr=0;  
   Ierr=FormComm();
  
   //从主控接受IO信息，设置接口内容，向子网发送与相关的IO变量
   Ierr=RecvSendPhyIO();
   fprintf(fdbg,"After RecvSendPhyIO, return %d .\n",Ierr);
   fflush(fdbg);
   
   //为CADPSS_PID对象分配缓冲区空间，并将其push到gPID.VPID中
   gPIDITF.allocBuf();
   gPIDITF.pushPID();
   
   //设置最小PID进程的最小序号接口箱为主控
   fprintf(fdbg, "minPIDProc = %d\n", mpiid.minPIDProc);
   fflush(fdbg);
   
   //设置最小PID进程的最小序号接口箱为主控
   if(mpiid.myid==mpiid.minPIDProc) 
      gPIDITF.setMaster();
   
   //物理接口初始化
   if (gPIDITF.ishaveInstantInput())
   {
      fprintf(fdbg,"ishaveInstantInput() return TRUE.\n");
      //传入参数：物理接口箱的仿真步长，浮点数
      Ierr=gPIDITF.init(simio.dt/simio.Ikt);
      gPIDITF.initInstantPara(simio.dt,simio.fhz,simio.Ikt);
   }
   else
   {
      fprintf(fdbg,"ishaveInstantInput() return FALSE.\n");
      //传入参数：物理接口箱的仿真步长，浮点数
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
   
   //创建初始化实时进程
   RTDevOpenClose(1);
    
   //开始计算
   //停止所有接口箱的同步信号
   gPIDITF.stopSync();
   
   //所有从站转换到运行状态
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
              
      //等待至计算结束
      wait_for_end();
      //文件关闭
      RTDevOpenClose(0);
      
      printf("End of Physical Device Interface!\n");
      fprintf(fdbg,"End of Physical Device Interface!\n");
      fflush(fdbg);
   }
   //结束MPI
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

//实时物理装置接口初始化
void rtphyinit()
{
   lock_pages_();
   phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Initializing;
}

//形成机电、电磁通讯体
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
   //----- 形成机电暂态、电磁暂态计算的通信体
   // 每个进程将自己的类型号发给主控
   int Ntype; 
   Ntype=SIMUTYPE_STPHY;
   MPI_Allgather( &Ntype, 1, MPI_INTEGER,IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);
   
   //所有计算进程通信体
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
   
   // 机电暂态通信体
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
   // 电磁暂态通信体
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

//从主控接受IO信息，设置接口内容，向子网发送与相关的IO变量
int RecvSendPhyIO()
{
   int NumVar,i,n,Nin,Nout,k;
   MPI_Status mpistat;
   
   MPI_Recv(&NumVar, 1, MPI_INTEGER, procphy.PCMain,101, procphy.CommST, &mpistat);
   int nlen=sizeof(PSTPIDIOVAR)*NumVar+2*sizeof(double);
   char *databuff= new char[nlen];
   MPI_Recv(databuff, nlen, MPI_CHAR, procphy.PCMain,102, procphy.CommST, &mpistat);
   PSTPIDIOVAR * iovars=(PSTPIDIOVAR *)databuff;
   //打印
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
   //统计相关子网进程号
   for (i=0;i<NumVar;i++)
   {
      //查找已有进程
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
   //设置物理接口的输入输出变量
   for (n=0;n<simio.numSubnet;n++) {
      Nin=0;
      Nout=0;
      for (i=0;i<NumVar;i++) {
         if (iovars[i].NetProcNo==procphy.pcsubnet[n]) { //相同子网
            if (1==iovars[i].MarkIO) { //输入
               gPIDITF.addavar(Nin,iovars[i].idno,iovars[i].NetProcNo,iovars[i].SignalType,iovars[i].ValueType,
                  iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
               Nin++;
            }else{   //输出
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
      //向子网返回输入输出变量
      k=Nin+Nout;
      //打印
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
  
//根据仿真程序初始化物理装置 
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
   
   //睡眠等待时间
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
   
   //启动接口箱通信
   gPIDITF.startMasterSim();

   //k的单位对应半个周波
   for (k=0;;k++) 
   {     
      if(MarkPhyInit==PhyInit_Start)
      {
         if (gPIDITF.ishaveInstantInput())
         {
            for (kNT=0;kNT<simio.Ikt;kNT++)  //小步长
            {
               //输出
               gPIDITF.updateInstantVal(k,kNT);
               gPIDITF.SenddataToPID();

               //等待输入到达
               gPIDITF.MarkInput_Wait(0);
            }
         }
         else
         {
            gPIDITF.SenddataToPID();

            //等待输入到达
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
               //why? 可能用于让初始化在整步长点退出循环
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
      
      //等待主线程置Init_over后返回
      for (k=0;;k++) 
      {
         if (gPIDITF.ishaveInstantInput())
         {
            for (kNT=0;kNT<simio.Ikt;kNT++)  //小步长
            {
               //输出
               gPIDITF.updateInstantVal(k,kNT);
               gPIDITF.SenddataToPID();
         
               //等待输入到达
               gPIDITF.MarkInput_Wait(0);
            }
         }
         else
         {
            gPIDITF.SenddataToPID();
         
            //等待输入到达
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

//实时仿真接口函数
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

   //收到仿真进程发送的开始命令后再启动
   for (i=0;i<simio.numSubnet;i++)
   {
      MPI_Recv(&k, 1, MPI_INTEGER, procphy.pcsubnet[i],1, procphy.CommST, &mpistat);
   }
     
   // 初值平衡完毕,开始仿真过程
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
         for (kNT=0;kNT<simio.Ikt;kNT++)  //小步长
         {
            //输出
            gPIDITF.updateInstantVal(NT,kNT);
            gPIDITF.SenddataToPID();
            //等待输入到达
            gPIDITF.MarkInput_Wait(0);
         }
      }
      else
      {
         //输出
         gPIDITF.SenddataToPID();
         //等待输入到达
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

//将错误信息在并行程序间发布
void SubDealParaErr(int &ErrorNo){
   int *IDataRecv=new int[mpiid.numprocs];
   //flog<<"Before MPI_Gather ErrorNo="<<ErrorNo<<endl;
   // send ErrorNo to Mainctrl
   
   //并行进程错误处理前同步，避免程序出错等待
   int Ierr=MPI_Barrier(procphy.CommST); //added by xdc 2013.4.18
   
   MPI_Gather(&ErrorNo, 1, MPI_INTEGER,IDataRecv, mpiid.numprocs, MPI_INTEGER,procphy.PCMain, procphy.CommST);
   //flog<<"After MPI_Gather ErrorNo="<<ErrorNo<<endl;
   // wait for ErrorNo from Mainctrl
   MPI_Bcast(&ErrorNo,1,MPI_INTEGER,procphy.PCMain,procphy.CommST);
   //flog<<"After MPI_Bcast ErrorNo="<<ErrorNo<<endl;
   delete []IDataRecv;
}

//-----------------------------------------------------------------
// 等待实时进程计算结束，处理文件输出和通信
//-----------------------------------------------------------------
void wait_for_end()
{
   while(SPhyRTIO::MRT_End!=phyrtio.MarkRTCalculation)
   {
      usleep(2000);
      DealFIFO();
   }
   //计算完毕后再输出一次
   usleep(2000);
   DealFIFO();
}

//-----------------------------------------------------------------
// 向记录文件中写入字符串
//-----------------------------------------------------------------
void Writestr2FIFO(int hdl, char *str)
{
   int nlen,nret;
   nlen=strlen(str);
   if (SPhyRTIO::MRT_Running==phyrtio.MarkRTCalculation) { //实时
      nret=rt_write(hdl,str,nlen);
   }
   else
   fwrite(str,sizeof(char),nlen,fdbg);
}
//-----------------------------------------------------------------
// 等待实时进程计算结束，处理文件输出和通信
//-----------------------------------------------------------------
void DealFIFO()
{
   char sline[MaxLeng_Buffer];
   int nret;
   // 屏幕信息输出
   nret=rt_read(phyrtio.hdl_crtprint,sline,MaxLeng_Buffer);
   if (nret>0) {
      sline[nret]='\0';
      printf(sline);
   }
   //错误信息输出
   do {
      nret=rt_read(phyrtio.hdl_FileError,sline,MaxLeng_Buffer);
      if (nret>0) {
         sline[nret]='\0';
         fputs(sline,fdbg);
      }
   }while(0<nret);
}

//-----------------------------------------------------------------
// 实时设备的打开和关闭，包括文件和通信端口
//-----------------------------------------------------------------
void RTDevOpenClose(int Isopen){
   char Sno1[3],sname[30];
   bool isok;
   int nret;
   //打开设备文件
   if (1==Isopen) {
      phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Running;
      sprintf(Sno1,"%d",mpiid.myid+1);
      // 屏幕打印信息
      sprintf(sname,"crtprint%d",mpiid.myid+1);
      sprintf(phyrtio.dev_crtprint,"FIFO%s",sname);
      fortran_fifo_open_(phyrtio.dev_crtprint, &phyrtio.hdl_crtprint);   
      // 错误信息文件
      sprintf(sname,"PhyERR%d.LIS",mpiid.myid+1);
      sprintf(phyrtio.dev_FileError,"FIFO%s",sname);
      fortran_fifo_open_(phyrtio.dev_FileError, &phyrtio.hdl_FileError);
   }else{
      fortran_fifo_close_(phyrtio.dev_crtprint, &phyrtio.hdl_crtprint,&nret);
      fortran_fifo_close_(phyrtio.dev_FileError,&phyrtio.hdl_FileError,&nret);
   }
}

