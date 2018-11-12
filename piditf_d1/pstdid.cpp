// PstPIDItf.cpp : �������̨Ӧ�ó������ڵ㡣
//

//#include "stdafx.h"
#include "mpi.h"
#include <stdio.h>
#include <algorithm>
#include "PstPIDITF.h"
#include "math.h"
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "rtftoc_intel.h"

#include "debug_config.h"

#include "cal_sock.h"
#include "MSG_SRFunc.h"
#include "MSG_SRDef.h"


//MPI��ʶ
struct SMPIID mpiid;
//����װ����ؽ���
struct SPROCPHY procphy;
// ����ļ�
FILE * fdbg;
FILE * Recfdbg;

//ʵʱ���
struct SPhyRTIO phyrtio;
// ����ϵͳ�����������������
struct SNETIO simio;
//ȫ�ֽӿڶ���
CADPSS_PID_ITF gPIDITF;
//˲ʱֵ����
const double DTINST=1.0e-4;   //100us
//�����շ�����  xdc added 2013.4.2
double *DataSendRecv=NULL;

//��ʼ���Ĵ���
enum {PhyInit_Start,PhyInit_Ok,PhyInit_Stop,PhyInit_Over} MarkPhyInit;
int IsBalance=0;
double BarrierT;

void fortran_rtprints_(char* temp)
{
  fprintf(fdbg,"%s\n",temp);
}

int main(int argc,char * argv[])
{

   printf("interface program begin----\n");
   if(RT_ENABLE) 
      printf("this problem is for real time  mode---\n");
   if(TCP_ENABLE)
       printf("this problem is for TCP mode---\n");

   int NT;   
   int k,kNT;      
   int i,Nin,Nout;
   MPI_Status mpistat;

   //1.��ʼ������װ�ý���ʵʱ����
   
   rtphyinit();
   printf("after rtphyini-----\n");
   
   MPI_Init(&argc,&argv);
   MPI_Comm_size(MPI_COMM_WORLD,&mpiid.numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&mpiid.myid);
   //ȡ���Ľ��̺�,Ϊ�����С����ӿڽ��̺���׼��
   mpiid.minPIDProc=mpiid.numprocs;
   
   char fname[32];
   char str_dbg[256];
   sprintf(fname,"PDIDERR%d.LIS",mpiid.myid+1);
   fdbg=fopen(fname,"w");
   if (NULL==fdbg) {
      printf("Fail to open file %s",fname);
      return 1;
   }

   char* cRfdbg="interface_receive.log";
   Recfdbg=fopen(cRfdbg,"w");
   if (NULL==Recfdbg) {
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
   if(! DEBUG_NO_ST)
   {
       Ierr=FormComm();
       printf("after FormComm---\n");
   }

   if(DEBUG)
   {
	   sprintf(str_dbg,"mpiid.minPIDProc=%d\n",mpiid.minPIDProc);

   }


   //������СPID���̵���С��Žӿ���Ϊ����
   //if(mpiid.myid==mpiid.minPIDProc)
   //if(mpiid.myid==mpiid.numprocs-1)
   if(DPDK_ENABLE==1)
   {
	  printf("mpiid.myid=%d,mpiid.numprocs=%d \n ",mpiid.myid,mpiid.numprocs);
      //gPIDITF.setMaster();
      int ret = dpdk_init();
      if(ret == -1)
         {
    	  sprintf(str_dbg,"dpdk init failed and exit !\n");
      	  dpdk_release();
      	  gPIDITF.finalize();
      	  return -1;
         }
      printf("mpiid.myid:%d:dpdk init done!\n",mpiid.myid);

   }
   else    //socket
   {
	   printf("mpiid.myid=%d,mpiid.numprocs=%d \n ",mpiid.myid,mpiid.numprocs);
	   printf("socket to be initialized!\n");
	   char CIP[100];
	   int portNO;
	   strcpy(CIP,"127.0.0.1");
	   portNO=8000;
		int Ierr=1;
		int iii=0;
		do {
			Ierr=InitConnection(CIP,portNO);
			iii=iii+1;
			printf("Connecting to Socket B, try %d\n",iii);
			sleep(1);

		}while ((Ierr!=0)&&(iii<100));

		if(Ierr!=0)
		{
			printf("Not able to connect to Socket link, so have to exit!\n");
		    exit(-1);
   	    }
   }

   char* pWrtMsg="hello,you are connected!\n";
   char* pWrtMsg2="hello,I am who!\n";

   WriteMessage(pWrtMsg, 40);
   WriteMessage(pWrtMsg2, 40);

   //�����ؽ���IO��Ϣ�����ýӿ����ݣ���������������ص�IO����

   Ierr=RecvSendPhyIO();
   fprintf(fdbg,"After RecvSendPhyIO, return %d .\n",Ierr);
   fflush(fdbg);
   

   //ΪCADPSS_PID������仺�����ռ䣬������push��gPID.VPID��
   gPIDITF.allocBuf();
   gPIDITF.pushPID();

	//����ӿڳ�ʼ��
	if (gPIDITF.ishaveInstantInput())
	{
		fprintf(fdbg,"ishaveInstantInput() return TRUE.\n");
		Ierr=gPIDITF.init(simio.fhz*simio.Ikt);
		gPIDITF.initInstantPara(simio.dt,simio.fhz,simio.Ikt);
	}else{
		fprintf(fdbg,"ishaveInstantInput() return FALSE.\n");
		Ierr=gPIDITF.init(simio.fhz);
	}
	if (Ierr!=0) {
		fprintf(fdbg,"Fail to initialize Physical device, ErrorNo= %d.\n",Ierr);
	}else{
		fprintf(fdbg,"Succeed to initialize Physical device.\n");
	}
	fflush(fdbg);


   
   //������СPID���̵���С��Žӿ���Ϊ����
   fprintf(fdbg, "minPIDProc = %d  myid=%d\n", mpiid.minPIDProc,mpiid.myid);
   fflush(fdbg);

     
   if (Ierr<0) Ierr=-Ierr;
   SubDealParaErr(Ierr);
   SubDealParaErr(Ierr);
   fprintf(fdbg,"After SubDealParaErr,Ierr=%d \n ",Ierr);
   fflush(fdbg);
   
   if (Ierr!=0) goto LabelEND;
     
   MarkPhyInit=PhyInit_Start;
   
   printf("Before RTSTPhySim:MarkPhyInit=%d\n",MarkPhyInit); 
   DigitalSimulating();
   printf("After RTSTPhySim:MarkPhyInit=%d\n",MarkPhyInit);
   
   while(DEBUG_REC)
   {

   }

   MarkPhyInit=PhyInit_Over;         
     
   printf("End of Physical Device Interface!\n");
   fprintf(fdbg,"End of Physical Device Interface!\n");
   fflush(fdbg);
   
   //����MPI
LabelEND: 
   delete []simio.Vin;
   delete []simio.Vout;
   delete []simio.Vlast;
   delete []DataSendRecv;
   
   MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();
   fclose(fdbg);
   fclose(Recfdbg);

   if(DPDK_ENABLE)
   {
	   dpdk_release();
   }

   gPIDITF.finalize();
   return 0;
}

void rtphyinit()
{
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
   Ntype=SIMUTYPE_STCom;
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
    	 //printf("mpiid.minPIDProc=%d,i=%d\n",mpiid.minPIDProc,i);
         //if(mpiid.minPIDProc>i) mpiid.minPIDProc=i;
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
   int NPCMat1,NPCPhy1,NPCUD1,NPCCom;
   NPCMat1=0;
   NPCPhy1=0;
   NPCUD1=0;
   NPCCom=0;
   for (i=0;i<NumProcST;i++) {
      if (IDataRecv[i]==SIMUTYPE_STSUB) {
         NPCSub=NPCSub+1;
         PCbuffer[NPCSub-1]=i;
      }
      if (IDataRecv[i]==SIMUTYPE_STMAT) NPCMat1=NPCMat1+1;
      if (IDataRecv[i]==SIMUTYPE_STUD) NPCUD1=NPCUD1+1;
      if (IDataRecv[i]==SIMUTYPE_STPHY) NPCPhy1=NPCPhy1+1;
      if (IDataRecv[i]==SIMUTYPE_STCom)
    	  {
    	   	   NPCCom=NPCCom+1;
    	   	 fprintf(fdbg,"NPCCom=%d,i=%d\n",NPCCom,i);

    	    	 //printf("mpiid.minPIDProc=%d,i=%d\n",mpiid.minPIDProc,i);
    	       mpiid.minPIDProc=NPCCom;

    	  }
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
	printf(" RecvSendPhyIO begin:----\n");

	PSTPIDIOVAR * iovars;
	char *databuff;
	if(!DEBUG_NO_ST)
	{
		MPI_Recv(&NumVar, 1, MPI_INTEGER, procphy.PCMain,101, procphy.CommST, &mpistat);
		int nlen=sizeof(PSTPIDIOVAR)*NumVar+2*sizeof(double);
		databuff= new char[nlen];
		MPI_Recv(databuff, nlen, MPI_CHAR, procphy.PCMain,102, procphy.CommST, &mpistat);
		iovars=(PSTPIDIOVAR *)databuff;
	}
	else
	{
		//define internal testing data
		/*struct PSTPIDIOVAR iovars[9]={
				{1   ,   1   ,   1   ,  1   ,   1   ,5422   ,  40   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   2   ,   1   ,  1   ,   1   ,5422   , 121   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   3   ,   1   ,  1   ,   1   ,5422   , 125   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   4   ,   1   ,  1   ,   1   ,5422   ,  36   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   5   ,   1   ,  1   ,   1   ,5422   ,  12   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   6   ,   1   ,  1   ,   1   ,5422   ,  38   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   7   ,   1   ,  1   ,   1   ,5422   , 123   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�հ�Ʒ__-230'},
				{1   ,   8   ,   1   ,  1   ,   1   ,26758   ,  36   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'�ս��NJ-230'},
				{1   ,   9   ,   1   ,  1   ,   2   ,23587   , 132   ,   1   ,   0   ,  1   ,   0   ,   0   ,   1,'ĪƷ2573_ac'}
		};
		NumVar=9;
		*/
	}
	//��ӡ
	fprintf(fdbg,"NumVar:%d\n",NumVar);
	fprintf(fdbg,"Valid,Vidno,ISST,SimProcNo,CompType,CompNo,VarNo,SigType,ValueType,ExIDNo,ChNo,Index,CompName \n");
	//fputs("Valid,Vidno,ISST,SimProcNo,CompType,CompNo,VarNo,SigType,ValueType,ExIDNo,ChNo,Index,CompName \n",fdbg);
	for (i=0;i<NumVar;i++)
	{
		iovars[i].CompName[MaxLen_Name-1]=0;
		k=MaxLen_Name-2;
		while(iovars[i].CompName[k]==' ')
		{
			iovars[i].CompName[k]=0;
			k--;
		}

		fprintf(fdbg,"config data, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s \n",
				iovars[i].Valid,iovars[i].Vidno,iovars[i].ISST,iovars[i].SimProcNo,
				iovars[i].CompType,iovars[i].CompNo,iovars[i].VarNo,iovars[i].SigType,iovars[i].ValueType,
				iovars[i].PidNo,iovars[i].PidChNo,iovars[i].Index,iovars[i].CompName);
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
      if (std::find(procphy.pcsubnet.begin(),procphy.pcsubnet.end(),iovars[i].SimProcNo)==procphy.pcsubnet.end()) {
         procphy.pcsubnet.push_back(iovars[i].SimProcNo);
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
         if (iovars[i].SimProcNo==procphy.pcsubnet[n]) { //��ͬ����
            if (1==iovars[i].Valid) { //����
               gPIDITF.addavar(Nin,iovars[i].Vidno,iovars[i].SimProcNo,iovars[i].SigType,iovars[i].ValueType,
                  iovars[i].VarNo,iovars[i].PidNo,iovars[i].PidChNo,1,1);
               Nin++;
            }else{   //���
               gPIDITF.addavar(Nout,iovars[i].Vidno,iovars[i].SimProcNo,iovars[i].SigType,iovars[i].ValueType,
                  iovars[i].VarNo,iovars[i].PidNo,iovars[i].PidChNo,1,1);
               Nout++;
            }
            k=Nin+Nout-1;
            stnetios[k].MarkIO=iovars[i].Valid;
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
      if(!DEBUG_NO_ST)
      {
        MPI_Send(&k, 1, MPI_INT,procphy.pcsubnet[n], 101, procphy.CommST);
        MPI_Send(stnetios, k*sizeof(PSTNET_PIDIO), MPI_CHAR,procphy.pcsubnet[n], 102, procphy.CommST);
      }
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

   printf(" RecvSendPhyIO end:----\n");
   return 0;
}
  
//ʵʱ����ӿں���
void DigitalSimulating()
{
   int NT;
   int Ierr;
   int k,kNT;
   int IsContinue;
   int SleepTime;
   int i,Nin,Nout;
   MPI_Status mpistat;
   char temp[256];

   fprintf(fdbg,"DigitalSimulating: to MPI Recv\n");
   fflush(fdbg);

   if(RT_ENABLE)
   {
      printf("MPI Barrier  begine ---\n");
      int xx=-2;
      xx=MPI_Barrier(procphy.CommCal); //return value is 0
      //xx=MPI_Barrier(MPI_COMM_WORLD);//not return any value 
      printf("MPI Barrier return value: %d\n",xx);

      printf("Second MPI Barrier  begine ---\n");
      xx=MPI_Barrier(procphy.CommCal); //return value is 0
      printf("Second MPI Barrier return value: %d\n",xx);
   }

   //�յ�������̷��͵Ŀ�ʼ�����������
   for (i=0;i<simio.numSubnet;i++)
   {
          if(DEBUG)
             printf("procphy.pcsubnet[%d]=%d",i,procphy.pcsubnet[i]);

	  if(!DEBUG_NO_ST)
	       MPI_Recv(&k, 1, MPI_INTEGER, procphy.pcsubnet[i],1, procphy.CommST, &mpistat);
   }
   fprintf(fdbg,"DigitalSimulating: after MPI Recv\n");
   fflush(fdbg);
     
   // ��ֵƽ�����,��ʼ�������
   //----- Integral part -------------------------------
      
   NT=0;
   sprintf(temp,"Starting simulating...\n");
   fortran_rtprints_(temp);
   
   phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Running;
   
   int x=0;

   if(DEBUG_NO_ST)
   {
	//   DataSendRecv = double rec[10];
	//   for(int i=0; i<10;i++)
    //	   rec[i]=i;
   }

   while (TRUE)
   {   
      //printf("begin to dead loop:\n");
      IsContinue=0;
      
      // Receive input values and IsContinue
      // Val_in(),KIT, IsCOntinue 
      for (i=0;i<simio.numSubnet;i++)
      {
         Nin=simio.IdVin[i+1]-simio.IdVin[i];
         
         //printf("MPI_Recv:\n");
         if(!DEBUG_NO_ST)
         {
             MPI_Recv(DataSendRecv, Nin+1, MPI_DOUBLE_PRECISION,
                  procphy.pcsubnet[i],101, procphy.CommST, &mpistat);
         }
         x++;
         if(x%INFO_PRINT_STEP==1 ||DEBUG )
                 fprintf(fdbg,"DigitalSimulating: MPI Recv times:%d\n",x);
         if(DEBUG)
         fprintf(fdbg,"after MPI_Recv:\n");
         if (Nin>0)
         {
        	if(DEBUG)
        	{
        		fprintf(fdbg,"MPI Recv data:\n");
        		int i=0;
        		for(;i<Nin+1;i++)
        		{
        			fprintf(fdbg,"MPI Recv No.%d: %20.15f\n",i+1,DataSendRecv[i]);
        		}
        	}
            memcpy(&simio.Vin[simio.IdVin[i]],DataSendRecv,Nin*sizeof(double)); 

            if(DEBUG)
            {
            	fprintf(fdbg, "Print DataSendRecv value, total size is %d-----------\n",Nin);
            	for(int i=0; i<Nin;i++)
            	{
            		fprintf(fdbg, "Print DataSendRecv value No. %d = %20f\n",i,DataSendRecv[i]);
            	}
            }
            //���ӻ����յ������ݣ��ŵ������͵����ݽṹ�У�
            gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);
         }

         IsContinue=int(DataSendRecv[Nin]+0.5);
         
         if((NT-1)%INFO_PRINT_STEP==0)
         {
            sprintf(temp,"pcsubnet No.=%d, NT=%d, Nin=%d, Input:\n",procphy.pcsubnet[i],NT,Nin);
            fortran_rtprints_(temp);
            
            for(k=0;k<Nin+1;k++)
            {
               if(k==Nin)
                  sprintf(temp,"%d \n",IsContinue);
               else
                  sprintf(temp,"%f,",DataSendRecv[k]);
               fortran_rtprints_(temp);
            } 
         }
      }
      
      if (1==IsContinue) break;
      
      //printf("main function:before getdata from PID------\n");
      Ierr=gPIDITF.getdatafromPID();
      //printf("after function:before getdata from PID------\n");
      //Send output values
      for (i=0;i<simio.numSubnet;i++)
      {
         Nout=simio.IdVout[i+1]-simio.IdVout[i];

         if(DEBUG)
        	 fprintf(fdbg,"Num of data to ST process:%d\n",Nout);
         if (Nout>0)
         {
            gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);
            memcpy(DataSendRecv,&simio.Vout[simio.IdVout[i]],Nout*sizeof(double));     
         }
         DataSendRecv[Nout]=int(phyrtio.MarkRTCalculation+0.5);

         if(DEBUG)
         {
        	 fprintf(fdbg,"main function: Nout=%d  \n",Nout);
        	 //printf("main function: DataSendRecv[0]=%f  \n",DataSendRecv[0]);

        	 for(int x=0;x<=Nout;x++)
        		 fprintf(fdbg,"main function: DataSendRecv[%d]=%f  \n",x,DataSendRecv[x]);
         }

         if(DEBUG)
         fprintf(fdbg,"MPI Send to ST%%%%%%%%%%%%%%%%%%%%\n");
         if(DEBUG)
         {
        	 fprintf(fdbg,"MPI Send: print raw data to be sent:\n");
        	 int x=0;
        	 for(;x<Nout+1;x++)
        	 {
        		 fprintf(fdbg,"%02x  ",(DataSendRecv)[x]);
        		 fprintf(fdbg,"%f  ",(DataSendRecv)[x]);
        	 }
        	 fprintf(fdbg,"\n");
         }
         if(!DEBUG_NO_ST)
         {
        	 MPI_Send(DataSendRecv,Nout+1,MPI_DOUBLE_PRECISION,
                  procphy.pcsubnet[i],102,procphy.CommST);
         }
         
         if(DEBUG)
         {
        	 fprintf(fdbg,"MPI Send: print raw data to be sent:\n");
        	 int x=0;
        	 for(;x<Nout+1;x++)
        	 {
        		 fprintf(fdbg,"%02x  ",(DataSendRecv)[x]);
        		 fprintf(fdbg,"%f  ",(DataSendRecv)[x]);
        	 }
        	 fprintf(fdbg,"\n");
         }

         if((NT-1)%INFO_PRINT_STEP==0)
         {
        	 fprintf(fdbg,"pcsubnet No.=%d, NT=%d, Nout=%d, Output:\n",procphy.pcsubnet[i],NT,Nout);
            sprintf(temp,"pcsubnet No.=%d, NT=%d, Nout=%d, Output:\n",procphy.pcsubnet[i],NT,Nout);
            fortran_rtprints_(temp);
            
            for(k=0;k<Nout+1;k++)
            {
               if(k==Nout)
               {
                  sprintf(temp,"%d \n",int(phyrtio.MarkRTCalculation));
                  fprintf(fdbg,"%d \n",int(phyrtio.MarkRTCalculation));
               }
               else
               {
                  sprintf(temp,"%f,",DataSendRecv[k]);
                  fprintf(fdbg,"%f,",DataSendRecv[k]);
               }
               fortran_rtprints_(temp);
            }
         }
      }
      
      if (gPIDITF.ishaveInstantInput())
      {
    	 sprintf(temp,"%s,","ishaveInstantInput True\n");
         for (kNT=0;kNT<simio.Ikt;kNT++)  //С����
         {
            //���
            gPIDITF.updateInstantVal(NT,kNT);
            sprintf(temp,"%s,","before SenddataToPID");
            gPIDITF.SenddataToPID();
            sprintf(temp,"%s,","after SenddataToPID");
            //�ȴ����뵽��
            gPIDITF.MarkInput_Wait(0);
         }
      }
      else
      {
    	  if(DEBUG)
    	  {
    	  sprintf(temp,"%s,","ishaveInstantInput False\n");
         //���
    	  fprintf(fdbg,"Senddatato PID--\n");
    	  }
    	 struct timeval t_start,t_end;
    	 long start,end;
    	 long cost_time=0;
    	 if(DEBUG_TIME)
    	 {
    		 gettimeofday(&t_start, NULL);
    		 start = ((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000;
    	 }
         gPIDITF.SenddataToPID();
         if(DEBUG_TIME)
         {
        	 gettimeofday(&t_end, NULL);
        	 end = ((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000;
        	 //printf("End time: %ld ms\n", end);

        	 //calculate time slot
        	 cost_time = end - start;
        	 printf("Cost time: %ld ms\n", cost_time);
         }
         //�ȴ����뵽��
         if(DEBUG)
         fprintf(fdbg,"To MarkInput---\n");
         gPIDITF.MarkInput_Wait(0);

         while(DEBUG_MUL_PACKETS)
         {
        	 gPIDITF.SenddataToPID();
        	          //�ȴ����뵽��
        	 fprintf(fdbg,"To MarkInput---\n");
        	 gPIDITF.MarkInput_Wait(0);
         }

         if(DEBUG_MUL_PACKETS==2)
         {
        	   fortran_rtprints_(temp);
        	   fortran_rtprints_("End of simulating!\n");


        	   gPIDITF.ClearOutput();

        	   gPIDITF.stopSim();
        	   return;
         }
      }

      NT++;
   }
   
   fortran_rtprints_(temp);
   fprintf(fdbg," to stop simulating!\n");
   fortran_rtprints_("End of simulating!\n");
   

   gPIDITF.ClearOutput();
   
   gPIDITF.stopSim();
   
   sprintf(temp,"RTSTPhySimulating Returned!\n");
   fprintf(fdbg,"End of simulating!\n");
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

