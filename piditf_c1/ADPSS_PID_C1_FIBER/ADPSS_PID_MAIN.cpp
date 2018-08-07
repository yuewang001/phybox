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
*	函数名：main
*	功能：
*		主程序。
*	修改：
*		2007-06-06	16:07:00	by ZhangXing
************************************************************************/
int main( int argc, char ** argv )
{
	//变量定义
	int i, j, k, Ierr=0;
	string phyPth;
//	char filename[256];

	// 初始化
	RtPhyInit();

//	init_rt_env_();

//	Ierr = dpdk_init();

	//MPI初始化，并获取通信域进程数和本进程标识
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &MpiId.NumProcALL);	
	MPI_Comm_rank(MPI_COMM_WORLD, &MpiId.MyId);

	//打开phy_emt.lis文件，记录程序运行信息。
	sprintf(filename, "/export/run/phy_emt_%d.lis", MpiId.MyId);
	fdbg = fopen(filename, "w");
	fprintf(fdbg, "******Physical Device Operation Information ******\n");
	fprintf(fdbg, "Total proc number: %d, my proc id: %d\n", MpiId.NumProcALL, MpiId.MyId);
	fflush(fdbg);

	//形成一些公用通信体
	Ierr = FormComm();

	init_rt_env_();

	Ierr = dpdk_init();

	if(Ierr != 0) 
	{
		printf("PHYEMT:quit abnormaly! dpdk_init ret = %d\n", Ierr);
		return -1;
	}

	//读EMTPARA.DAT，获得与板卡程序通信的EMT程序进程号
	Ierr = REmtParal();
	fprintf(fdbg, "*******Read EmtParaInfo.DAT******\n");
	fprintf(fdbg, "EmtParaInfo.DT = %f\n", EmtParaInfo.DT);
	fprintf(fdbg, "EmtParaInfo.T = %f\n", EmtParaInfo.T);
	fprintf(fdbg, "EmtParaInfo.TStart = %f\n", EmtParaInfo.TStart);
	fflush(fdbg);
	NStep = int(EmtParaInfo.T/EmtParaInfo.DT);

	// 读取alloc_in.pth文件，获取物理接口数据路径
	phyPth = ReadAllocin();

	// 读板卡通道信息
	ReadDvc(phyPth+"-B1.DVC");

	// 初始化物理接口
	Ierr = gPID.init(EmtParaInfo.DT);
	fprintf(fdbg, "gPID.init() return = %d\n", Ierr);
	fprintf(fdbg, "NStep = %d\n", NStep);
	fflush(fdbg);


	if(Ierr == -1)
	{
		fclose(fdbg); 
		dpdk_release();
		gPID.finalize();	
		printf("PHYEMT:quit abnormaly! gPID.init() ret = %d\n", Ierr);
		MPI_Finalize();
		return -1;
	}

	AllocRcdBuf();

	if (0==Ierr)
	{
//		usleep(1000*1000);

		// 打开FIFO设备
		RTDevOpenClose(1);

        // 停止所有接口箱的同步信号
        gPID.stopSync(); //是否还需要停止同步信号（目前接口装置不判断主从）

        //所有从站转换到运行状态
  //      gPID.startSlaveSim(); 


#ifndef _INFINIBAND
		// 同步计算进程
		MPI_Barrier(ProcPhy.CommCAL);

		// 对于非IB网络（Myrinet），需要采用等待同步
		// 创建实时线程后休眠至2s，避免多个实时线程创建不同步
		fortran_rtclock_gettime_(&BarrierT);
		BarrierT += 2;
#endif //_INFINIBAND

		// 创建实时线程		
		create_rt_thread_((void*)RtEmtPhy);

		printf("PHY: after create_rt_thread\n");

		// 等待至计算结束
		Wait_For_End();
		
		// 关闭FIFO设备
		RTDevOpenClose(0);

		//释放物理接口
		gPID.finalize();
	}

	fclose(fdbg); 
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	ReleaseRcdBuf();

	dpdk_release();

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
*	函数名：FormComm
*	输入：
*	功能：
*		形成通信体。
*	修改：
*		2007-01-15	15:14:00	by ZhangXing
************************************************************************/
int FormComm()
{	
	int *IDataRecv;
	int *PCbuffer;
	int i, Ntype;

	IDataRecv = new int[MpiId.NumProcALL];
	PCbuffer = new int[MpiId.NumProcALL];
	
	MPI_Group GroupAll,GroupST,GroupEMT,GroupEMTCAL,GroupCAL;		//全部进程组、机电暂态进程组、电磁暂态进程组、电磁暂态子网进程组、计算进程组
	
	fprintf(fdbg, "******Formcomm() begin******\n");
	fflush(fdbg);

	//----- 形成机电暂态、电磁暂态计算的通信体
	MPI_Comm_group(MPI_COMM_WORLD, &GroupAll);

	// 每个进程都发送自己的类型号，并收集所有进程的类型号
	Ntype=SIMUTYPE_EMTPHY;											//本进程类型号
	MPI_Allgather( &Ntype, 1, MPI_INTEGER, IDataRecv, 1, MPI_INTEGER, MPI_COMM_WORLD);

	// 计算进程通信体
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
		MPI_Group_incl(GroupAll, NumProcCAL, PCbuffer, &GroupCAL);		//形成计算进程组
		MPI_Comm_create(MPI_COMM_WORLD, GroupCAL, &ProcPhy.CommCAL);	//形成计算通信域
		fprintf(fdbg, "Comm CAL is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommCAL = MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of CAL proc: %d\n", NumProcCAL);
	fflush(fdbg);

	// 机电暂态通信体
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
		//将已存在的进程组GroupCAL中NumProcST个的生成新的进程组GroupST
		MPI_Group_incl(GroupCAL, NumProcST, PCbuffer, &GroupST);		//形成机电暂态进程组
		MPI_Comm_create(ProcPhy.CommCAL, GroupST, &ProcPhy.CommST);				//形成机电暂态通信域
		fprintf(fdbg, "Comm ST is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommST = MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of ST proc: %d\n", NumProcST);
	fflush(fdbg);

	// 电磁暂态通信体
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
		//将已存在的进程组GroupCAL中NumProcEMT个的生成新的进程组GroupEMT
		MPI_Group_incl(GroupCAL, NumProcEMT, PCbuffer, &GroupEMT);		//形成电磁暂态进程组
		MPI_Comm_create(ProcPhy.CommCAL, GroupEMT, &ProcPhy.CommEMT);			//形成电磁暂态通信域
		fprintf(fdbg, "Comm EMT is created!\n");
		fflush(fdbg);
	}
	else
	{
		ProcPhy.CommEMT=MPI_COMM_NULL;
	}
	fprintf(fdbg, "Number of EMT proc: %d\n", NumProcEMT);
	fflush(fdbg);

	//电磁暂态子网通信体（仅电磁暂态程序本身）
	int NumProcEMTCAL = 0;								//电磁暂态子网数
	for (i=0; i<MpiId.NumProcALL; i++)
	{
		if (SIMUTYPE_EMTSUB == IDataRecv[i])
		{
			PCbuffer[NumProcEMTCAL] = i-NumProcST;		//该子网进程在电磁暂态进程组中的标识号，默认不同类型进程的启动顺序与COMMSIMU.PMN中定义的顺序一致
			NumProcEMTCAL = NumProcEMTCAL + 1;
		}
	}
	if (NumProcEMTCAL>0)
	{
		//将已存在的进程组GroupEMT中NumProcEMTCAL个的生成新的进程组GroupEMTCAL
		MPI_Group_incl(GroupEMT, NumProcEMTCAL, PCbuffer, &GroupEMTCAL);		//形成电磁暂态子网进程组
		MPI_Comm_create(ProcPhy.CommEMT, GroupEMTCAL, &ProcPhy.CommEMTCAL);	//形成电磁暂态子网通信体		
	}
	else
	{
		ProcPhy.CommEMTCAL=MPI_COMM_NULL;
	}

	fprintf(fdbg, "Number of EMTCAL proc: %d\n", NumProcEMTCAL);
	fflush(fdbg);

	//释放进程组
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
*   函数名：ReadAllocin
*   功能：
*       读alloc_in.pth，返回物理接口数据路径。
*   修改：
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

    //读各电磁进程的数据路径
    i=0;
    while (filePth.getline(sline, MaxLongLineLen))
    {
        pline.exec();
        emtPth[i] = pline[0];
        //去'
        j=0;
        while (string::npos!=(k=emtPth[i].find('\'',j))) 
        {
            emtPth[i].erase(k,1);
            j=k;
        }
        //去空格
        pline.trim(emtPth[i]);
        i++;
    }
    nSub = i;
    filePth.close();

    //打印各电磁进程的数据路径
    fprintf(fdbg, "******Read alloc_in.pth******\n");
    for (i=0; i<nSub; i++)
    {
        fprintf(fdbg, "emtpath[%d] = %s \n", i, emtPth[i].c_str());
    }
    fflush(fdbg);

    //分析第一个电磁进程的数据路径，获取物理接口进程的数据路径
    phyPth = emtPth[0];
    i=0;            //i记录路径中倒数第二个'/'字符的位置
    j=0;            //j记录路径中最后一个'/'字符的位置
    while (string::npos!=(k=phyPth.find('/',j+1)))
    {
        i=j;
        j=k;
    }
    phyPth.erase(i+1,j-i-1);
    phyPth.insert(i+1, "Phy");
    fprintf(fdbg, "phyPth = %s\n", phyPth.c_str());
    fflush(fdbg);

    return phyPth;
}

/************************************************************************
*	函数名：REmtParal
*	功能：
*		读EMTPARA.DAT，获得与板卡程序通信的EMT程序进程号。
*	修改：
*		2007-06-06	16:11:00	by ZhangXing
************************************************************************/
int REmtParal()
{
	char sline[MaxLongLineLen];
	ParseString pline(sline);
	int i;

	//打开EMTPARAL.DAT文件
	ifstream filedat;
	filedat.open("./EMTPARAL.DAT");//, ios::nocreate);
	if (!filedat.is_open())
	{
		fprintf(fdbg, "No file EMTPARAL.DAT.\n");
		fflush(fdbg);
		return 1;
	}

	//读第一行数据
	filedat.getline(sline, MaxLongLineLen);
	if (pline.exec()<2)
	{
		fprintf(fdbg, "Read EMTPARAL.DAT Line1 Error!\n");
		fflush(fdbg);
		return 1;
	}
	EmtParaInfo.NPhyProc = atoi(pline[0].c_str());
	EmtParaInfo.NMatProc = atoi(pline[1].c_str());
	
	//读第二行数据
	filedat.getline(sline, MaxLongLineLen);
	if (pline.exec()<2)
	{
		fprintf(fdbg, "Read EMTPARAL.DAT Line2 Error!\n");
		fflush(fdbg);
		return 1;
	}
	EmtParaInfo.T = atof(pline[0].c_str());
	EmtParaInfo.DT = atof(pline[1].c_str());
	if (pline.count() > 2)			// 支持输出定时功能
	{
		EmtParaInfo.TStart = atof(pline[2].c_str());
	}
	else
	{
		EmtParaInfo.TStart = 0;	
	}

/*
	//读第三行以后数据
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
	return 0;
}

/************************************************************************
*	函数名：ReadDvc
*	输入：
*		filename：*-B1.DVC文件名。
*	功能：
*		读*-B1.DVC文件，获得输入输出通道信息。
*	修改：
*		2009-03-29	10:40:00	by ZhangXing
************************************************************************/
int ReadDvc(string filename)
{
	int i=0, j, nIn, nOut, MaxChNo;
	char sline[MaxLongLineLen];
	ParseString pline(sline);
	int minPIDProc=MpiId.NumProcALL;		//最小PID进程号，该进程对应的最小序号接口箱为主控

	// 打开DVC文件
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
		if ((1!=strncmp(sline, "0", 1)) && (2!=strncmp(sline, "0", 1)))	//无效
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

			//统计最小的PID进程号
			if (minPIDProc > ch.PIDProc) minPIDProc = ch.PIDProc;

			if (MpiId.MyId!=ch.PIDProc)
			{
				//该通道不是本物理接口进程对应的通道
				continue;
			}
			
			//统计输入输出通道数目
			if (1 == ch.InOut)						//仿真输出
				PIDChInfo.NChOut++;
			else									//仿真输入
				PIDChInfo.NChIn++;

			//分析该通道的计算进程信息，并写入ProcPhy.SimProc
			for (j=0; j<ProcPhy.SimProc.size(); j++)
			{
				if (ProcPhy.SimProc[j].Id == ch.SimProc)
				{
					//找到对应的计算进程信息
					break;
				}
			}
			if (j < ProcPhy.SimProc.size())
			{
				if (1 == ch.InOut)					//仿真输出
					ProcPhy.SimProc[j].nOut++;
				else								//仿真输入
					ProcPhy.SimProc[j].nIn++;
			}
			else
			{
				SSIMPROCINFO simproc;
				simproc.Id = ch.SimProc;
				simproc.nOut = 0;
				simproc.nIn = 0;

				if (1 == ch.InOut)					//仿真输出
					simproc.nOut++;
				else								//仿真输入
					simproc.nIn++;

				ProcPhy.SimProc.push_back(simproc);
			}

			//记录通道信息
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

	//逐个将通道添加到gPID中	
	for (i=0; i<ProcPhy.SimProc.size(); i++)
	{
		nIn = 0;
		nOut = 0;
		for (j=0; j<PIDChInfo.NChnl; j++)
		{
			if (PIDChInfo.ch[j].SimProc == ProcPhy.SimProc[i].Id)
			{
				if (1 == PIDChInfo.ch[j].InOut)		//仿真输出
				{
					gPID.addavar(nOut, PIDChInfo.ch[j].IdNo, PIDChInfo.ch[j].SimProc, PIDChInfo.ch[j].SigType,
							PIDChInfo.ch[j].ValueType, PIDChInfo.ch[j].VarNo, PIDChInfo.ch[j].PIDNo,
							PIDChInfo.ch[j].SlotNo, PIDChInfo.ch[j].ChNo, PIDChInfo.ch[j].Gain, PIDChInfo.ch[j].IoSpec);
					nOut++;
				}
				else								//仿真输入
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

	//为CADPSS_PID对象分配缓冲区空间，并将其push到gPID.VPID中
	gPID.allocBuf();
	gPID.pushPID();

	//设置最小PID进程的最小序号接口箱为主控
    fprintf(fdbg, "minPIDProc = %d\n", minPIDProc);
    fflush(fdbg);
    if (MpiId.MyId == minPIDProc)
    {
        gPID.setMaster();
    }
	return 0;
}


/************************************************************************
*	函数名：RtEmtPhy
*	功能：
*		实时线程，用于实时仿真。
*	修改：
*		2007-06-07	10:09:00	by ZhangXing
************************************************************************/
void *RtEmtPhy(void *arg)
{
	double StartTime, LastTime, EndTime, StepTime, MaxStep, MinStep;
	double TNow;
	int i, j, k, ITime, SleepTime, otcnt=0, maxot, Ierr;
	int SubProcCh, SubProcId, SubChNo;
	MPI_Status MpiStt;
	char sline[MaxLongLineLen];
	bool bEnd;                         //程序结束标志
	I16 ret, recv_err=0, send_err=0;
	double g_us_t=0,g_us_g=0;


#ifndef _INFINIBAND
	fortran_clock_sleepto_(&BarrierT);
	// 同步计算进程
	MPI_Barrier(ProcPhy.CommCAL);
#else //_INFINIBAND
	// Infiniband网络采用新的同步方式，避免多个实时线程创建不同步
	MPI_Barrier(ProcPhy.CommCAL);
#endif //_INFINIBAND

	printf("PHY: rt_thread, after MPI_Barrier()\n");

	// 用于统计最大和最小计算步长
	MaxStep = 0;
	MinStep = 1000000;

	TNow = EmtParaInfo.DT;
	ITime = 1;

	PhyRtIO.RTCalcMark = SPhyRtIO::MRT_Running;


	//整秒时发送启动命令
	wait_PPS_signal();


	gPID.startSlaveSim(); 
	//发送启动命令
	gPID.startMasterSim();

	//整秒以后开始数据交互
	wait_PPS_signal();

	fortran_rtclock_gettime_(&StartTime);
//	LastTime = StartTime;
	LastTime = 0;

	fprintf(fdbg, "PID START（GPS） -----> %d-%d %d:%d:%d.%d.%d \n",startTime.gtime_y,startTime.gtime_d,startTime.gtime_h,startTime.gtime_m,startTime.gtime_s,startTime.gtime_ms,startTime.gtime_us);
	fflush(fdbg);

	while (ITime <= floor(EmtParaInfo.T/EmtParaInfo.DT+0.5))
	{
		//等待物理接口更新
		ret = getSignal_NextDT(&g_us_t,&g_us_g,EmtParaInfo.DT);//该行数为阻塞函数，内部轮询等待下一个周期到达

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

            //读取采样数据，并将其发送到各计算进程	
            gPID.getdatafromPID();
            for (i=0; i<ProcPhy.SimProc.size(); i++)
            {
				gPID.setSimuVal(ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].Data);

				// 每一对计算/接口进程都是双向通讯，在仿真数据之后附带1个状态数据
				ProcPhy.SimProc[i].Data[ProcPhy.SimProc[i].nIn] = send_err;
				MPI_Send(ProcPhy.SimProc[i].Data, ProcPhy.SimProc[i].nIn+1, MPI_DOUBLE_PRECISION, ProcPhy.SimProc[i].Id, 60, MPI_COMM_WORLD);

				
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

            //从各计算进程读取数据，并将其下传至物理接口
            for (i=0; i<ProcPhy.SimProc.size(); i++)
            {
				MPI_Recv(ProcPhy.SimProc[i].Data, ProcPhy.SimProc[i].nOut+1, MPI_DOUBLE_PRECISION, ProcPhy.SimProc[i].Id, 61, MPI_COMM_WORLD, &MpiStt);
				gPID.GetSimuVal(ProcPhy.SimProc[i].Id, ProcPhy.SimProc[i].Data);

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



				//最后一个数据是状态数据，判断是否中断仿真
				recv_err = (int)(ProcPhy.SimProc[i].Data[ProcPhy.SimProc[i].nOut]+0.5);
				if (1 == recv_err) bEnd = true;
			}
//          fortran_rtclock_gettime_(&timestamp[(ITime-1)*2+5]);
            //到输出起始时间后才输出
            if (TNow >= EmtParaInfo.TStart)
            {
                gPID.SenddataToPID();
            }

            //停止仿真命令
            if (bEnd)
            {
                sprintf(sline, "PID %d recv stop signal from EMT\n", MpiId.MyId);
				WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);
                break;
            }

            // 统计最大和最小步长
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

	// 停止发送
	gPID.stopSim();

	getEndTime();
	fprintf(fdbg, "PID END  （GPS） -----> %d-%d %d:%d:%d.%d.%d \n",currTime.gtime_y,currTime.gtime_d,currTime.gtime_h,currTime.gtime_m,currTime.gtime_s,currTime.gtime_ms,currTime.gtime_us);
	fflush(fdbg);
	
	sprintf(sline, "Running Time of rt thread = %f\n", EndTime-StartTime);
	WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);

	sprintf(sline, "Max Step Time = %f, Min Step Time = %f, OverTime Times = %d, Max Step idx = %d\n", MaxStep, MinStep, otcnt, maxot);
	WriteStr2FIFO(PhyRtIO.hdl_FileError, sline);

	PhyRtIO.RTCalcMark = SPhyRtIO::MRT_End;

	return NULL;
}

// 等待实时进程计算结束，处理文件输出和通信
void Wait_For_End()
{
	while(SPhyRtIO::MRT_End!=PhyRtIO.RTCalcMark)
	{
		usleep(2000);
		DealFIFO();
	}

	//计算完毕后再输出一次
	usleep(2000);
	DealFIFO();
}

// 向记录文件中写入字符串
void WriteStr2FIFO(int hdl, char *str)
{
	int nlen,nret;
	nlen=strlen(str);

	if (SPhyRtIO::MRT_Running==PhyRtIO.RTCalcMark) 	//实时运行
	{
		nret=write(hdl,str,nlen);
	}
	else
	{
		fprintf(fdbg, "%s", str);
		fflush(fdbg);
	}
}

// 等待实时进程计算结束，处理文件输出和通信
void DealFIFO()
{
	char sline[MaxLongLineLen];
	int nret;
	
	// 屏幕信息输出
	nret=read(PhyRtIO.hdl_crtprint,sline,MaxLongLineLen);
	if (nret>0) 
	{
		sline[nret]='\0';
		printf(sline);
	}

	//错误信息输出
	do {
		nret=read(PhyRtIO.hdl_FileError,sline,MaxLongLineLen);
		if (nret>0) {
			sline[nret]='\0';
			fprintf(fdbg, "%s", sline);
			fflush(fdbg);
		}
	}while(0<nret);
}

// 实时设备的打开和关闭，包括文件和通信端口
void RTDevOpenClose(int Isopen)
{
	char sname[30];
	bool isok;
	int nret;
	
	//打开设备文件
	if (1==Isopen) 
	{		
		// 屏幕打印信息
		sprintf(sname,"crtprint%d",MpiId.MyId+1);
		sprintf(PhyRtIO.dev_crtprint,"FIFO%s",sname);		
		fortran_fifo_open_(PhyRtIO.dev_crtprint, &PhyRtIO.hdl_crtprint);
		
		// 错误信息文件
		sprintf(sname,"PhyERR%d.LIS",MpiId.MyId+1);
		sprintf(PhyRtIO.dev_FileError,"FIFO%s",sname);
		fortran_fifo_open_(PhyRtIO.dev_FileError, &PhyRtIO.hdl_FileError);
	}
	else
	{
		fortran_fifo_close_(PhyRtIO.dev_crtprint, &PhyRtIO.hdl_crtprint,&nret);
		fortran_fifo_close_(PhyRtIO.dev_FileError,&PhyRtIO.hdl_FileError,&nret);
	}
}

//实时物理装置接口初始化
void RtPhyInit(){
   PhyRtIO.RTCalcMark=SPhyRtIO::MRT_Initializing;
}
