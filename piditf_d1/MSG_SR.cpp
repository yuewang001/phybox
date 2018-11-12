//---------------------------------------------------------------------
//---- Send Message to GUI by Socket ----------------------------------
//---- 2017.11.21, by Qing Mu ----------------------------------------
//---------------------------------------------------------------------
#include "PMF_GlbVarsDef.h"
#include "Pub_Func.h"
#include "MSG_SRDef.h"
#include "MSG_SRFunc.h"
#include "EMTIOdef.h"
#include "EMTIOFunc.h"
#include "cal_sock.h"
void MsgSend(int Ntype,char* Databuf,int LenData,int* Ierr)
{//---- Input: Ntype,NData,LenData
	//---- Ntype: Type code of Databuf
	//---- Databuf: data to be send
	//---- LenData: Length of data, in bytes
	//---- Output: Ierr
	int Nerr;
	int Msghead[3];
	int nlen,Nret,LenSend,i,k;
	char MsgBuff[Msg_MaxLen+1];

	//生成发送报文
	Msghead[1]=Ntype;
	Msghead[2]=LenData;
	Bytecopy((char*)&Msghead[1],MsgBuff,Msg_Len_MsgHead);
	Bytecopy(Databuf,&MsgBuff[Msg_Len_MsgHead],LenData);

	//发送报文
	k=1;
	nlen=LenData+Msg_Len_MsgHead;         //报文长度
	for (int i = 1;i<=Msg_MaxSR_Times;i++)
	{
#ifdef _REALTIME
		//实时且在运行中，则通过rtfifo发送报文
		if ((g_CtrlPara.iRTvalid==1)&&(Mark_Run==g_SimSt.iMarkCalc)){
			cplusplus_fifo_write(&hdl_SocketOutGUI,&MsgBuff[k-1],&nlen,&LenSend);
		}
		else{
			LenSend=WriteMessage(&MsgBuff[k-1],nlen);
		}
#else
		//直接通过socket发送报文
		LenSend=WriteMessage(&MsgBuff[k-1],nlen);  
#endif
		k=k+LenSend;
		nlen=nlen-LenSend;

		if (nlen <= 0) break;
	}
	if (nlen > 0){
		*Ierr=MErr_Exceed_MaxSR_Times;
	}
	else
	{
		*Ierr=0;
	}
#if _DBG_LEVEL>=3
	if (LenSend>0){
		Write2FifoInfo(Hdl_F_EMTERR,2002,(double)Ntype,(double)LenSend,0.0,0.0,0.0);
		Write2FifoHex(Hdl_F_EMTERR,MsgBuff,LenSend);
	}
#endif      
}

//---------------------------------------------------------------------
//---- Recv Message head from GUI by Socket ---------------------------
//---- 2002.11.21, by Li Yalou ----------------------------------------
//---------------------------------------------------------------------
void MsgHeadRecv( int* Ntype,int* LenData)
{
	int NData[2];

	int lenRecv,Nerr;

#ifdef _REALTIME
	//实时且在运行中，通过rtfifo读数据
	if ((g_CtrlPara.iRTvalid==1)&&(Mark_Run==g_SimSt.iMarkCalc)){
		int tmp = Msg_Len_MsgHead;
		cplusplus_fifo_read(&hdl_SocketInGUI,(char*)&NData[0],&tmp,&lenRecv);
	}
	else
	{
		lenRecv=ReadMessage((char*)&NData[0],Msg_Len_MsgHead);
	}
#else
	//直接通过socket读数据
	lenRecv=ReadMessage((char*)&NData[0],Msg_Len_MsgHead);
#endif
		if (lenRecv > 0)
		{
			*Ntype=NData[0];
			*LenData=NData[1];
		}
		else
		{
			*Ntype=0;
			*LenData=0;
		}
#if _DBG_LEVEL>=3
		if (lenRecv > 0)
		{
			Write2FifoInfo(Hdl_F_EMTERR,2001,(double)Ntype[0],(double)*LenData,0.0,0.0,0.0);
		}
#endif      
}

//---------------------------------------------------------------------
//---- Recv Message body from GUI by Socket ---------------------------
//---- 2002.11.21, by Li Yalou ----------------------------------------
//---------------------------------------------------------------------
void MsgBodyRecv(char* Databuf,int *LenData,int* Ierr)
	//---- Input: Ntype,NData,LenData
	//---- Ntype: Type code of Databuf
	//---- Databuf: data to be receved
	//---- LenData: Length of data, in bytes
	//---- Output: Ierr
{
	int nlen,LenRecv,k,Nerr;

	nlen=*LenData;
	k=1;
	for (int i = 1;i<=Msg_MaxSR_Times;i++)
	{
#ifdef _REALTIME
		//实时且在运行中，通过rtfifo读数据
		if ((g_CtrlPara.iRTvalid==1)&&(Mark_Run==g_SimSt.iMarkCalc)){
			cplusplus_fifo_read(&hdl_SocketInGUI,&Databuf[k-1],&nlen,&LenRecv);
		}
		else{
			LenRecv=ReadMessage(&Databuf[k-1],nlen);
		}
#else
		//直接通过socket读数据
		LenRecv=ReadMessage(&Databuf[k-1],nlen);
#endif
		k=k+LenRecv;
		nlen=nlen-LenRecv;
		if (nlen <= 0) break;
	}
	if (nlen > 0)
	{
		*Ierr=MErr_Exceed_MaxSR_Times;
	}
	else
	{
		*Ierr=0;
	}
#if _DBG_LEVEL>=3
	Write2FifoInfo(Hdl_F_EMTERR,1006,(double)*LenData,0.0,0.0,0.0,0.0);
	Write2FifoHex(Hdl_F_EMTERR,Databuf,*LenData);
#endif   
}

//---------------------------------------------------------------------
//---- Recv Message body from GUI by Socket ---------------------------
//---- 2002.11.21, by Li Yalou ----------------------------------------
//---------------------------------------------------------------------
void MsgBodyClr(int LenData,int* Ierr)
	//---- Input: LenData
	//---- Output: Ierr
{
	int nlen,LenRecv,k,i;
	char Databuf[MaxBufLen];

	//若数据长度为0，则不处理
	if (LenData==0) return;

	nlen=LenData;
	k=1;
	//读取长度为LenData的数据，不处理
	for (int i = 1;i<=Msg_MaxSR_Times;i++)
	{
#ifdef _REALTIME
		//实时且在运行中，通过rtfifo读数据
		if ((g_CtrlPara.iRTvalid==1)&&(Mark_Run==g_SimSt.iMarkCalc)){
			cplusplus_fifo_read(&hdl_SocketInGUI,&Databuf[k-1],&nlen,&LenRecv);
		}
		else{
			LenRecv=ReadMessage(&Databuf[k-1],nlen);
		}
#else
		//直接通过socket读数据
		LenRecv=ReadMessage(&Databuf[k-1],nlen);
#endif
		k=k+LenRecv;
		nlen=nlen-LenRecv;
		if (nlen <= 0) break;
	}
	if (nlen > 0){
		*Ierr=MErr_Exceed_MaxSR_Times;
	}
	else
	{
		*Ierr=0;
	}
}

//----------------------------------------------------------------------
//---- Copy a Byte array to a Buffer -----------------------------------
//----------------------------------------------------------------------
void Bytecopy(char* s,char* t,int nlen)
{
	for (int i = 0;i<nlen;i++)
	{
		t[i]=s[i];
	}
}

