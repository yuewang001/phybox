//FIFO����
#ifndef _MSG_SRDEF_H
#define _MSG_SRDEF_H

#define Msg_MaxLen 4096

//�Ӽ�����򵽽���
#define MT_Char 1                       //�ַ���
#define MT_EMT_ProcNo 2                 //���̺�
#define MT_EMT_Time 3                   //����ʱ��
#define MT_EMT_Status 21                //����״̬
#define MT_EMT_ConRes 23                //������Ӧ
#define MT_EMT_MonRes 24                //������Ӧ
#define MT_EMT_RcdCmdRes 31             //¼��������Ӧ
#define MT_EMT_RcdInfoRes 32            //¼����Ϣ��Ӧ
#define MT_EMT_RcdFile 33               //¼���ļ�

//�ӽ��浽�������
#define MT_EMT_T 1                      //����ʱ�䡢ʱ����ƣ����͵�������
#define MT_EMT_Cmd 21                   //����������͵�������
#define MT_EMT_ConInfo 23               //������Ϣ
#define MT_EMT_MonInfo 24               //������Ϣ
#define MT_EMT_RcdCmd 31                //¼������
#define MT_EMT_RcdInfo 32               //¼����Ϣ

// ���Ƽ�״̬��Ϣ
#define Msg_CS_Start 1                  //����
#define Msg_CS_Stop 2                   //����
#define Msg_CS_Pause 3                  //��ͣ
#define Msg_CS_Continue 4               //��������
#define Msg_CS_AbnStop 5                //�������������

//��������
#define Msg_Len_I4 4
#define Msg_Len_R4 4
#define Msg_Len_R8 8
#define Msg_Len_Char 1
#define Msg_Len_MsgHead 8
#define Msg_Len_Con 20
#define Msg_Len_Mon 12
#define Msg_MaxSR_Times 10
#define Msg_Len_RcdFile 132

//¼��״̬����
#define Rcd_Stop 0
#define Rcd_Running 1
#define Rcd_Error 2

//��ء�¼��֧�ֵ���������Ŀ������ʵʱ���治֧�ֶ�̬�ڴ���䣬��ҪԤ����
#define Max_Con_VarNum 50               //���Ʊ�����Ŀ
#define Max_Mon_VarNum 200              //���ӱ�����Ŀ
#define Max_Rcd_VarNum 500              //¼��������Ŀ   ////penghy////2012.5.25ֱ�����Ʊ�������100-->500
#define Max_Emt_NetNum 64               //�����̬������Ŀ

//¼��֧�ֵĵ�ͨ�����Ԥ¼������������real*4��ʽ�洢
#define Max_PreRcd_Len 10000

//������Ϣ
#define MErr_Type_NotDef 1
#define MErr_Exceed_MaxSR_Times 2
#pragma pack(1) /*ָ������1�ֽ�*/
	//--------------------------------------------------------------------------------
	//���Ʊ����ṹ
	typedef struct 
	{
		int CompType;
		int CompNo;
		int VarNo;
		double Val;
	}StruConVar;


	//���Ʊ��Ľṹ
		typedef struct
		{	int VarNum;
		StruConVar Var[Max_Con_VarNum];
		}StruConInfo;

	//���ӱ����ṹ
		typedef struct
		{		
			int CompType;
		int CompNo;
		int VarNo;
		}StruMonVar;


	//���ӱ��Ľṹ
		typedef struct
		{		
			int VarNum;
			StruMonVar Var[Max_Mon_VarNum];
		}StruMonInfo;

	//������Ӧ���Ľṹ
		typedef struct
		{		
			int VarNum;
			double Val[Max_Mon_VarNum];
		}StruMonRes;

	//¼����Ϣ����
		typedef struct
		{		
			int VarNum;
			int FreqRatio;
				double PreRcdTime;
			double RcdTime;
			StruMonVar Var[Max_Rcd_VarNum];
		}StruRcdInfo;

	//¼������������Ϣ
		typedef struct
		{		
			int NetId;
			int CompType;
				int CompNo;
				int VarNo;
				float Var1;
				float Var2;
		}StruRcdTrigVar;

	//¼�������
		typedef struct
		{		
			int Cmd;
			StruRcdTrigVar TrigVar;
			int RcdNet[Max_Emt_NetNum];
		}StruRcdCmd;

	//¼���ļ�
		typedef struct
		{		
			int Typ;
			char Name[257];
		}StruRcdFile;
#pragma pack () /*ȡ��ָ�����룬�ָ�ȱʡ����*/
	//--------------------------------------------------------------------------------
	//��ǰ����¼����Ϣ��ÿ���յ����淢��¼����Ϣ����ʱ���и��£�¼����ɺ�����

	extern StruRcdInfo MpRcdInfo;
	extern StruRcdCmd MpRcdCmd;
	extern StruRcdFile MpRcdFile;
	extern float RcdBuf[Max_Rcd_VarNum+2];
	extern int RcdStatus;
	extern int RcdStartTime;   //���յ�¼�������ʱ��
	extern int RcdDataLen,RcdDataCnt;
#endif
