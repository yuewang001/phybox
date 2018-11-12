//FIFO配置
#ifndef _MSG_SRDEF_H
#define _MSG_SRDEF_H

#define Msg_MaxLen 4096

//从计算程序到界面
#define MT_Char 1                       //字符串
#define MT_EMT_ProcNo 2                 //进程号
#define MT_EMT_Time 3                   //仿真时刻
#define MT_EMT_Status 21                //计算状态
#define MT_EMT_ConRes 23                //控制响应
#define MT_EMT_MonRes 24                //监视响应
#define MT_EMT_RcdCmdRes 31             //录波命令响应
#define MT_EMT_RcdInfoRes 32            //录波信息响应
#define MT_EMT_RcdFile 33               //录波文件

//从界面到计算程序
#define MT_EMT_T 1                      //仿真时间、时间控制，发送到主进程
#define MT_EMT_Cmd 21                   //计算命令，发送到主进程
#define MT_EMT_ConInfo 23               //控制信息
#define MT_EMT_MonInfo 24               //监视信息
#define MT_EMT_RcdCmd 31                //录波命令
#define MT_EMT_RcdInfo 32               //录波信息

// 控制及状态信息
#define Msg_CS_Start 1                  //启动
#define Msg_CS_Stop 2                   //结束
#define Msg_CS_Pause 3                  //暂停
#define Msg_CS_Continue 4               //继续计算
#define Msg_CS_AbnStop 5                //程序非正常结束

//常量定义
#define Msg_Len_I4 4
#define Msg_Len_R4 4
#define Msg_Len_R8 8
#define Msg_Len_Char 1
#define Msg_Len_MsgHead 8
#define Msg_Len_Con 20
#define Msg_Len_Mon 12
#define Msg_MaxSR_Times 10
#define Msg_Len_RcdFile 132

//录波状态定义
#define Rcd_Stop 0
#define Rcd_Running 1
#define Rcd_Error 2

//监控、录波支持的最大变量数目，由于实时仿真不支持动态内存分配，需要预分配
#define Max_Con_VarNum 50               //控制变量数目
#define Max_Mon_VarNum 200              //监视变量数目
#define Max_Rcd_VarNum 500              //录波变量数目   ////penghy////2012.5.25直流控制保护试验100-->500
#define Max_Emt_NetNum 64               //电磁暂态子网数目

//录波支持的单通道最大预录波点数，采用real*4格式存储
#define Max_PreRcd_Len 10000

//错误信息
#define MErr_Type_NotDef 1
#define MErr_Exceed_MaxSR_Times 2
#pragma pack(1) /*指定对齐1字节*/
	//--------------------------------------------------------------------------------
	//控制变量结构
	typedef struct 
	{
		int CompType;
		int CompNo;
		int VarNo;
		double Val;
	}StruConVar;


	//控制报文结构
		typedef struct
		{	int VarNum;
		StruConVar Var[Max_Con_VarNum];
		}StruConInfo;

	//监视变量结构
		typedef struct
		{		
			int CompType;
		int CompNo;
		int VarNo;
		}StruMonVar;


	//监视报文结构
		typedef struct
		{		
			int VarNum;
			StruMonVar Var[Max_Mon_VarNum];
		}StruMonInfo;

	//监视响应报文结构
		typedef struct
		{		
			int VarNum;
			double Val[Max_Mon_VarNum];
		}StruMonRes;

	//录波信息报文
		typedef struct
		{		
			int VarNum;
			int FreqRatio;
				double PreRcdTime;
			double RcdTime;
			StruMonVar Var[Max_Rcd_VarNum];
		}StruRcdInfo;

	//录波触发变量信息
		typedef struct
		{		
			int NetId;
			int CompType;
				int CompNo;
				int VarNo;
				float Var1;
				float Var2;
		}StruRcdTrigVar;

	//录波命令报文
		typedef struct
		{		
			int Cmd;
			StruRcdTrigVar TrigVar;
			int RcdNet[Max_Emt_NetNum];
		}StruRcdCmd;

	//录波文件
		typedef struct
		{		
			int Typ;
			char Name[257];
		}StruRcdFile;
#pragma pack () /*取消指定对齐，恢复缺省对齐*/
	//--------------------------------------------------------------------------------
	//当前子网录波信息，每次收到界面发来录波信息报文时进行更新，录波完成后清零

	extern StruRcdInfo MpRcdInfo;
	extern StruRcdCmd MpRcdCmd;
	extern StruRcdFile MpRcdFile;
	extern float RcdBuf[Max_Rcd_VarNum+2];
	extern int RcdStatus;
	extern int RcdStartTime;   //接收到录波命令的时刻
	extern int RcdDataLen,RcdDataCnt;
#endif
