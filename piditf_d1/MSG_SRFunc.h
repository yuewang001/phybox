//FIFO≈‰÷√
#ifndef _MSG_SRFUNC_H
#define _MSG_SRFUNC_H
#ifdef __cplusplus
extern "C"
{
#endif

	extern  void Bytecopy(char* s,char* t,int nlen);
extern void MsgSend(int Ntype,char* Databuf,int LenData,int* Ierr);
extern void MsgHeadRecv( int* Ntype,int* LenData);
extern void MsgBodyRecv(char* Databuf,int *LenData,int* Ierr);
extern void MsgBodyClr(int LenData,int* Ierr);

#ifdef __cplusplus
};
#endif
#endif
