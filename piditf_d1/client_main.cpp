/*
 * client_main.cpp
 *
 *  Created on: 2018Äê11ÔÂ7ÈÕ
 *      Author: wangy
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "cal_sock.h"
#include "MSG_SRFunc.h"
#include "MSG_SRDef.h"

#define MaxBufLen 4096

int main(int argc,char *argv[])
{
	printf("socket to be initialized!\n");
		   char CIP[100];
		   int portNO;
		   strcpy(CIP,"127.0.0.1");
		   portNO=1234;
			int Ierr=1;
			int iii=0;
			do {
				Ierr=InitConnection(CIP,portNO);
				iii=iii+1;
				printf("Connecting to GUT, try %d\n",iii);
				sleep(1);

			}while ((Ierr!=0)&&(iii<500));

                        if(Ierr!=0)
                        {
                            printf("Can not connect to %s,exit!\n",CIP);
                            return -1;
                        }
			char sline[MaxBufLen];
			int nret;
			char ss[33];
			int val_Len;
			val_Len = MaxBufLen;
			nret=ReadMessage(sline,MaxBufLen);
			if(nret>0)
			{
				for(int i=0;i<nret;i++)
					printf("%c   ",sline[i]);

			}
			else
			{
				printf("read nothing!\n");
			}
}

