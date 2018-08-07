// PstPIDItf.cpp : �������̨Ӧ�ó������ڵ㡣
//

//#include "stdafx.h"
#include "mpi.h"
#include <stdio.h>
#include <algorithm>
#include "PstPIDITF.h"
#include "math.h"

#define DEBUG_W_BOX 0
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
const double DTINST=1.0e-4;	//100us

//��ʼ���Ĵ���
enum {PhyInit_Start,PhyInit_Ok,PhyInit_Stop,PhyInit_Over} MarkPhyInit;

int main(int argc,char * argv[])
{
	int i,Nin,Nout;
	MPI_Status mpistat;
	int IsBalance;
	int k,kNT;
	int NT;
	rtphyinit();
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&mpiid.numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&mpiid.myid);
	char fname[32];
	char str_dbg[256];
	sprintf(fname,"PhyERR%d.LIS",mpiid.myid+1);
	fdbg=fopen(fname,"w");
	if (NULL==fdbg) {
		printf("Fail to open file %s",fname);
		return 1;
	}
	sprintf(str_dbg,"Process %d of %d start to run...\n",mpiid.myid, mpiid.numprocs);
	fputs(str_dbg,fdbg);
	fputs(str_dbg,stdout);
	//�γ�һЩ����ͨ����
	int Ierr=0;
	Ierr=FormComm();
	//�����ؽ���IO��Ϣ�����ýӿ����ݣ���������������ص�IO����
	Ierr=RecvSendPhyIO();
	fprintf(fdbg,"After RecvSendPhyIO, return %d .\n",Ierr);
	//�����ӿڳ�ʼ��
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
	if (Ierr<0) Ierr=-Ierr;
	SubDealParaErr(Ierr);
	fprintf(fdbg,"After SubDealParaErr,Ierr=%d \n ",Ierr);
	fflush(fdbg);
	if (Ierr!=0) goto LabelEND;
	//��ʼ����
	//----- Caculate ����װ�� initial values -----------------------
	// Receive input values
	for (i=0;i<simio.numSubnet;i++)
	{
		Nin=simio.IdVin[i+1]-simio.IdVin[i];
		if (Nin>0)
		{
			fprintf(fdbg,"Before MPI_Recv simio.IdVin[%d]=%d,pcsubnet=%d,Nin=%d,procphy.CommST=%d\n",i,simio.IdVin[i],procphy.pcsubnet[i],Nin,procphy.CommST);
			fflush(fdbg);
			MPI_Recv(&simio.Vin[simio.IdVin[i]], Nin+1, MPI_DOUBLE_PRECISION, procphy.pcsubnet[i],101, procphy.CommST, &mpistat);
			fprintf(fdbg,"Before GetSimuVal \n ");
			fflush(fdbg);
			gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);
			//����˲ʱֵ�������ʷֵ
			gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);
			fprintf(fdbg,"After GetSimuVal \n ");
			fflush(fdbg);
		}
	}
	Nin=simio.IdVin[simio.numSubnet];
	fprintf(fdbg,"Nin=%d \n Val_in()=",Nin);
	for (i=0;i<Nin;i++) {
		fprintf(fdbg,"%f,",simio.Vin[i]);
	}
	fputs("\n",fdbg);
	fflush(fdbg);
	gPIDITF.MarkInput_Clear();



	for (k=0;;k++) {
		if (gPIDITF.ishaveInstantInput())
		{
			for (kNT=0;kNT<simio.Ikt;kNT++)	//С����
			{
				while(DEBUG_W_BOX)
				{
					gPIDITF.updateInstantVal(k,kNT);
					gPIDITF.SenddataToPID();
				}
				//���
				gPIDITF.updateInstantVal(k,kNT);
				gPIDITF.SenddataToPID();
				//�ȴ����뵽��
				gPIDITF.MarkInput_Wait(0);
				gPIDITF.MarkInput_Clear();
			}
		}else{
			gPIDITF.SenddataToPID();
			//�ȴ����뵽��
			gPIDITF.MarkInput_Wait(0);
			gPIDITF.MarkInput_Clear();
		}
		Ierr=gPIDITF.getdatafromPID();
		for (i=0;i<simio.numSubnet;i++)
		{
			Nout=simio.IdVout[i+1]-simio.IdVout[i];
			gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);
		}
		Nout=simio.IdVout[simio.numSubnet];
		//fprintf(fdbg,"Nout=%d \n Vout()=",Nout);
		//for (i=0;i<Nout;i++) {
		//	fprintf(fdbg,"%f,",simio.Vout[i]);
		//}
		//fputs("\n",fdbg);
		double dd=0;
		for (i=0;i<Nout;i++) {
			if (fabs(simio.Vlast[i]-simio.Vout[i])>dd) {
				dd=fabs(simio.Vlast[i]-simio.Vout[i]);
			}
			simio.Vlast[i]=simio.Vout[i];
		}
		if (dd<0.01) {
			MarkPhyInit=PhyInit_Ok;
			IsBalance=1;
			break;
		}else if (k>1000) {
			MarkPhyInit=PhyInit_Ok;
			IsBalance=0;
		}
	}
	MarkPhyInit=PhyInit_Over;
	// ��ֵƽ�����
	//Send output values
	for (i=0;i<simio.numSubnet;i++)
	{
		Nout=simio.IdVout[i+1]-simio.IdVout[i];
		if (Nout>0)
		{
			MPI_Send(&simio.Vout[simio.IdVout[i]],Nout,MPI_DOUBLE_PRECISION,procphy.pcsubnet[i],102,procphy.CommST);
		}
	}
	if (1!=IsBalance) Ierr=1;
	SubDealParaErr(Ierr);
	if (0!=Ierr) {
		fputs("Fail to initialization in Physical Device!",fdbg);
		puts("Fail to initialization in Physical Device!");
		//MarkPhyInit=PhyInit_Stop;
		//if (phyrtio.MarkRTCalculation==SPhyRTIO::MRT_Running) {
		//	phyrtio.MarkRTCalculation=SPhyRTIO::MRT_End;
		//	RTDevOpenClose(0);
		//}
		goto LabelEND;
	}
	//�յ�������̷��͵Ŀ�ʼ�����������
	for (i=0;i<simio.numSubnet;i++)
	{
		MPI_Recv(&k, 1, MPI_INTEGER, procphy.pcsubnet[i],1, procphy.CommST, &mpistat);
	}
	MarkPhyInit=PhyInit_Stop;
	//�������
	//----- Integral part --------------------------------
	NT=0;
	//kDT,kNT
	//rtl_printf("kDT=%f kNT=%d \n",kDT,kNT);
	fputs("IsContinue,Vin,Vout \n",fdbg);
	//	   fortran_rtclock_gettime_(&Tstart);
	while (TRUE){
		fprintf(fdbg,"NT=%d\n",NT);
		//sprintf(sline,"NT=%d\n",NT);
		//Writestr2FIFO(phyrtio.hdl_FileError,sline);
		int IsContinue=0;
		kNT=0;
		// Receive input values and IsContinue
		// Val_in(),KIT, IsCOntinue 
		for (i=0;i<simio.numSubnet;i++)
		{
			Nin=simio.IdVin[i+1]-simio.IdVin[i];
			if (Nin>0)
			{
				MPI_Recv(&simio.Vin[simio.IdVin[i]], Nin+1, MPI_DOUBLE_PRECISION, procphy.pcsubnet[i],101, procphy.CommST, &mpistat);
				gPIDITF.GetSimuVal(procphy.pcsubnet[i],&simio.Vin[simio.IdVin[i]]);
			}
			IsContinue=int(simio.Vin[simio.IdVin[i]+Nin]+0.5);
		}
		Nin=simio.IdVin[simio.numSubnet];
		fprintf(fdbg,"Nin=%d \n Val_in()=",Nin);
		for (i=0;i<Nin;i++) {
			fprintf(fdbg,"%f,",simio.Vin[i]);
		}
		fputs("\n",fdbg);
		if (1==IsContinue) break;
		if (gPIDITF.ishaveInstantInput())
		{
			for (kNT=0;kNT<simio.Ikt;kNT++)	//С����
			{
				//���
				gPIDITF.updateInstantVal(NT,kNT);
				gPIDITF.SenddataToPID();
				//�ȴ����뵽��
				gPIDITF.MarkInput_Wait(0);
				gPIDITF.MarkInput_Clear();
			}
		}else{
			//���
			gPIDITF.SenddataToPID();
			//�ȴ����뵽��
			gPIDITF.MarkInput_Wait(0);
			gPIDITF.MarkInput_Clear();
		}
		Ierr=gPIDITF.getdatafromPID();
		//Send output values
		for (i=0;i<simio.numSubnet;i++)
		{
			Nout=simio.IdVout[i+1]-simio.IdVout[i];
			if (Nout>0)
			{
				gPIDITF.setSimuVal(procphy.pcsubnet[i],&simio.Vout[simio.IdVout[i]]);
				MPI_Send(&simio.Vout[simio.IdVout[i]],Nout,MPI_DOUBLE_PRECISION,procphy.pcsubnet[i],102,procphy.CommST);
			}
		}
		//��ӡ
		//Nout=simio.IdVout[simio.numSubnet];
		//fprintf(fdbg,"Nout=%d \n Valout=",Nout);
		//for (i=0;i<Nout;i++) {
		//	fprintf(fdbg,"%f,",simio.Vout[i]);
		//}
		//fputs("\n",fdbg);
		//		sprintf(sline,"%d,",NT);
		//		Writestr2FIFO(phyrtio.hdl_FileError,sline);
		//		for (i=0;i<Nin;i++) {
		//		   sprintf(sline,"%f,",val_in[i]);
		//   		Writestr2FIFO(phyrtio.hdl_FileError,sline);
		//		}
		//		for (i=0;i<Nout;i++) {
		//		   sprintf(sline,"%f,",val_out[i]);
		//   		Writestr2FIFO(phyrtio.hdl_FileError,sline);
		//		}
		//		Writestr2FIFO(phyrtio.hdl_FileError,"\n");
		//        flog<< IsContinue<<",";
		//		for (i=0;i<Nin;i++) {
		//			flog<<val_in[i]<<",";
		//		}
		// 		for (i=0;i<Nout;i++) {
		//			flog<<val_out[i]<<",";
		//		}
		//		flog<<endl;
		NT++;
	}
	//��������
	//		for (n=0;n<Vphy.size();n++) {
	//			Vphy[n].Finilize();
	//		}
	phyrtio.MarkRTCalculation=SPhyRTIO::MRT_End;
//����MPI
LabelEND: 
	delete []simio.Vin;
	delete []simio.Vout;
	delete []simio.Vlast;
	fclose(fdbg);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}

//ʵʱ����װ�ýӿڳ�ʼ��
void rtphyinit(){
	//lock_pages_();
	phyrtio.MarkRTCalculation=SPhyRTIO::MRT_Initializing;
}
//�γɻ��硢���ͨѶ��
int FormComm(){
    int *IDataSend,*IDataRecv,*Ncount,*NPos;
	int *PCbuffer;
	int i;
	MPI_Group GroupAll,GrouPCMain_Sub,GroupST,GroupEMT;
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
	// ������̬ͨ����
	int NumProcST=0;
	for (i=0;i<mpiid.numprocs;i++){
		if (IDataRecv[i]<=100) {
			NumProcST=NumProcST+1;
			PCbuffer[NumProcST-1]=i;
		}
	}
	if (NumProcST>0) {
		MPI_Group_incl(GroupAll,NumProcST,PCbuffer,&GroupST);
		MPI_Comm_create(MPI_COMM_WORLD,GroupST,&procphy.CommST);
		procphy.PCMain=0;
		MPI_Comm_size(procphy.CommST,&procphy.numprocst);
		MPI_Comm_rank(procphy.CommST,&procphy.myidst);
	}else{
		procphy.CommST=MPI_COMM_NULL;
	}
	// �����̬ͨ����
	int NumProcEMT=0;
	for (i=0;i<mpiid.numprocs;i++){
		if (IDataRecv[i]>100) {
			NumProcEMT=NumProcEMT+1;
			PCbuffer[NumProcEMT-1]=i;
		}
	}
	if (NumProcEMT>0) {
		MPI_Group_incl(GroupAll,NumProcEMT,PCbuffer,&GroupEMT);
		MPI_Comm_create(MPI_COMM_WORLD,GroupST,&CommEMT);
	}
	MPI_Group_free( &GroupAll);
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
int RecvSendPhyIO(){
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
		fprintf(fdbg,"%d,%s,%d,%d,%s,%d,%d,%d,%d,%d,%f \n",
			iovars[i].MarkIO,iovars[i].PHY_Name,iovars[i].NetProcNo,iovars[i].CompType,iovars[i].CompName,
			iovars[i].ValueType,iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
	}
	fflush(fdbg);
	double *pdouble;
	pdouble=(double*) (databuff+sizeof(PSTPIDIOVAR)*NumVar);
	simio.dt=*pdouble;
	pdouble++;
	simio.fhz=*pdouble;
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
	//���������ӿڵ������������
	for (n=0;n<simio.numSubnet;n++) {
		Nin=0;
		Nout=0;
		for (i=0;i<NumVar;i++) {
			if (iovars[i].NetProcNo==procphy.pcsubnet[n]) {	//��ͬ����
				if (1==iovars[i].MarkIO) {	//����
					gPIDITF.addavar(Nin,iovars[i].idno,iovars[i].NetProcNo,iovars[i].SignalType,iovars[i].ValueType,
						iovars[i].VarNo,iovars[i].PIBNo,iovars[i].SlotNo,iovars[i].SignNo,iovars[i].KAmplify);
					Nin++;
				}else{	//���
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
    simio.Vin=new double[simio.IdVin[n]+1];
	simio.Vout=new double[simio.IdVout[n]];
	simio.Vlast=new double[simio.IdVout[n]];
	if (simio.dt>DTINST) {
		simio.Ikt=int(simio.dt/DTINST+0.5);
	}else{
		simio.Ikt=1;
	}
	fprintf(fdbg,"simio.Ikt=%d\n",simio.Ikt);
	delete []databuff;
	delete []stnetios;

	return 0;
}
//��������Ϣ�ڲ��г���䷢��
void SubDealParaErr(int &ErrorNo){
	int *IDataRecv=new int[mpiid.numprocs];
	//flog<<"Before MPI_Gather ErrorNo="<<ErrorNo<<endl;
	// send ErrorNo to Mainctrl
	MPI_Gather(&ErrorNo, 1, MPI_INTEGER,IDataRecv, mpiid.numprocs, MPI_INTEGER,procphy.PCMain, procphy.CommST);
	//flog<<"After MPI_Gather ErrorNo="<<ErrorNo<<endl;
	// wait for ErrorNo from Mainctrl
	MPI_Bcast(&ErrorNo,1,MPI_INTEGER,procphy.PCMain,procphy.CommST);
	//flog<<"After MPI_Bcast ErrorNo="<<ErrorNo<<endl;
	delete []IDataRecv;
}