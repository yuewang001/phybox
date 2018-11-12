#ifndef _CAL_SOCK_H
#define _CAL_SOCK_H
//#define SOCKAPI extern "C" __declspec(dllexport) int __stdcall 

#define SOCKAPI int
#ifdef __cplusplus
extern "C"
{
#endif
//////////////////////////
// 共输出三个函数
extern SOCKAPI InitConnection(const char *pIP_Addr, int SERV_PORT);
extern SOCKAPI WriteMessage(const char *pWrtMsg, const int iBuffSize);
extern SOCKAPI ReadMessage(char *pReadMsg, const int iBuffSize);
extern SOCKAPI CloseConnection(void);
//////////////////////////
#ifdef __cplusplus
};
#endif
#endif
