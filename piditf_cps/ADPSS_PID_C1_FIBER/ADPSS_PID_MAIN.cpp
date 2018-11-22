#include "mpi.h"
#include <stdio.h>
#include <map>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "ParseStringSTL.h"
#include "rtftoc_intel.h"
#include "ADPSS_PID_MAIN.h"

FILE* fdbg;

#define NSTEP 2

double *timestamp,*timestamp1,*timestamp2;
int *qry;
double *RcdData;
double BarrierT;
int NStep;
char filename[256];
extern struct _global_time currTime,startTime;
/************************************************************************
*	��������main
*	���ܣ�
*		������
*	�޸ģ�
*		2007-06-06	16:07:00	by ZhangXing
************************************************************************/
int main( int argc, char ** argv )
{
	//��������
	int i, j, k, Ierr=0;
	string phyPth;
//	char filename[256];

	// ��ʼ��
	RtPhyInit();

//	init_rt_env_();

//	Ierr = dpdk_init();

	//MPI��ʼ��������ȡͨ����������ͱ����̱�ʶ
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &MpiId.NumProcALL);	
	MPI_Comm_rank(MPI_COMM_WORLD, &MpiId.MyId);

	//��phy_emt.lis�ļ�����¼����������Ϣ��
	sprintf(filename, "/export/run/phy_emt_%d.lis", MpiId.MyId);
	fdbg = fopen(filename, "w");
	fprintf(fdbg, "******Physical Device Operation Information ******\n");
	fprintf(fdbg, "Total proc number: %d, my proc id: %d\n", MpiId.NumProcALL, MpiId.MyId);
	fflush(fdbg);

	//�γ�һЩ����ͨ����
	Ierr = FormComm();

	//init_rt_env_();

	//Ierr = dpdk_init();

	/*
	if(Ierr != 0) 
	{
		printf("PHYEMT:quit abnormaly! dpdk_init ret = %d\n", Ierr);
		return -1;
	}
	*/

	//��EMTPARA.DAT�������忨����ͨ�ŵ�EMT������̺�
	Ierr = REmtParal();
	fprintf(fdbg, "REmtParal function return %d\n", Ierr);
	fprintf(fdbg, "*******Read EmtParaInfo.DAT******\n");
	fprintf(fdbg, "EmtParaInfo.DT = %f\n", EmtParaInfo.DT);
	fprintf(fdbg, "EmtParaInfo.T = %f\n", EmtParaInfo.T);
	fprintf(fdbg, "EmtParaInfo.TStart = %f\n", EmtParaInfo.TStart);
	fflush(fdbg);
	NStep = int(EmtParaInfo.T/EmtParaInfo.DT);

	// ��ȡalloc_in.pth�ļ�����ȡ����ӿ�����·��
	phyPth = ReadAllocin();

	// ���忨ͨ����Ϣ
	ReadDvc(phyPth+"-B1.DVC");

	// ��ʼ������ӿ�
	Ierr = gPID.init(EmtParaInfo.DT);
	fprintf(fdbg, "gPID.init() return = %d\n", Ierr);
	fprintf(fdbg, "NStep = %d\n", NStep);
	fflush(fdbg);


	if(Ierr == -1)
	{
		fclose(fdbg); 
		//dpdk_release();
		gPID.finalize();	
		printf("PHYEMT:quit abnormaly! gPID.init() ret = %d\n", Ierr);
		MPI_Finalize();
		return -1;
	}

	AllocRcdBuf();
	fprintf(fdbg, "gPID.init() return = %d\n", Ierr);

	if (0==Ierr)
	{
//		usleep(1000*1000);

		// ��FIFO�豸
		RTDevOpenClose(1);
		fprintf(fdbg, "after RTDevOpenClose\n");
		printf("after RTDevOpenClose\n");

        // ֹͣ���нӿ����ͬ���ź�
        gPID.stopSync(); //�Ƿ���Ҫֹͣͬ���źţ�Ŀǰ�ӿ�װ�ò��ж����ӣ�
        fprintf(fdbg, "gPID.stopSync()\n");
        printf("gPID.stopSync()\n");

        //���д�վת��������״̬
  //      gPID.startSlaveSim(); 


#ifndef _INFINIBAND
		// ͬ���������
        printf("begin MPI_Barrier\n");
		//MPI_Barrier(ProcPhy.CommCAL);
		printf("after MPI_Barrier\n");

		// ���ڷ�IB���磨Myrinet������Ҫ���õȴ�ͬ��
		// ����ʵʱ�̺߳�������2s��������ʵʱ�̴߳�����ͬ��
		fortran_rtclock_gettime_(&BarrierT);
		BarrierT += 2;
#endif //_INFINIBAND

		// ����ʵʱ�߳�		
		printf("main to create_rt_thread RtEmtPhy\n");
		create_rt_thread_((void*)RtEmtPhy);

		printf("PHY: after create_rt_thread\n");

		// �ȴ����������
		Wait_For_End();
		
		// �ر�FIFO�豸
		RTDevOpenClose(0);

		//�ͷ�����ӿ�
		gPID.finalize();
	}

	fclose(fdbg); 
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	ReleaseRcdBuf();

	//dpdk_release();

	return 0;
}

void AllocRcdBuf()
{
//	if (NStep <= NSTEP)
	{
		timestamp = new double[20*10000];
		timestamp1 = new double[20*10000];
		timestamp2 = new double[20*10000];
        qry = new int[20*10000];
		RcdData = new double[20*10000*323];			// 20s, 100us, 3AO, 6DI
	}
}

void ReleaseRcdBuf()
{
	int i,j;

//	if (NStep <= NSTEP)
	{
		ofstream fts;
		fts.open("./timestamp.log");
		for(i=0;i<10*10000;i++)
		{
			sprintf(filename, "%14.6f, %14.6f, %14.6f, %d\n", timestamp[i],timestamp1[i],timestamp2[i], qry[i]);
			fts<<filename;
		}
		fts.close();
		delete[] timestamp;
		delete[] timestamp1;
		delete[] timestamp2;
        delete[] qry;

		ofstream frcd;
		frcd.open("./datarecord.log");
		for(i=0;i<20*10000;i++)
		{
			for(j=0;j<9;j++)
			{
				sprintf(filename, "%14.6f,  ", RcdData[i*323+j]);
				frcd<<filename;
			}
			sprintf(filename, "\n");
			frcd<<filename;
		}
		frcd.close();
		delete[] RcdData;
	}

}

/************************************************************************
*	��������FormComm
*	���룺
*	���ܣ�
*		�γ�ͨ���塣
*	�޸ģ�
*		2007-01-15	15:14:00	by ZhangXing
************************************************************************/
int FormComm()
{	
	int *IDataRecv;
	int *PCbuffer;
	int i, Ntype;

	IDataRecv = new int[MpiId.NumProcALL];
	PCbuffer = new int[MpiId.NumProcALL];
	
	MPI_Group GroupAll,GroupST,GroupEMT,GroupEMTCAL,GroupCAL;		//ȫ�������顢������̬�����顢�����̬�����顢�����̬���������顢���������
	
	fprintf(fdbg, "******Formcomm() begin******\n");
	fflush(fdbg);

	//----- �γɻ�����̬�������̬�����ͨ����
	MPI_Comm_group(MPI_COMM_WORLD, &GroupAll);

	// ÿ�����̶������Լ������ͺţ����ռ����н��̵����ͺ�
	Ntype=SIMUTYPE_EMTPHY;											//���������ͺ�
	MPI_Allgather( &Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);

	// �������ͨ����
	int NumProcCAL = 0;
	for (i=0; i<MpiId.NumProcALL; i++)
	{
		if (IDataRecv[i]<=200)
		{
			PCbuffer[NumProcCAL] = i;
			NumProcCAL = NumProcCAL + 1;
		}
	}
	if (NumProcCAL>0)
	{
		MPI_Group_incl(GroupAll, NumProcCAL, PCbuffer, &GroupCAL);		//�γɼ��������
		MPI_Comm_create(MPI_COMM_WORLD, GroupCAL, &ProcPhy.CommCAL);	//�γɼ���ͨ����
		fprintf(fdbg, "Comm CAL is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommCAL = MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of CAL proc: %d\n", NumProcCAL);
	fflush(fdbg);

	// ������̬ͨ����
	int NumProcST = 0;
	for (i=0; i<MpiId.NumProcALL; i++)
	{
		if (IDataRecv[i]<=100)
		{
			PCbuffer[NumProcST] = i;
			NumProcST = NumProcST + 1;            
		}
	}
	if (NumProcST>0)
	{
		//���Ѵ��ڵĽ�����GroupCAL��NumProcST���������µĽ�����GroupST
		MPI_Group_incl(GroupCAL, NumProcST, PCbuffer, &GroupST);		//�γɻ�����̬������
		MPI_Comm_create(ProcPhy.CommCAL, GroupST, &ProcPhy.CommST);				//�γɻ�����̬ͨ����
		fprintf(fdbg, "Comm ST is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommST = MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of ST proc: %d\n", NumProcST);
	fflush(fdbg);

	// �����̬ͨ����
	int NumProcEMT = 0;
	for (i=0; i<MpiId.NumProcALL; i++)
	{
		if ((IDataRecv[i]>100)&&(IDataRecv[i]<=200))
		{
			PCbuffer[NumProcEMT] = i;
			NumProcEMT = NumProcEMT + 1;
		}
	}	
	if (NumProcEMT>0)
	{
		//���Ѵ��ڵĽ�����GroupCAL��NumProcEMT���������µĽ�����GroupEMT
		MPI_Group_incl(GroupCAL, NumProcEMT, PCbuffer, &GroupEMT);		//�γɵ����̬������
		MPI_Comm_create(ProcPhy.CommCAL, GroupEMT, &ProcPhy.CommEMT);			//�γɵ����̬ͨ����
		fprintf(fdbg, "Comm EMT is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommEMT=MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of EMT proc: %d\n", NumProcEMT);
	fflush(fdbg);

	//�����̬����ͨ���壨�������̬������
	int NumProcEMTCAL = 0;								//�����̬������
	for (i=0; i<MpiId.NumProcALL; i++)
	{
		if (SIMUTYPE_EMTSUB == IDataRecv[i])
		{
			PCbuffer[NumProcEMTCAL] = i-NumProcST;		//�����������ڵ����̬�������еı�ʶ�ţ�Ĭ�ϲ�ͬ���ͽ��̵�����˳����COMMSIMU.PMN�ж����˳��һ��
			NumProcEMTCAL = NumProcEMTCAL + 1;
		}
	}
	if (NumProcEMTCAL>0)
	{
		//���Ѵ��ڵĽ�����GroupEMT��NumProcEMTCAL���������µĽ�����GroupEMTCAL
		MPI_Group_incl(GroupEMT, NumProcEMTCAL, PCbuffer, &GroupEMTCAL);		//�γɵ����̬����������
		MPI_Comm_create(ProcPhy.CommEMT, GroupEMTCAL, &ProcPhy.CommEMTCAL);	//�γɵ����̬����ͨ����		
	}
	else
	{
		ProcPhy.CommEMTCAL=MPI_COMM_NULL;
	}

	fprintf(fdbg, "Number of EMTCAL proc: %d\n", NumProcEMTCAL);
	fflush(fdbg);

	//�ͷŽ�����
    MPI_Group_free(&GroupAll);
	if(NumProcCAL>0) MPI_Group_free(&GroupCAL);
	if(NumProcST>0) MPI_Group_free(&GroupST);
	if(NumProcEMT>0) MPI_Group_free(&GroupEMT);
	if(NumProcEMTCAL>0) MPI_Group_free(&GroupEMTCAL);
	
	fprintf(fdbg, "Form communicator success!\n");
	fflush(fdbg);

	delete[] IDataRecv;
	delete[] PCbuffer;

	return 0;
}

/************************************************************************
*   ��������ReadAllocin
*   ���ܣ�
*       ��alloc_in.pth����������ӿ�����·����
*   �޸ģ�
*       2007-06-06  17:36:00    by ZhangXing
************************************************************************/
string ReadAllocin()
{
    int i, j, k, nSub;
    ifstream filePth;
    string emtPth[10], phyPth;
    char sline[MaxLongLineLen];
    ParseString pline(sline);

    filePth.open("./alloc_in.pth");
    if (!filePth.is_open())
    {
        fprintf(fdbg, "Cannot find ./alloc_in.pth\n");
        fflush(fdbg);
        return NULL;
    }

    //������Ž��̵�����·��
    i=0;
    while (filePth.getline(sline, MaxLongLineLen))
    {
        pline.exec();
        emtPth[i] = pline[0];
        //ȥ'
        j=0;
        while (string::npos!=(k=emtPth[i].find('\'',j))) 
        {
            emtPth[i].erase(k,1);
            j=k;
        }
        //ȥ�ո�
        pline.trim(emtPth[i]);
        i++;
    }
    nSub = i;
    filePth.close();

    //��ӡ����Ž��̵�����·��
    fprintf(fdbg, "******Read alloc_in.pth******\n");
    for (i=0; i<nSub; i++)
    {
        fprintf(fdbg, "emtpath[%d] = %s \n", i, emtPth[i].c_str());
    }
    fflush(fdbg);

    //������һ����Ž��̵�����·������ȡ����ӿڽ��̵�����·��
    phyPth = emtPth[0];
    i=0;            //i��¼·���е����ڶ���'/'�ַ���λ��
    j=0;            //j��¼·�������һ��'/'�ַ���λ��
    while (string::npos!=(k=phyPth.find('/',j+1)))
    {
        i=j;
        j=k;
    }
    phyPth.erase(i+1,j-i-1);
    phyPth.insert(i+1, "Phy");
    fprintf(fdbg, "phyPth = %s\n", phyPth.c_str());
    fflush(fdbg);
    fprintf(fdbg, "ReadAllocin function end!\n");
    return phyPth;
}

/************************************************************************
*	��������REmtParal
*	���ܣ�
*		��EMTPARA.DAT�������忨����ͨ�ŵ�EMT������̺š�
*	�޸ģ�
*		2007-06-06	16:11:00	by ZhangXing
************************************************************************/
int REmtParal()
{
	fprintf(fdbg, "Read EMTPARAL.DAT successfully start!!!\n");
	char sline[MaxLongLineLen];
	ParseString pline(sline);
	int i;

	//��EMTPARAL.DAT�ļ�
	ifstream filedat;
	filedat.open("./EMTPARAL.DAT");//, ios::nocreate);
	if (!filedat.is_open())
	{
		fprintf(fdbg, "No file EMTPARAL.DAT.\n");
		fflush(fdbg);
		return 1;
	}

	//����һ������
	filedat.getline(sline, MaxLongLineLen);
	if (pline.exec()<2)
	{
		fprintf(fdbg, "Read EMTPARAL.DAT Line1 Error!\n");
		fflush(fdbg);
		return 1;
	}
	EmtParaInfo.NPhyProc = atoi(pline[0].c_str());
	EmtParaInfo.NMatProc = atoi(pline[1].c_str());
	
	//���ڶ�������
	filedat.getline(sline, MaxLongLineLen);
	if (pline.exec()<2)
	{
		fprintf(fdbg, "Read EMTPARAL.DAT Line2 Error!\n");
		fflush(fdbg);
		return 1;
	}
	EmtParaInfo.T = atof(pline[0].c_str());
	EmtParaInfo.DT = atof(pline[1].c_str());
	if (pline.count() > 2)			// ֧�������ʱ����
	{
		EmtParaInfo.TStart = atof(pline[2].c_str());
	}
	else
	{
		EmtParaInfo.TStart = 0;	
	}

/*
	//���������Ժ�����
	for (i=0; i<EmtParaInfo.NPhyProc; i++)
	{
		if ((filedat.getline(sline, MaxLongLineLen))&&(pline.exec()))
		{
			EmtParaInfo.EmtProc[i] = atoi(pline[0].c_str());
			EmtParaInfo.PhyProc[i] = atoi(pline[1].c_str());
		}
		else
		{
			fprintf(fdbg, "Read EMTPARAL.DAT Line%d Error!\n", i+3);
		}
	}
*/
	filedat.close();
	fprintf(fdbg, "Read EMTPARAL.DAT successfully end\n");
	return 0;
}

/************************************************************************
*	��������ReadDvc
*	���룺
*		filename��*-B1.DVC�ļ�����
*	���ܣ�
*		��*-B1.DVC�ļ�������������ͨ����Ϣ��
*	�޸ģ�
*		2009-03-29	10:40:00	by ZhangXing
************************************************************************/
int ReadDvc(string filename)
{
	int i=0, j, nIn, nOut, MaxChNo;
	char sline[MaxLongLineLen];
	ParseString pline(sline);
	int minPIDProc=MpiId.NumProcALL;		//��СPID���̺ţ��ý��̶�Ӧ����С��Žӿ���Ϊ����

	// ��DVC�ļ�
	ifstream filedvc;
	filedvc.open(filename.c_str());
	if (!filedvc.is_open())
	{
		fprintf(fdbg, "File %s not found!\n", filename.c_str());
		fflush(fdbg);
		return 1;
	}
	fprintf(fdbg, "******Read *-B1.DVC******\n");
	fflush(fdbg);

	PIDChInfo.NChnl = 0;
	PIDChInfo.NChIn = 0;
	PIDChInfo.NChOut = 0;
	while(filedvc.getline(sline, MaxLongLineLen))
	{
		if ((1!=strncmp(sline, "0", 1)) && (2!=strncmp(sline, "0", 1)))	//��Ч
		{
			continue;
		}
		else
		{
			SCHINFO ch;

			pline.exec();
			ch.InOut = atoi(pline[0].c_str());
			ch.SimProc = atoi(pline[3].c_str());
			ch.PIDProc = atoi(pline[10].c_str());

			//ͳ����С��PID���̺�
			if (minPIDProc > ch.PIDProc) minPIDProc = ch.PIDProc;

			if (MpiId.MyId!=ch.PIDProc)
			{
				//��ͨ�����Ǳ�����ӿڽ��̶�Ӧ��ͨ��
				continue;
			}
			
			//ͳ���������ͨ����Ŀ
			if (1 == ch.InOut)						//�������
				PIDChInfo.NChOut++;
			else									//��������
				PIDChInfo.NChIn++;

			//������ͨ���ļ��������Ϣ����д��ProcPhy.SimProc
			for (j=0; j<ProcPhy.SimProc.size(); j++)
			{
				if (ProcPhy.SimProc[j].Id == ch.SimProc)
				{
					//�ҵ���Ӧ�ļ��������Ϣ
					break;
				}
			}
			if (j < ProcPhy.SimProc.size())
			{
				if (1 == ch.InOut)					//�������
					ProcPhy.SimProc[j].nOut++;
				else								//��������
					ProcPhy.SimProc[j].nIn++;
			}
			else
			{
				SSIMPROCINFO simproc;
				simproc.Id = ch.SimProc;
				simproc.nOut = 0;
				simproc.nIn = 0;

				if (1 == ch.InOut)					//�������
					simproc.nOut++;
				else								//��������
					simproc.nIn++;

				ProcPhy.SimProc.push_back(simproc);
			}

			//��¼ͨ����Ϣ
			ch.IdNo = atoi(pline[1].c_str());
			ch.PhyName = pline[2].c_str();
			ch.CompType = atoi(pline[4].c_str());
			ch.CompNo = atoi(pline[5].c_str());
			ch.SigType = atoi(pline[6].c_str());
			ch.ValueType = atoi(pline[7].c_str());
			ch.VarNo = atoi(pline[8].c_str());
			ch.VarRatio = atof(pline[9].c_str());
			ch.PIDNo = atoi(pline[11].c_str());
			ch.SlotNo = atoi(pline[12].c_str());
			ch.ChNo = atoi(pline[13].c_str());
			ch.Gain = atof(pline[14].c_str());
			ch.IoSpec = atoi(pline[15].c_str());

			fprintf(fdbg, "InOut[%d] = %d, IdNo[%d] = %d, SimProc[%d] = %d, SigType[%d] = %d, PIDProc[%d] = %d, PIDNo[%d] = %d, SlotNo[%d] = %d, ChNo[%d] = %d, Gain[%d] = %f, IoSpec[%d] = %d\n", i, ch.InOut, i, ch.IdNo, i, ch.SimProc, i, ch.SigType, i, ch.PIDProc, i, ch.PIDNo, i, ch.SlotNo, i, ch.ChNo, i, ch.Gain, i, ch.IoSpec);
			fflush(fdbg);
			i++;
			PIDChInfo.ch.push_back(ch);
		}
	}
	PIDChInfo.NChnl = PIDChInfo.ch.size();
	fprintf(fdbg, "PIDChInfo.NChnl = %d\n", PIDChInfo.NChnl);
	fprintf(fdbg, "PIDChInfo.NChIn = %d\n", PIDChInfo.NChIn);
	fprintf(fdbg, "PIDChInfo.NChOut = %d\n", PIDChInfo.NChOut);
	fflush(fdbg);
	filedvc.close();

	//�����ͨ����ӵ�gPID��	
	for (i=0; i<ProcPhy.SimProc.size(); i++)
	{
		nIn = 0;
		nOut = 0;
		for (j=0; j<PIDChInfo.NChnl; j++)
		{
			if (PIDChInfo.ch[j].SimProc == ProcPhy.SimProc[i].Id)
			{
				if (1 == PIDChInfo.ch[j].InOut)		//�������
				{
					gPID.addavar(nOut, PIDChInfo.ch[j].IdNo, PIDChInfo.ch[j].SimProc, PIDChInfo.ch[j].SigType,
							PIDChInfo.ch[j].ValueType, PIDChInfo.ch[j].VarNo, PIDChInfo.ch[j].PIDNo,
							PIDChInfo.ch[j].SlotNo, PIDChInfo.ch[j].ChNo, PIDChInfo.ch[j].Gain, PIDChInfo.ch[j].IoSpec);
					nOut++;
				}
				else								//��������
				{
					gPID.addavar(nIn, PIDChInfo.ch[j].IdNo, PIDChInfo.ch[j].SimProc, PIDChInfo.ch[j].SigType,
							PIDChInfo.ch[j].ValueType, PIDChInfo.ch[j].VarNo, PIDChInfo.ch[j].PIDNo,
							PIDChInfo.ch[j].SlotNo, PIDChInfo.ch[j].ChNo, PIDChInfo.ch[j].Gain, PIDChInfo.ch[j].IoSpec);
					nIn++;
				}
			}
		}

		fprintf(fdbg, "Sim Proc %d, nIn = %d, nOut = %d\n", ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].nIn, ProcPhy.SimProc[i].nOut);
		fprintf(fdbg, "Sim Proc %d, nIn = %d, nOut = %d\n", ProcPhy.SimProc[i].Id, nIn, nOut);
		fflush(fdbg);
	}

	//ΪCADPSS_PID������仺�����ռ䣬������push��gPID.VPID��
	gPID.allocBuf();
	gPID.pushPID();

	//������СPID���̵���С��Žӿ���Ϊ����
    fprintf(fdbg, "minPIDProc = %d\n", minPIDProc);
    fflush(fdbg);
    if (MpiId.MyId == minPIDProc)
    {
        gPID.setMaster();
    }
    fprintf(fdbg, "ReadDvc: function end\n");
	return 0;
}


/************************************************************************
*	��������RtEmtPhy
*	���ܣ�
*		ʵʱ�̣߳�����ʵʱ���档
*	�޸ģ�
*		2007-06-07	10:09:00	by ZhangXing
************************************************************************/
static void *RtEmtPhy(void *arg)
{
	printf("RtEmtPhy on new thread starts:\n");
	double StartTime, LastTime, EndTime, StepTime, MaxStep, MinStep;
	double TNow;
	int i, j, k, ITime, SleepTime, otcnt=0, maxot, Ierr;
	int SubProcCh, SubProcId, SubChNo;
	MPI_Status MpiStt;
	char sline[MaxLongLineLen];
	bool bEnd;                         //���������־
	I16 ret, recv_err=0, send_err=0;
	double g_us_t=0,g_us_g=0;


#ifndef _INFINIBAND
	fortran_clock_sleepto_(&BarrierT);
	// ͬ���������
	printf("RtEmtPhy before MPI_Barrier:\n");
	//MPI_Barrier(ProcPhy.CommCAL);
	printf("RtEmtPhy after MPI_Barrier:\n");
#else //_INFINIBAND
	// Infiniband��������µ�ͬ����ʽ��������ʵʱ�̴߳�����ͬ��
	MPI_Barrier(ProcPhy.CommCAL);
#endif //_INFINIBAND

	printf("PHY: rt_thread, after MPI_Barrier()\n");

	// ����ͳ��������С���㲽��
	MaxStep = 0;
	MinStep = 1000000;

	TNow = EmtParaInfo.DT;
	ITime = 1;

	PhyRtIO.RTCalcMark = SPhyRtIO::MRT_Running;


	//����ʱ������������
	//wait_PPS_signal();


	gPID.startSlaveSim(); 
	//������������
	gPID.startMasterSim();

	//�����Ժ�ʼ���ݽ���
	//wait_PPS_signal();
	sleep(1);

	fortran_rtclock_gettime_(&StartTime);
//	LastTime = StartTime;
	LastTime = 0;

	fprintf(fdbg, "PID START��GPS�� -----> %d-%d %d:%d:%d.%d.%d \n",startTime.gtime_y,startTime.gtime_d,startTime.gtime_h,startTime.gtime_m,startTime.gtime_s,startTime.gtime_ms,startTime.gtime_us);
	fflush(fdbg);

	while (ITime <= floor(EmtParaInfo.T/EmtParaInfo.DT+0.5))
	{
		//�ȴ�����ӿڸ���
		printf("main function before get Signal_NextDT\n");
		sleep(1);
		//ret = getSignal_NextDT(&g_us_t,&g_us_g,EmtParaInfo.DT);//������Ϊ�����������ڲ���ѯ�ȴ���һ�����ڵ���
		printf("main function after get Signal_NextDT\n");
		if(1)
		{
		//	fortran_rtclock_gettime_(&StepTime);
			StepTime = g_us_t;
//          timestamp[(ITime-1)*2+4]=StepTime;
            if ((ITime>=360*10000) && (ITime<380*10000))			// 360s ~ 380s
            {
                timestamp[(ITime-360*10000)]=g_us_t;
				timestamp1[(ITime-360*10000)]=g_us_g;
				timestamp2[(ITime-360*10000)]=g_us_t-g_us_g;
                qry[(ITime-360*10000)]=ret;
            }


			int ii=0,jj=0;

			printf("main function before gPID.getdatafromPID\n");

            //��ȡ�������ݣ������䷢�͵����������	
            gPID.getdatafromPID();
            printf("main function after gPID.getdatafromPID\n");
            for (i=0; i<ProcPhy.SimProc.size(); i++)
            {
				gPID.setSimuVal(ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].Data);

				// ÿһ�Լ���/�ӿڽ��̶���˫��ͨѶ���ڷ�������֮�󸽴�1��״̬����
				ProcPhy.SimProc[i].Data[ProcPhy.SimProc[i].nIn] = send_err;
				printf("before MPI Send\n");
				MPI_Send(ProcPhy.SimProc[i].Data, ProcPhy.SimProc[i].nIn+1, MPI_DOUBLE_PRECISION, ProcPhy.SimProc[i].Id, 60, MPI_COMM_WORLD);
				printf("after MPI Send\n");
				
				if ((ITime>=360*10000) && (ITime<380*10000))			// 360s ~ 380s
				{
					/*
					for(ii=0;ii<ProcPhy.SimProc[i].nIn;ii++)
					{
						RcdData[(ITime-360*10000)*323+jj] = (float)ProcPhy.SimProc[i].Data[ii];
						jj++;
					}
	*/
					if (i==0)
					{
						RcdData[(ITime-360*10000)*9] = ProcPhy.SimProc[i].Data[25];
						RcdData[(ITime-360*10000)*9+1] = ProcPhy.SimProc[i].Data[26];
						RcdData[(ITime-360*10000)*9+2] = ProcPhy.SimProc[i].Data[27];
					}
					else
					{
						RcdData[(ITime-360*10000)*9+3] = ProcPhy.SimProc[i].Data[25];
						RcdData[(ITime-360*10000)*9+4] = ProcPhy.SimProc[i].Data[26];
						RcdData[(ITime-360*10000)*9+5] = ProcPhy.SimProc[i].Data[27];
					}
                }
            }

            //�Ӹ�������̶�ȡ���ݣ��������´�������ӿ�
            for (i=0; i<ProcPhy.SimProc.size(); i++)
            {
            	printf("before MPI Recv\n");
				MPI_Recv(ProcPhy.SimProc[i].Data, ProcPhy.SimProc[i].nOut+1, MPI_DOUBLE_PRECISION, ProcPhy.SimProc[i].Id, 61, MPI_COMM_WORLD, &MpiStt);
				printf("after MPI Recv\n");
				gPID.GetSimuVal(ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].Data);
				printf("after gPID.GetSimuVal\n");
				if ((ITime>=360*10000) && (ITime<380*10000))			// 360s ~ 380s
				{
					
					if (i==0)
						RcdData[(ITime-360*10000)*9+6] = ProcPhy.SimProc[i].Data[9];
					else
						RcdData[(ITime-360*10000)*9+7] = ProcPhy.SimProc[i].Data[10];

					/*
					for(ii=0;ii<ProcPhy.SimProc[i].nOut;ii++)
					{
						RcdData[(ITime-360*10000)*323+jj] = (float)ProcPhy.SimProc[i].Data[ii];
						jj++;
					}		*/				
				}


				printf("before just status if to stop sim\n");
				//���һ��������״̬���ݣ��ж��Ƿ��жϷ���
				recv_err = (int)(ProcPhy.SimProc[i].Data[ProcPhy.SimProc[i].nOut]+0.5);
				if (1 == recv_err) bEnd = true;
			}
//          fortran_rtclock_gettime_(&timestamp[(ITime-1)*2+5]);
            //�������ʼʱ�������
            printf("before just TNow >= EmtParaInfo.TStart \n");
            if (TNow >= EmtParaInfo.TStart)
            {
                gPID.SenddataToPID();
            }

            printf("after gPID.SenddataToPID \n");

            //ֹͣ��������
            if (bEnd)
            {
            	printf("stop sim \n");
                sprintf(sline, "PID %d recv stop signal from EMT\n", MpiId.MyId);
				WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);
                break;
            }

            // ͳ��������С����
            if (StepTime-LastTime > MaxStep)
            {
                MaxStep = StepTime-LastTime;
                maxot=ITime;
            }
            else if (StepTime-LastTime < MinStep)
            {
                MinStep = StepTime-LastTime;
            }
            if (StepTime-LastTime > 1.2*1000000*EmtParaInfo.DT)
            {
                otcnt++;
            }
            LastTime = StepTime;

            ITime++;

            TNow = ITime*EmtParaInfo.DT;
        }
    }
    fortran_rtclock_gettime_(&EndTime);


	/*
	for (k=0; k<10; k++)
	{
		ret = gPID.MarkInput_Wait(0);
		if (ret > 0)
		{
			for (i=0; i<ProcPhy.SimProc.size(); i++)
			{
				for (j=0; j<ProcPhy.SimProc[i].nOut; j++) ProcPhy.SimProc[i].Data[j] = 0;
				gPID.GetSimuVal(ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].Data);
			}
			gPID.SenddataToPID();
		}
	}
	*/

	// ֹͣ����
	gPID.stopSim();

	getEndTime();
	fprintf(fdbg, "PID END  ��GPS�� -----> %d-%d %d:%d:%d.%d.%d \n",currTime.gtime_y,currTime.gtime_d,currTime.gtime_h,currTime.gtime_m,currTime.gtime_s,currTime.gtime_ms,currTime.gtime_us);
	fflush(fdbg);
	
	sprintf(sline, "Running Time of rt thread = %f\n", EndTime-StartTime);
	WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);

	sprintf(sline, "Max Step Time = %f, Min Step Time = %f, OverTime Times = %d, Max Step idx = %d\n", MaxStep, MinStep, otcnt, maxot);
	WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);

	PhyRtIO.RTCalcMark = SPhyRtIO::MRT_End;

	return NULL;
}

// �ȴ�ʵʱ���̼�������������ļ������ͨ��
void Wait_For_End()
{
	while(SPhyRtIO::MRT_End!=PhyRtIO.RTCalcMark)
	{
		usleep(2000);
		DealFIFO();
	}

	//������Ϻ������һ��
	usleep(2000);
	DealFIFO();
}

// ���¼�ļ���д���ַ���
void WriteStr2FIFO(int hdl, char *str)
{
	int nlen,nret;
	nlen=strlen(str);

	if (SPhyRtIO::MRT_Running==PhyRtIO.RTCalcMark) 	//ʵʱ����
	{
		nret=write(hdl,str,nlen);
	}
	else
	{
		fprintf(fdbg, "%s", str);
		fflush(fdbg);
	}
}

// �ȴ�ʵʱ���̼�������������ļ������ͨ��
void DealFIFO()
{
	char sline[MaxLongLineLen];
	int nret;
	
	// ��Ļ��Ϣ���
	nret=read(PhyRtIO.hdl_crtprint,sline,MaxLongLineLen);
	if (nret>0) 
	{
		sline[nret]='\0';
		printf(sline);
	}

	//������Ϣ���
	do {
		nret=read(PhyRtIO.hdl_FileError,sline,MaxLongLineLen);
		if (nret>0) {
			sline[nret]='\0';
			fprintf(fdbg, "%s", sline);
			fflush(fdbg);
		}
	}while(0<nret);
}

// ʵʱ�豸�Ĵ򿪺͹رգ������ļ���ͨ�Ŷ˿�
void RTDevOpenClose(int Isopen)
{
	char sname[30];
	bool isok;
	int nret;
	

	//���豸�ļ�
	if (1==Isopen) 
	{		
		// ��Ļ��ӡ��Ϣ
		printf("break point 1:\n");
		sprintf(sname,"crtprint%d",MpiId.MyId+1);
		printf("break point 2:\n");
		sprintf(PhyRtIO.dev_crtprint,"FIFO%s",sname);		
		printf("break point 3:\n");
		fortran_fifo_open_(PhyRtIO.dev_crtprint, &PhyRtIO.hdl_crtprint);
		printf("break point 4:\n");
		
		// ������Ϣ�ļ�
		sprintf(sname,"PhyERR%d.LIS",MpiId.MyId+1);
		printf("break point 5:\n");
		sprintf(PhyRtIO.dev_FileError,"FIFO%s",sname);
		printf("break point 6:\n");
		fortran_fifo_open_(PhyRtIO.dev_FileError, &PhyRtIO.hdl_FileError);
		printf("break point 7:\n");
	}
	else
	{
		printf("break point 8:\n");
		fortran_fifo_close_(PhyRtIO.dev_crtprint, &PhyRtIO.hdl_crtprint,&nret);
		fortran_fifo_close_(PhyRtIO.dev_FileError,&PhyRtIO.hdl_FileError,&nret);
		printf("break point 9:\n");
	}
	printf("break point 11:\n");
}

//ʵʱ����װ�ýӿڳ�ʼ��
void RtPhyInit(){
   PhyRtIO.RTCalcMark=SPhyRtIO::MRT_Initializing;
}
