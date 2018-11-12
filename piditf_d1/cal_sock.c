////////////////////////////////////////////
// Cal_sock.cpp
// 描述: 由暂稳计算程序调用的的动态连接库
// 作者：yy_yang	
// 日期：2002-11-17
////////////////////////////////////////////
#define _LINUX 1
#include <stdio.h>
#ifdef _WINDOWS
#include <winsock2.h>
#else 
#ifdef _LINUX
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif 
#endif 
#include "cal_sock.h"


#ifdef _WINDOWS
SOCKET sock_fd;
#else 
#ifdef _LINUX
int sock_fd =  - 1;
#endif 
#endif 
//unsigned int SERV_PORT = 9876;





////////////////////////////////
// 初始化socket连接
// 返回值：-1 失败
//        其它值 成功
////////////////////////////////


SOCKAPI InitConnection(const char *pIP_Addr, int SERV_PORT) //2006.4.12
{
   struct sockaddr_in serv_addr;
   int ret;

   // windows需要初始化socket
#ifdef _WINDOWS
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;

   wVersionRequested = MAKEWORD(2, 2);

   err = WSAStartup(wVersionRequested, &wsaData);
#ifdef _DEBUG
   printf("Initiate Windows Socket, Return Value: %d\n", err);
#endif 
   if (err != 0)
   {
      return  - 1;
   } 
#else 
#ifdef _LINUX
#endif 
#endif 

   // 建立socket
   sock_fd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _DEBUG
   printf("Call socket function, Return socket file desc: %d\n", sock_fd);
#endif 

   if ( - 1 == sock_fd)
   {
      return  - 1;
   }

   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(SERV_PORT);

   if (0 == pIP_Addr)
   {
      serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   }
   else
   {
      serv_addr.sin_addr.s_addr = inet_addr(pIP_Addr);
   }


   ret = connect(sock_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
#ifdef _DEBUG
   printf("Call connect function, Return Value: %d\n", ret);
#endif 

   if (0 != ret)
   {
      return  - 1;
   } 

#ifdef _DEBUG
   printf("Connect success!\n\n");
#endif 
   return 0;
}


////////////////////////////////
// 将信息写入socket
// 参数：pWrtMsg -- 需要发送的数据存放的缓冲区指针
//       iBuffSize -- 需要发送数据的大小
// 返回值：-1 失败
//        其它值：成功写入的字节数
////////////////////////////////
SOCKAPI WriteMessage(const char *pWrtMsg, const int iBuffSize)
{
   int ret;

   if (0 == pWrtMsg)
   {
      return  - 1;
   }

   ret = send(sock_fd, pWrtMsg, iBuffSize, 0);

#ifdef _DEBUG
   printf("Send message, Return Value: %d\n", ret);
#endif 

   return ret;
}


////////////////////////////////
// 将信息写入socket
// 参数：pReadMsg -- 读出数据存放的缓冲区指针
//       piBuffSize -- 需要读取数据的大小
// 返回值：-1 失败
//        其它值：成功实际读出的字节数
////////////////////////////////
SOCKAPI ReadMessage(char *pReadMsg, const int iBuffSize)
{
   int ret;
   fd_set rset;
   struct timeval timeover;

   if (0 == pReadMsg)
   {
      return  - 1;
   } 

   timeover.tv_sec = 0;
   timeover.tv_usec = 0;



#ifdef _DEBUG
   printf("Before select timeover.tv_usec %l \n", timeover.tv_usec);
#endif 
   FD_ZERO(&rset);
   FD_SET(sock_fd, &rset);
   select(sock_fd + 1, &rset, 0, 0, &timeover);

#ifdef _DEBUG
   printf("After select\n");
#endif 
   ret =  - 1;

   if (FD_ISSET(sock_fd, &rset))
   {
#ifdef _DEBUG
      printf("Read is Available");
#endif 
      ret = recv(sock_fd, pReadMsg, iBuffSize, 0);
   }

#ifdef _DEBUG
   printf("Read message, Return Value: %d\n", ret);
#endif 
   return ret;
}

////////////////////////////////
// 关闭socket连接
// 参数：无
// 返回值：-1 失败
//        其它值：成功
////////////////////////////////
SOCKAPI CloseConnection(void)
{
   int ret;
#ifdef _WINDOWS
   ret = closesocket(sock_fd);
#else 
#ifdef _LINUX
   ret = close(sock_fd);
#endif 
#endif 

#ifdef _DEBUG
   printf("Close socket, Return Value: %d\n", ret);
#endif 
   return ret;
}
