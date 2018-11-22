////////////////////////////////////////////
// Cal_sock.cpp
// ����: �����ȼ��������õĵĶ�̬���ӿ�
// ���ߣ�yy_yang	
// ���ڣ�2002-11-17
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
static int sock_fd =  - 1;
#endif 
#endif 
//unsigned int SERV_PORT = 9876;





////////////////////////////////
// ��ʼ��socket����
// ����ֵ��-1 ʧ��
//        ����ֵ �ɹ�
////////////////////////////////


SOCKAPI InitConnection(const char *pIP_Addr, int SERV_PORT) //2006.4.12
{
   struct sockaddr_in serv_addr;
   int ret;

   // windows��Ҫ��ʼ��socket
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

   // ����socket
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
// ����Ϣд��socket
// ������pWrtMsg -- ��Ҫ���͵����ݴ�ŵĻ�����ָ��
//       iBuffSize -- ��Ҫ�������ݵĴ�С
// ����ֵ��-1 ʧ��
//        ����ֵ���ɹ�д����ֽ���
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
// ����Ϣд��socket
// ������pReadMsg -- �������ݴ�ŵĻ�����ָ��
//       piBuffSize -- ��Ҫ��ȡ���ݵĴ�С
// ����ֵ��-1 ʧ��
//        ����ֵ���ɹ�ʵ�ʶ������ֽ���
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
// �ر�socket����
// ��������
// ����ֵ��-1 ʧ��
//        ����ֵ���ɹ�
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
