//#define __RTVM_POLLUTED_APP__
//#define __KERNEL__

#include "yjdnet.h"
#include "yjdioctl.h"
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "cal_sock.h"
#include "MSG_SRFunc.h"
#include "MSG_SRDef.h"
#include "debug_config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PACK_LENGTH 4096 
#define MAX_COUNT 65535
#define DEFAULT_REC_PORT 8001
#define MAXLINE 4096


//#include <gpos_bridge/sys/gpos.h>

extern FILE *fdbg;
extern FILE *Recfdbg;

struct ctl_pkt {
	U16 head[2];
	U16 reserved;
	U16 data_len;
	U16 stamp[4];
	U16 order;
	U16	frm_head;
	U8  card;
	U8  func;
	U16 reg_addr;
	U16 reg_cnt;
	U16	data[512];
};

//发送数据包结构
struct sim_pkt {
	U16 head[2];
	U16 reserved;
	U16 data_len;
	U16 stamp[4];
	U16 order;
	U16	frm_head;
	U16	data[512];
};

struct fiber_pkt{
	U16 type;
	U16 total_size;
	U16 current_pkt_no;
	U16 analog_data_count;
};


enum {
	//FUNC_TYPE_UP = 0x00,
	//FUNC_TYPE_DOWN,
	FUNC_TYPE_READ  = 0x00,
	FUNC_TYPE_WRITE = 0X01,

	SEL_SLOT_0 = 0x00,
	SEL_SLOT_1,
	SEL_SLOT_2,
	SEL_SLOT_3,
	SEL_SLOT_4,
	SEL_SLOT_5,
	SEL_SLOT_6,
	SEL_SLOT_7,
	SEL_SLOT_8,
	SEL_SLOT_9,
	SEL_MASTER = 0x80
};

#define BUFFER_RX_SG	16
#define ENET_FRAME_HEAD_LEN 14

struct  dev_buff {

	//U32: 8 bytes
	//U16: 4 bytes
	//

	float ai_data[MAX_AI_CHANS]; //4bytes
	float	ao_data[MAX_AO_CHANS]; //4 byte
	unsigned char	di_data[MAX_DI_CHANS]; //1 byte
	unsigned char	do_data[MAX_DO_CHANS]; //1 byte

	U16 ai_data_count;
	U16 ao_data_count;
	U16 di_data_count;
	U16 do_data_count;

	caddr_t tx_buff;
	char *rx_buff[BUFFER_RX_SG];
	struct rtnet_frame tx_frame;
	struct rtnet_frame rx_frame;
	u32	id;
	struct ctl_pkt ctl_pkt;
	u32	updates;
	u32	opens;
	sem_t	rx_sem;
	pthread_spinlock_t lock;
	pthread_spinlock_t datalock;
	u8 status;// >0 ----- in simulation status
	u8 mode;
	u32 sync_cyc;
	u64	tx_count;
	u64	rx_count;

};

struct sync_note{
	unsigned long long l_1; 
	unsigned long long l_2;
	u32	cpu_freq;				// kHz
	u32 exceed_n30;
	u32 exceed_n20;
	u32 exceed_n10;
	u32 exceed_0;
	u32 exceed_p10;
	u32 exceed_p20;
	u32 exceed_p30;
	u32 max_cyc;
	u32	max_cyc_pos;
	u32	min_cyc;
	u32	min_cyc_pos;
	u32	cyc_cnt1;				// < -30%
	u32	cyc_cnt2;				// -30 ~ -20%
	u32	cyc_cnt3;				// -20 ~ -10%
	u32	cyc_cnt4;				// -10 ~   0%
	u32	cyc_cnt5;				//   0 ~ +10%
	u32	cyc_cnt6;				// +10 ~ +20%
	u32	cyc_cnt7;				// +20 ~ +30%
	u32	cyc_cnt8;				// > +30%
	u32 total;					// total count
};

extern void rtclock_gettime(unsigned long long *tt);
extern void clock_sleepto(unsigned long long tt);
static struct sync_note s_note;
static struct dev_buff gs_dev[MAX_DEVICES];
//static unsigned int gs_dev_core[MAX_DEVICES]={0};
static unsigned int pthread_rx_event_core = 0;
static unsigned int pthread_gpsTimer_core=0;
int gs_dev_no[MAX_DEVICES]={-1,-1,-1,-1};

#ifndef DPDK_FRAMEWORK


#define RT_ETIMEDOUT     1           //is what?
#define MAX_PKT_BURST 128
#define NSECS_PER_SEC 1000000000L

#define rt_timespec_normalize(t) {\
		if ((t)->tv_nsec >= NSECS_PER_SEC) { \
			(t)->tv_nsec -= NSECS_PER_SEC; \
			(t)->tv_sec++; \
		} else if ((t)->tv_nsec < 0) { \
			(t)->tv_nsec += NSECS_PER_SEC; \
			(t)->tv_sec--; \
		} \
}

#define rt_timespec_sub(t1, t2) do { \
		(t1)->tv_nsec -= (t2)->tv_nsec;  \
		(t1)->tv_sec -= (t2)->tv_sec; \
		rt_timespec_normalize(t1);\
} while (0)

#define rt_timespec_add_ns(t,n) do { \
		(t)->tv_nsec += (n);  \
		rt_timespec_normalize(t); \
} while (0)

#define rt_timespec_nz(t) ((t)->tv_sec != 0 || (t)->tv_nsec != 0)

#define rt_timespec_lt(t1, t2) ((t1)->tv_sec < (t2)->tv_sec || ((t1)->tv_sec == (t2)->tv_sec && (t1)->tv_nsec < (t2)->tv_nsec))

void swapu16(U16* ps)
{
	if(! ps)
		return ;
	char* p=(char* )ps;
	char tmp=p[0];
	p[0]= p[1];
	p[1]=tmp;
}

void swapu32(float* ps)
{
	if(! ps)
		return ;
	char* p=(char* )ps;
	char tmp=p[0];
	p[0]= p[3];
	p[3]=tmp;

	tmp=p[1];
	p[1]=p[2];
	p[2]=tmp;

}


static inline u64 rt_timespec_to_ns(const struct timespec *ts)
{
	u64 t;

	t = ((u64) ts->tv_sec * NSECS_PER_SEC) + ts->tv_nsec;

	return t;
}



extern uint32_t
rte_fiber_generic_pkg(uint8_t port_id, uint16_t queue_id, void *data, uint32_t len);


extern  uint32_t
rte_fiber_tx_burst(uint8_t port_id, uint16_t queue_id,
		uint16_t pkt_len);

extern rte_fiber_rx_burst(uint8_t port_id, uint16_t queue_id,
		struct rte_mbuf **rx_pkts, uint16_t nb_pkts);

extern dpdk_env_exit();
extern void dpdk_wait_core();
#define TimeStamp 8		

/****下表是常用ccitt 16,生成式1021反转成8408后的查询表格****/
unsigned short crc_table[256] = {
		0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
		0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
		0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
		0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
		0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
		0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
		0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
		0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
		0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
		0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
		0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
		0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
		0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
		0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
		0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
		0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
		0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
		0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
		0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
		0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
		0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
		0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
		0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
		0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
		0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
		0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
		0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
		0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
		0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
		0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
		0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
		0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
}; 

static unsigned short calc_aurora_crc(unsigned char *ptr,int len) 
{ 
	unsigned short crc; 
	crc = 0xffff; 
	while (len--) 
	{ 
		crc = (crc >> 8) ^ crc_table[(crc ^ *ptr++) & 0xff];
	} 
	return (crc ^ 0xFFFF); 
}

/* 该函数发送数据，添加必要的CRC
 *
 *
 */
static int dpdk_write(U16 DeviceId, unsigned char *data, U16 length)
{
	if(DEBUG_NO_DPDK)
		return 0;

	if(!data)
	{
		return -1;
	}
	U16 port_id = DeviceId/2;
	U16 channel_id = DeviceId%2;

	// 如果光纤网卡底层不自带CRC,要求程序添加CRC
	// 即使添加吧CRC,下面IO卡还是检查出来CRC错误,这是因为实际AURORA帧添加的包头未能列入计算
	// IO卡要求v0.0.007以上版本,并且GPIO_SWITCH[5]的OFF位置（即忽略CRC正误）
	// 数据末尾添加16位CRC,0x1021多项式,与aurora v8.3的CRC计算要求一样
	// 把CRC数据填充到data末尾,并且length+2
	int ret;
	U16 crc;
	crc = calc_aurora_crc(data, length);

	data[length++] = (U8)crc;
	data[length++] = (U8)(crc >> 8);
	ret = rte_fiber_generic_pkg(port_id, channel_id, data, length);

	if (ret != 1)
	{
		rte_wmb();
		ret = rte_fiber_tx_burst(port_id, channel_id, length);
	}
	else
	{
		fprintf(fdbg,"Fatal error for no pkt to send !\n");
		return -1;
	}
	return 1;

}


/*
static int dpdk_write(U16 DeviceId, struct dev_buff *dev, U16 DesPortId)
{
	// 注意:
	// 光纤网卡的发送帧从ctl_pkt或sim_pkt的order字段开始发送
	// 所以起始地址减去14字节以太网物理包头,再减去16字节光纤网卡包头
	//unsigned char *data = dev->tx_frame.payload + (14 + 16);
	unsigned char *data = dev->tx_frame.payload ;
	int length = dev->tx_frame.len;

	if(0)
	{
		int offset=0;
		U16* ps=(U16* )data;
		printf("dpdk write: packet type :%2x----",ps[0]);
		printf("dpdk write: total size :%d------",ps[1]);
		printf("dpdk write: current pack  :%d----",ps[2]);
		printf("dpdk write: analog data count :%d  :",ps[3]);

		//float * f=(float* )(data+8*sizeof(unsigned char));
		int i=0;
		for(;i<2;i++)
			printf("dpdk write: analog data[%d]=%f --   ;",i,((float*)(data+8))[i]);

	}


	int i;
	int ret;
	U16 crc;

	U16 port_id = DeviceId/2;
    U16 channel_id = DeviceId%2;

	//calculate the total packet size
	U16 total_size=(length/MAX_PACK_LENGTH)+1;

    int index=0;
    int left_length=length;


    while(index<total_size)
    {

    	U16* pu=(U16* )data;
    	pu[0]=0x88fb;
    	pu[1]=total_size;
    	pu[2]=index+1;

    	if(left_length > MAX_PACK_LENGTH)
    	{
    		length = MAX_PACK_LENGTH;
    	}
    	else
    	{
    		length = left_length;
    	}

    	// 如果光纤网卡底层不自带CRC,要求程序添加CRC
    	// 即使添加吧CRC,下面IO卡还是检查出来CRC错误,这是因为实际AURORA帧添加的包头未能列入计算
    	// IO卡要求v0.0.007以上版本,并且GPIO_SWITCH[5]的OFF位置（即忽略CRC正误）
    	// 数据末尾添加16位CRC,0x1021多项式,与aurora v8.3的CRC计算要求一样
    	// 把CRC数据填充到data末尾,并且length+2
    	crc = calc_aurora_crc(data, length);

    	data[length++] = (U8)crc;
    	data[length++] = (U8)(crc >> 8);

    	printf("dpdk_write: length: %d\n",length);

#if 1
    	if (dev->status == 0)
    	{
    		printf("DeviceId = %d Send Data --->",DeviceId);
    		for (i = 0; i < length; i++)
    			printf("%02x ",*(data+i));
    		printf("\n");
    	}
#endif
    	ret = rte_fiber_generic_pkg(port_id, channel_id, data, length);

    	if (ret != 1)
    	{
    		rte_wmb();
    		ret = rte_fiber_tx_burst(port_id, channel_id, length);
    	}
    	else
    	{
    		printf("Fatal error for no pkt to send !\n");
    		return -1;
    	}
    	left_length=left_length-MAX_PACK_LENGTH;
    	data=data+MAX_PACK_LENGTH-3*sizeof(U16);
    }

	return 1;
}

 */
static struct rte_mbuf *dpdk_read(int DeviceId, struct dev_buff *dev)
{
	int ret = 0;
	int i;	
	struct rte_mbuf *pkt;
	struct rte_mbuf *pkts[MAX_PKT_BURST];

	U16 port_id = DeviceId/2;
	U16 channel_id = DeviceId%2;
	//U8 xx=0;
	//printf("dpdk_read: port_id:%d, channel_id:%d\n",port_id,channel_id);
	while(1)
	{
		/*
		if(xx==0)
		{
		   printf("dpdk_read: port_id:%d, channel_id:%d\n",port_id,channel_id);
		   xx=1;
		}
		 */
		ret = rte_fiber_rx_burst(port_id, channel_id, pkts, 1);

		if ( ret != 0)
		{
			pkt = pkts[0];

			for( i =0 ; i < ret; i ++)
			{
				pkt=pkts[i];

#if 0
				int j;
				printf("\=============DeviceId=%d, pkt_nb= %d, pkt->pkt.data_len=%d =============\n",DeviceId, ret, pkt->pkt.pkt_len);
				for(j=0; j<pkt->pkt.pkt_len; j ++)
				{
					printf(" %2.2x ", *((U8*)&pkt->pkt.data[j]));
				}

				printf("\n\n");
#endif	
			}	


			break;
		}
		else
			return NULL;
	}
	return pkt;
}

#endif

/*
static I16 yjdnet_write(U16 DeviceId, struct dev_buff *dev,U16 DesPortId)
{
	dev->dev_buff

	int i, err;
	printf("yjdnet_write: DeviceId:%d, DesPortId:%d\n",DeviceId,DesPortId);
	if ((err = dpdk_write(DeviceId, dev, DesPortId)) != 1)
		yjdnet_debug("dpdk_write(%d) is failed: %d\n", DeviceId, err);
	else {
		if (dev->status)
			return err;
#if 0
		yjdnet_debug("--> %3.3d : ", dev->tx_frame.len);
		for (i = 0; i < dev->tx_frame.len; i++)
			yjdnet_debug("%2.2x ", dev->tx_frame.payload[i]);
		yjdnet_debug("\n");
#endif
	}
	return err;
}
 */
/*
 * 本函数将原始的数据组包，加上必要的字头
 */

static I16 yjdnet_write(U16 DeviceId, struct dev_buff *dev,U16 DesPortId)
{
	int dpdk_send_time=0;
	unsigned char *data = dev->tx_frame.payload ;
	U32 length = dev->tx_frame.len;

	U16 lbuff=MAX_PACK_LENGTH+1024;

	unsigned char* datasend=malloc(lbuff);

	memset(datasend,0,lbuff);

	//packet no
	U16 packet_total_num=(length/MAX_PACK_LENGTH)+1;

	if(DEBUG)
	{
		fprintf(fdbg,"yjdnet_write: total data length=%d\n",length);
		fprintf(fdbg,"yjdnet_write:  data length limitation of each send =%d\n",MAX_PACK_LENGTH);
		fprintf(fdbg,"yjdnet_write: packet_total_num=%d\n",packet_total_num);

	}

	U16 current_num=1;
	unsigned char* pmove=data;

	I32 left_length=length;
	I32 data_sent=0;
	int send_length;

	int err=0;

	if(!DPDK_ENABLE)
	{
		char CIP[100];
		int portNO;
		strcpy(CIP,"127.0.0.1");
		portNO=8000;
		int Ierr=1;
		int iii=0;

		do {
			Ierr=InitConnection(CIP,portNO);
			iii=iii+1;
			fprintf(fdbg,"Connecting to Socket B, try %d\n",iii);
			sleep(1);

		}while ((Ierr!=0)&&(iii<100));

		if(Ierr!=0)
		{
			fprintf(fdbg,"Not able to connectSocket B, bye!\n");
			return -1;
		}

	}

	while(current_num <= packet_total_num)
	{

		if(DEBUG)
			fprintf(fdbg,"yjdnet_write: current_num=%d\n",current_num);
		memset(datasend,0,lbuff);
		U16* ps=(U16*) datasend;
		ps[0]=0x88FB;
		swapu16(ps);

		ps[1]=packet_total_num;
		swapu16(ps+1);

		ps[2]=current_num;
		swapu16(ps+2);

		if(DEBUG)
			fprintf(fdbg,"yjdnet_write: left_length:%d,MAX_PACK_LENGTH:%d \n",left_length,MAX_PACK_LENGTH);
		send_length=((left_length-MAX_PACK_LENGTH)>0)?MAX_PACK_LENGTH:left_length;

		if(DEBUG)
			fprintf(fdbg,"yjdnet_write: No. %d packet length:%d\n",current_num,send_length);
		left_length-=send_length;

		memcpy(datasend+sizeof(U16)*3,data+data_sent,send_length);
		usleep(0);//without this line, packet sending will run into packet-loss problem.
		if(DEBUG_RAW)
		{
			int x=0;
			fprintf(fdbg,"yjdnet_write: print data before send: current pack： %d :-------------\n",current_num);
			for(;x<send_length+sizeof(U16)*3;x++)
				fprintf(fdbg,"%02x ",datasend[x]);
			fprintf(fdbg,"\n");

		}


		if(DEBUG)
		{
			fprintf(fdbg,"yjdnet_write: DeviceId:%d, DesPortId:%d\n",DeviceId,DesPortId);
			fprintf(fdbg,"yjdnet_write:the length sent by dpdk:%d\n",send_length+sizeof(U16)*3);
		}

		if(!DPDK_ENABLE)
		{
			/*
			int Ierr;
			fprintf(fdbg,"yjdnet_write: send data by socket.\n");
			//void MsgSend(int Ntype,char* Databuf,int LenData,int* Ierr)
			//MsgSend(1,datasend,send_length+sizeof(U16)*3,&Ierr);

			int tmp_length=send_length+sizeof(U16)*3;

			fprintf(fdbg,"socket send length:%d\n",tmp_length);
			fprintf(fdbg,"socket max length:%d\n",Msg_MaxLen);

			char CIP[100];
			int portNO;
			strcpy(CIP,"127.0.0.1");
			portNO=8000;
			Ierr=1;
			int iii=0;

			do {
				Ierr=InitConnection(CIP,portNO);
				iii=iii+1;
				fprintf(fdbg,"Connecting to Socket B, try %d\n",iii);
				sleep(1);

			}while ((Ierr!=0)&&(iii<100));

			if(Ierr!=0)
			{
				fprintf(fdbg,"Not able to connectSocket B, bye!\n");
				return -1;
			}
			 */



			//char* pWrtMsg="hello,you are connected!\n";

			//WriteMessage(pWrtMsg, 40);
			WriteMessage(datasend,send_length+sizeof(U16)*3);

		}



		if(DPDK_ENABLE)
		{
			if ((err = dpdk_write(DeviceId, datasend, send_length+sizeof(U16)*3)) != 1)
			{
				fprintf(fdbg,"yjdnet_write: write failed,DeviceId:%d, DesPortId:%d\n",DeviceId,DesPortId);

				fprintf(fdbg,"dpdk_write(%d) is failed: %d\n", DeviceId, err);

			}
			else
			{
				dpdk_send_time++;

				if(DEBUG)
				{
					fprintf(fdbg,"yjdnet_write:dpdk_send_time=%d\n",dpdk_send_time);
					/*
				int tmp_length=send_length+sizeof(U16)*3);
				cout<<"packet size:"<<tmp_length<<endl;
				cout(;i<tmp_length,i++)
								   f1<<hex<<datasend[i]<<" ";
					 */

					/*
				fstream  f1("me.txt");

				if(!f1) return;
				int i=0;
				int tmp_length=send_length+sizeof(U16)*3);
				f1<<"packet size:"<<tmp_length<<endl;
				for(;i<tmp_length,i++)
				   f1<<hex<<datasend[i]<<" ";

				f1<<endl;
				f1.close();
					 */


				}
				/*
			if (dev->status ==0 )
			{
				printf("yjdnet_write: status =0, quit!\n");
				free(datasend);
				datasend=0;
				return err;
			}
				 */

#if 0
				int i;
				yjdnet_debug("--> %3.3d : ", dev->tx_frame.len);
				for (i = 0; i < dev->tx_frame.len; i++)
					yjdnet_debug("%2.2x ", dev->tx_frame.payload[i]);
				yjdnet_debug("\n");
#endif
			}
		}
		data_sent+=send_length;

		pmove=data+send_length;
		current_num++;
		if(DEBUG)
			fprintf(fdbg,"yjdnet_write: current_num=%d\n",current_num);


	}

	free(datasend);
	datasend=0;
	fprintf(fdbg,"yjdnet_write: end of send data\n");
	return err;

}

struct _global_time currTime,startTime;
U16 global_us;


static void *__rx_event(__attribute__((unused)) void *arg)
{
	//U16 DeviceId, dev_index;
	//struct dev_buff *dev;
	//struct rte_mbuf *pkt_rx;
	set_thread_affinity(pthread_rx_event_core);

	//U16 tmp111[9];

	//printf("__rx_event: do nothing but will dead loop-------------------\n");

	/*
	int retval=dpdk_init();
	if (retval != 0)
	{
		printf("dpdk init failed with error[%d]\n", retval);
		return -1;
	}

	printf("__rx_event: dpdk init successfully!\n");
	 */
	fprintf(fdbg,"__rx_event: suppose dpdk initialed by the main function!\n");

	//U8 xx=0;

	usleep(200);

	while (1)
	{
		I16 ret=-1;
		ret=read_all_devices_data();
		if(ret<0)
			continue;

	}

}


I16 read_all_devices_data()
{
	int dev_index;

	for (dev_index = 0; dev_index < MAX_DEVICES; dev_index++)
	{
		if (gs_dev_no[dev_index] == -1) continue;

		U16 DeviceId = gs_dev_no[dev_index];

		struct dev_buff* dev = &gs_dev[DeviceId];

		return get_input_data(DeviceId,dev);

	}
	return 1;

}

void test_get_input_data(U16 DeviceId, struct dev_buff *dev)
{

	dev->ai_data_count=5;
	dev->di_data_count=2;
	dev->ai_data[0]=0.7;
	dev->ai_data[1]=0.8;
	dev->ai_data[2]=0.9;
	dev->ai_data[3]=0.7;
	dev->ai_data[4]=0.6;
	dev->di_data[0]=1;
	dev->di_data[1]=0;
	//dev->di_data[2]=0;
	//printf("test_get_input_data: set ai_data_count=0; di_data_count=1;\n");

}

I16 get_input_data(U16 DeviceId, struct dev_buff *dev)
{

	if(DEBUG_NO_DPDK_READ || DEBUG_SIM_REC)
	{
		test_get_input_data(DeviceId, dev);
		return 1;
	}

	//printf("get_intput_data start!\n");
	int run=1;
	int grid[MAX_COUNT+1];
	U16 total_size=0;
	U16 current_pack=0;
	U16 pkt_type=0;

	int limited_length=1024*1024*10;

	unsigned char* data_receive=malloc(limited_length);
	memset(data_receive,0,MAX_COUNT*6+4098);
	memset(grid,0,MAX_COUNT);

	int data_copied=0;

	int run_time=0;

	while(run==1)
	{
		run_time++;
		//printf("run time:%d\n",run_time);
		struct rte_mbuf *pkt_rx;

		pkt_rx = dpdk_read(DeviceId, dev);


		if (pkt_rx	== NULL)
		{
			if(pkt_type == 0 )
			{
				free(data_receive);
				return 0;

			}
			continue;
		}


		if(!pkt_rx)
		{
			if(pkt_type == 0 )
			{
				free(data_receive);
				return 0;
			}
			continue;
		}


		// rte_mbuf结构中的pkt下的data指向ctl_pkt或sim_pkt的stamp字段
		//pkt = (struct ctl_pkt*)(pkt_rx->pkt.data - 8);
		//dev->rx_frame.len = pkt_rx->pkt.data_len + (14 + 8);

		unsigned char* pkt = (unsigned char*)(pkt_rx->pkt.data + 8);

		if(DEBUG)
		{
			fprintf(fdbg,"get_intput_data get real data\n");
			printf("print-------------------\n");
			int x=0;
			for(;x<pkt_rx->pkt.data_len;x++)
				fprintf(fdbg,"%02x ",pkt[0]);
			fprintf(fdbg,"-------------\n");
		}

		U16* pu=(U16*)pkt;

		swapu16(pu);
		swapu16(pu+1);
		swapu16(pu+2);

		pkt_type=pu[0];

		if(total_size == 0)
		{
			total_size=pu[1];
		}
		else
		{
			if(total_size != pu[1])
			{
				fprintf(fdbg,"get_input_data: get total_size:%d\n",pu[1]);
				fprintf(fdbg,"get_input_data: get total_size in previous packet:%d\n",total_size);
				fprintf(fdbg,"the total size in this packet is not equal with the previous one!\n");
				free(data_receive);
				return -1;
			}
		}


		if( pu[2]-1 != current_pack)
		{
			if(DEBUG)
			{
				fprintf(fdbg,"get_input_data: get current pack:%d\n",pu[2]);
				fprintf(fdbg,"get_input_data: get current pack in previous :%d\n",current_pack);
				fprintf(fdbg,"get_input_dat pack sequence is wrong！\n");
			}
			free(data_receive);
			return -1;
		}
		current_pack=pu[2];

		/*
		U16 tmp=((U16* )pkt)[3];
		swapu16(&tmp);
		printf("get_input_data: tmp(size of ai data：%d\n",tmp);
		 */



		int length=pkt_rx->pkt.data_len-8-2;

		if(DEBUG)
			fprintf(fdbg,"get_input_data: length:%d\n",length);

		length=pkt_rx->pkt.data_len-sizeof(U16)*3-8-2;


		if(data_copied+length>limited_length)
		{
			fprintf(fdbg,"!!!!------------  data received are too big, program exit !!--\n ");
			free(data_receive);
			return -1;
		}
		memcpy(data_receive+data_copied, pkt+sizeof(U16)*3,length);


		if(DEBUG)
		{
			fprintf(fdbg,"get_input_data: DevideID=%d,pkt_type=%x,total_packet_size=%d,current_pack=%d\n",DeviceId,pkt_type,total_size,current_pack);
			fprintf(fdbg,"get_input_data: length:%d\n",length);
		}

		if(0)
		{
			fprintf(fdbg,"get_input_data: print raw data received:\n");
			int x=0;
			for(;x<length;x++)
				fprintf(fdbg,"%02x ",(data_receive+data_copied)[x]);
			fprintf(fdbg,"\n");
		}

		data_copied+=length;

		//printf("get_input_data: data copied at this time : %d\n,",data_copied);

		/*
		grid[current_pack]=1;
		//printf("get_input_data: set current pack : grid[%d]=%d\n",current_pack,grid[current_pack]);
		int i=1;
		run=0;
		for( ;i<=total_size; i++)
		{
			//printf("grid[%d]=%d\n",i,grid[i]);
			if( grid[i] == 0  )
			{
				run=1;
				break;
			}
		}
		 */

		rte_pktmbuf_free(pkt_rx);

		if( current_pack == total_size )
			break;

	}

	if(DEBUG)
	{
		fprintf(fdbg,"get all packets, exit!\n");
		fprintf(fdbg,"get_input_data: total data copied:%d,",data_copied);
	}
	dev->rx_count++;

	U16 *ps=(U16*)data_receive;
	swapu16(ps);
	dev->ai_data_count=ps[0];

	if(dev->ai_data_count > MAX_COUNT || dev->ai_data_count <0 )
	{

		free(data_receive);


		char fname[32];
		char str_dbg[256];
		sprintf(fname,"PDIDERR.LIS");

		fdbg=fopen(fname,"w");
		if (NULL==fdbg) {
			printf("Fail to open file %s",fname);

			return 1;
		}
		sprintf(str_dbg,"count of AI data: %d exceeds the limitation",dev->ai_data_count);
		fputs(str_dbg,fdbg);
		//fputs(str_dbg,stdout);
		fflush(fdbg);


		return -1;
	}


	//fprintf(fdbg,"get_input_data:dev->rx_count:%d\n",dev->rx_count);
	int xx=dev->rx_count;
	if(xx%INFO_PRINT_INPUT_STEP==1 ||DEBUG)
	{
		fprintf(fdbg,"get_input_data:dev->rx_count:%d\n",dev->rx_count);
		fprintf(fdbg,"get_input_data: get ai_data_count=%d\n",dev->ai_data_count);
	}

	int i=0;
	float* pu=(float*)(data_receive+sizeof(U16));

	for(;i<(dev->ai_data_count);i++)
	{
		swapu32(pu+i);

		if(xx%INFO_PRINT_INPUT_STEP==1 ||DEBUG)
		{
			//printf("get_input_data: get analog data[%d]=%f\n",i,pu[i]);
			fprintf(fdbg,"get_input_data: get analog data[%d]=%f\n",i,pu[i]);
		}
	}

	memcpy(dev->ai_data,(U8*)pu,dev->ai_data_count*sizeof(float));

	U8* offset=data_receive+sizeof(U16)+dev->ai_data_count*sizeof(float);

	ps=(U16* )offset;
	swapu16(ps);
	dev->di_data_count=ps[0];

	if(dev->di_data_count > MAX_COUNT || dev->di_data_count <0 )
	{
		free(data_receive);
		char fname[32];
		char str_dbg[256];
		sprintf(fname,"PDIDERR.LIS");
		fdbg=fopen(fname,"w");
		if (NULL==fdbg) {
			printf("Fail to open file %s",fname);
			return 1;
		}
		sprintf(str_dbg,"count of DI data: %d exceeds the limitation",dev->ai_data_count);
		fputs(str_dbg,fdbg);
		fputs(str_dbg,stdout);
		fflush(fdbg);

		return -1;
	}

	if(xx%INFO_PRINT_INPUT_STEP==1 || DEBUG)

		fprintf(fdbg,"get_input_data: get di_data_count=%d\n",dev->di_data_count);
	memcpy(dev->di_data,(U8*)(offset+sizeof(U16)),dev->di_data_count*sizeof(U8));

	if(DEBUG || xx%INFO_PRINT_STEP==1)
	{
		if(dev->di_data_count>0)
			for(i=0;i<dev->di_data_count;i++)
				fprintf(fdbg,"get_input_data: get digital data[%d]=%d\n",i,dev->di_data[i]);
		fprintf(fdbg,"get_input_data:print input  data done----------------\n");
	}
	free(data_receive);
	return data_copied;

}

U32	get_cpu_freq(void)
{
	/*
	FILE *fd;
	char buff[256];
	U32 freq;
	F32 f;

	fd = fopen("/proc/cpuinfo", "r");
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "cpu MHz         : %f", f);
	freq = f * 1000;
	printf("cpu kHz : %d\n", freq);
	return freq;*/
	return 2666283;
}
pthread_t pid_thread;
I16 pid_thread_create()
{
	int core;
	core = allocate_rtcore();

	if (core < 0)
	{
		printf("PHY pthread_create(rx_event): allocate_rtcore failed,core = %d\n",core);
		return -1;
	}
	else
		printf("pthread_create(rx_event): allocate_rtcore,core = %d\n",core);

	pthread_rx_event_core = core;
	pthread_create(&pid_thread, NULL, __rx_event, &pthread_rx_event_core);

	printf("PHY: pthread_create returns 0, and its cpu no. is %d\n",pthread_rx_event_core);

	return 0;
}



static  void*  __rx_socket_server(__attribute__((unused)) void *arg)
{
	fprintf(Recfdbg,"start socket listen server !\n");
	fflush(Recfdbg);
	int    socket_fd, connect_fd;
	struct sockaddr_in     servaddr;
	char    buff[4096];
	int     n;

	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		fprintf(Recfdbg,"create socket error: \n");
		fflush(Recfdbg);
		fclose(Recfdbg);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(DEFAULT_REC_PORT);


	if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		sprintf(Recfdbg,"bind socket error: (errno: )\n");
		fflush(Recfdbg);
		fclose(Recfdbg);
		exit(0);
	}

	if( listen(socket_fd, 10) == -1){
		fprintf(Recfdbg,"listen socket error: (errno: )\n");
		fflush(Recfdbg);
		fclose(Recfdbg);
		exit(0);
	}
	fprintf(Recfdbg,"======waiting for client's request======\n");
	while(1){

		if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){
			fprintf(Recfdbg,"accept socket error: (errno: )");
			continue;
		}

		n = recv(connect_fd, buff, MAXLINE, 0);

		if(send(connect_fd, "Hello,you are connected!\n", 26,0) == -1)
		{
			perror("send error");
			close(connect_fd);
			fflush(Recfdbg);
			fclose(Recfdbg);
			exit(0);
		}

		buff[n] = '\0';
		fprintf(Recfdbg,"recv msg from client: %s\n", buff);
		fflush(Recfdbg);
		fclose(Recfdbg);

	}
	fflush(Recfdbg);
	fclose(Recfdbg);
	close(socket_fd);

}


int pid;
I16 pid_thread_create_socket()
{

	printf("pid_thread_create_socket: to create a thread!\n:");
	int corenum;
	pthread_create(&pid, NULL, __rx_socket_server, &corenum);

	return 0;
}
unsigned long long t3,t4;

static void *__gpsTimer_event(__attribute__((unused)) void *arg)
{
	u64 t_H=0,t_L=0;
	u64 i=0;
	U8 TAG=0;
	unsigned long long  curr_time,curr_time1;
	printf("gpstime thread created...\n");

	set_thread_affinity(pthread_gpsTimer_core);

	printf("__gpsTimer_event: begin to dead loop\n:");
	printf("__gpsTimer_event:code commented for debug\n:");

	while(1)
	{
		/*

		t_H = rte_fiber_get_register(0,0,17);
		t_L = rte_fiber_get_register(0,0,18);
		 */

		currTime.gtime_y  = 10*((t_H>>26)%16)+((t_H>>22)%16);
		currTime.gtime_d  = 100*((t_H>>20)%4)+10*((t_H>>16)%16)+((t_H>>12)%16);
		currTime.gtime_h  = 10*((t_H>>10)%4)+((t_H>>6)%16);
		currTime.gtime_m  = 10*((t_H>>3)%8)+2*(t_H%8)+(t_L>>31)%2;
		currTime.gtime_s = 10*((t_L>>28)%8)+(t_L>>24)%16;
		currTime.gtime_ms = 100*((t_L>>20)%16)+10*((t_L>>16)%16)+(t_L>>12)%16;
		currTime.gtime_us = 100*((t_L>>8)%16)+10*((t_L>>4)%16)+t_L%16;


		//	printf("TH: 0x%08x  TL:0x%08x  \n",t_H,t_L);

	}
}


pthread_t gpsTimer_thread;
I16 gpsTimer_thread_create()
{
	int core;
	core = allocate_rtcore();

	if (core < 0)
	{
		printf("pthread_create(gpsTimer): allocate_rtcore failed,core = %d\n",core);
		return -1;
	}
	else
		printf("pthread_create(gpsTimer): allocate_rtcore,core = %d\n",core);

	pthread_gpsTimer_core = core;
	pthread_create(&gpsTimer_thread, NULL, __gpsTimer_event, &pthread_gpsTimer_core);

	printf("gpsTimer: pthread_create returns 0, and its cpu no. is %d\n",pthread_gpsTimer_core);

	return 0;
}

I16 wait_PPS_signal()
{
	U8 moniter_ms = 0;
	U8 ppsSignal = 0;

	struct _global_time lockTime;

	/*检测整秒来临后，返回有效标志，可以启动仿真*/
	while(1)
	{
		if((currTime.gtime_ms == 999)&&(moniter_ms==0))
		{
			moniter_ms = 1;
		}

		if((moniter_ms == 1)&&(currTime.gtime_ms == 0))
		{
			ppsSignal = 1;

			lockTime.gtime_us = currTime.gtime_us;
			lockTime.gtime_ms = currTime.gtime_ms;
			lockTime.gtime_s = currTime.gtime_s;
			lockTime.gtime_m = currTime.gtime_m;
			lockTime.gtime_h = currTime.gtime_h;
			lockTime.gtime_d = currTime.gtime_d;
			lockTime.gtime_y = currTime.gtime_y;

			startTime.gtime_us = lockTime.gtime_us;
			startTime.gtime_ms = lockTime.gtime_ms;

			global_us = lockTime.gtime_us;

			startTime.gtime_s = lockTime.gtime_s;
			startTime.gtime_m = lockTime.gtime_m;
			startTime.gtime_h = lockTime.gtime_h;
			startTime.gtime_d = lockTime.gtime_d;
			startTime.gtime_y = lockTime.gtime_y;

			break;
		}
	}

	printf("The Start time is--->%d-%d %d:%d:%d.%d.%d \n",startTime.gtime_y,startTime.gtime_d,startTime.gtime_h,startTime.gtime_m,startTime.gtime_s,startTime.gtime_ms,startTime.gtime_us);

	return 0;
}


I16 getSignal_NextDT(double *_global_us_t,double *_global_us_g,double dt)
{
	U8	signal_NextDT=0;
	U16 n;

	struct _global_time curr2;

	double DT=dt*1000000;

	while(1)
	{
		n = currTime.gtime_us - global_us;
		if (n > 60000) n += 1000;
		if (n > DT)
		{
			global_us += DT;

			*_global_us_g +=DT;
			*_global_us_t =*_global_us_g+n-DT;
			if(global_us >999)
				global_us = global_us-1000;
			signal_NextDT = 1;
			//printf("%d\n", currTime.gtime_us);

			if(n>55) 
			{
				//		printf("%d, CURR    --->%d-%d %d:%d:%d.%d.%d \n",global_us,currTime.gtime_y,currTime.gtime_d,currTime.gtime_h,currTime.gtime_m,currTime.gtime_s,currTime.gtime_ms,currTime.gtime_us);
				//		printf("%d, PRE    --->%d-%d %d:%d:%d.%d.%d \n",global_us,curr2.gtime_y,curr2.gtime_d,curr2.gtime_h,curr2.gtime_m,curr2.gtime_s,curr2.gtime_ms,curr2.gtime_us);
			}

			curr2.gtime_us = currTime.gtime_us;
			curr2.gtime_ms = currTime.gtime_ms;
			curr2.gtime_s = currTime.gtime_s;
			curr2.gtime_m = currTime.gtime_m;
			curr2.gtime_h = currTime.gtime_h;
			curr2.gtime_d = currTime.gtime_d;
			curr2.gtime_y = currTime.gtime_y;

			break;
		}


		/*		if((currTime.gtime_us - global_us)>50)
		{		
			global_us += 50;
			if(global_us >999)
				global_us = global_us-1000;
			signal_NextDT = 1;
			break;
		}

		if( ((currTime.gtime_us - global_us)<0)&&((currTime.gtime_us+1000-global_us)>50) )
		{
			global_us += 50;
			if(global_us >999)
				global_us = global_us-1000;
			signal_NextDT = 1;
			break;
		}*/
	}

	if(signal_NextDT == 1) 
	{	
		return 1;
	}
}

I16 getEndTime()
{	
	printf("The End time is--->%d-%d %d:%d:%d.%d.%d \n",currTime.gtime_y,currTime.gtime_d,currTime.gtime_h,currTime.gtime_m,currTime.gtime_s,currTime.gtime_ms,currTime.gtime_us);

	return 0;
}

static I16 PID_Register(U16 DeviceId_in, U8 Mode)
{
	struct dev_buff *dev;
	rtnet_mem_t rx_md;
	struct enet_frame *frame;
	int err, core;

	//struct sim_pkt *tx_sim_pkt;

	int DeviceId = DeviceId_in;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	fprintf(fdbg,"PID_Register: mode:%2x\n",Mode);
	dev->mode  = Mode;
	dev->tx_count = 0;//add by zhangy
	dev->rx_count = 0;//add by zhangy

	if (dev->opens) {
		dev->opens++;
		return NoError;
	}

	dev->id = DeviceId;

	if (!(dev->tx_buff = malloc(sizeof(struct enet_frame)))) {
		yjdnet_debug("rt_alloc_mem(%d) is failed\n", DeviceId);
		goto err_alloc_mem;
	}

	sem_init(&dev->rx_sem, 1, 0);
	pthread_spin_init(&dev->lock, 1);
	pthread_spin_init(&dev->datalock,1);

	dev->tx_frame.payload = dev->tx_buff;
	frame = (struct enet_frame *)dev->tx_frame.payload;
	memset(frame, 0x00, sizeof(struct enet_frame));

	dev->opens++;

	//tx_sim_pkt = (struct sim_pkt *)frame->data;
	//tx_sim_pkt->order =0;

	return NoError;

	err_alloc_mem:
	err_notify:
	err_rxring_allocate:
	//rtnet_close(DeviceId);
	err_open:
	return ErrorUnknownMacAddr;
}


static I16 PID_Release(U16 DeviceId)
{
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	if (!dev->opens) return NoError;
	dev->opens--;
	if (!dev->opens) {
		//rtnet_close(DeviceId);
		free(dev->tx_buff);
		sem_destroy(&dev->rx_sem);
	}

	if (pthread_rx_event_core != 0 )
	{
		release_rtcore(pthread_rx_event_core);
		printf("\npthread_create(rx_event): release_rtcore,core = %d\n",pthread_rx_event_core);
		pthread_rx_event_core = 0 ;

	}


	if (pthread_gpsTimer_core != 0)
	{
		release_rtcore(pthread_gpsTimer_core);
		printf("\npthread_create(gpsTimer): release_rtcore,core = %d\n",pthread_gpsTimer_core);
		pthread_gpsTimer_core = 0 ;
	}

	printf("NET %d COMM TO DEVICE->",DeviceId);
	printf(" tx: %d",dev->tx_count);
	printf(" rx: %d\n",dev->rx_count);

	return NoError;
}









static I16 PID_DI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;
	struct grp_cfg	*cfg;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;


	dev = &gs_dev[DeviceId];

	for (i = 0; i < Count; i++)
	{
		Data[i] = dev->di_data[ i];
		if(DEBUG)
		{
			fprintf(fdbg,"PID_DI_GrpRead: Data[%d]=%d----\n    ",i,Data[i]);

		}
	}

	dev->di_data_count=Count;
	if(DEBUG)
	{
		fprintf(fdbg,"PID_DI_GrpRead: size of data:%d---- \n  ",Count);
	}

	return NoError;
}

static I16 PID_DO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;


	dev = &gs_dev[DeviceId];

	if(Count> MAX_COUNT)
	{
		char fname[32];
		char str_dbg[256];
		sprintf(fname,"PDIDERR.LIS");
		fdbg=fopen(fname,"w");
		if (NULL==fdbg) {
			printf("Fail to open file %s",fname);
			return 1;
		}
		sprintf(str_dbg,"count of DO data: %d exceeds the limitation",Count);
		fputs(str_dbg,fdbg);
		fputs(str_dbg,stdout);
		fflush(fdbg);

		return -1;
	}

	dev->do_data_count=Count;

	for (i = 0; i < Count; i++)
		dev->do_data[ i ] = Data[i];

	return NoError;
}

static I16 PID_AI_GrpRead(U16 DeviceId, U16 GrpId, float *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;


	dev = &gs_dev[DeviceId];

	for (i = 0; i < Count; i++)
	{
		Data[i] = dev->ai_data[i];
		if(DEBUG)
		{
			fprintf(fdbg,"PID_AI_GrpRead: dev->ai_data[%d]=%f----    ",i,dev->ai_data[i]);
			fprintf(fdbg,"PID_AI_GrpRead: Data[%d]=%f-----",i,Data[i]);
			fprintf(fdbg,"\n");
		}
	}


	return NoError;
}

static I16 PID_AO_GrpWrite(U16 DeviceId, U16 GrpId, float *Data, U16 Count)
{
	//printf("function PID_AO_GrpWrite---\n");
	int i;
	struct dev_buff *dev;

	//printf("function PID_AO_GrpWrite: DeviceID:%d, MAX_DEVICES:%f\n",DeviceId,MAX_DEVICES);
	int max_tmp=MAX_DEVICES;
	//printf("function PID_AO_GrpWrite: DeviceID:%d, max_tmp:%d\n",DeviceId,max_tmp);

	if (DeviceId >= max_tmp)
		return ErrorUnknownMacAddr;

	//printf("function PID_AO_GrpWrite: not return ErrorUnknownMacAddr---\n");

	//printf("function PID_AO_GrpWrite: not GrpId >= MAX_GRP_CFGS return ErrorAOGrpWrite---\n");
	dev = &gs_dev[DeviceId];

	//printf("function PID_AO_GrpWrite: Count=%d\n",Count);

	//if (Count != cfg->count)
	//	return ErrorAOGrpWrite;
	//printf("function PID_AO_GrpWrite: not Count != cfg->count return ErrorAOGrpWrite---\n");

	if(Count> MAX_COUNT)
	{
		char fname[32];
		char str_dbg[256];
		sprintf(fname,"PDIDERR.LIS");
		fdbg=fopen(fname,"w");
		if (NULL==fdbg) {
			printf("Fail to open file %s",fname);
			return 1;
		}
		sprintf(str_dbg,"count of AO data: %d exceeds the limitation",Count);
		fputs(str_dbg,fdbg);
		fputs(str_dbg,stdout);
		fflush(fdbg);

		return -1;
	}

	F64* f=(F64*) Data;
	dev->ao_data_count = Count;

	for (i = 0; i < Count; i++)
	{
		dev->ao_data[i] = f[i];
		//printf("function PID_AO_GrpWrite f[%d]=%f  ---\n",i,f[i]);
		//printf("function PID_AO_GrpWrite dev->ao_data[%d]=%f:---\n",i,dev->ao_data[i]);

	}

	//printf("function PID_AO_GrpWrite count value:%d---\n",i-1);
	return NoError;
}

static I16 PID_AI_VoltScale(float *Data, F64 *Slp, F64 *Itcp, F64 *Volt, U16 Count)
{
	int i;

	if (!Data || !Slp || !Itcp || !Volt)
		return ErrorAIVoltScale;

	for (i = 0; i < Count; i++)
		//Volt[i] = (Data[i] - Itcp[i]) * Slp[i];
	{
		Volt[i]=Data[i];
		if(DEBUG)
		{
			fprintf(fdbg,"PID_AI_VoltScale: Count=%d-----",Count);
			fprintf(fdbg,"PID_AI_VoltScale:  Data[%d]=%f-----",i,Data[i]);
			fprintf(fdbg,"PID_AI_VoltScale:  Volt[%d]=%f-----",i,Volt[i]);
			fprintf(fdbg,"\n");
		}
	}




	return NoError;
}

static I16 PID_AO_VoltScale(F64 *Volt, F64 *Slp, F64 *Itcp, float *Data, U16 Count)
{
	if(DEBUG)
		fprintf(fdbg, "PID_AO_VoltScale: start\n");
	int i, tmp;

	if (!Data || !Slp || !Itcp || !Volt)
		return ErrorAOVoltScale;

	for (i = 0; i < Count; i++) {
		/*
		tmp = (Volt[i] - Itcp[i]) * Slp[i];
		if (tmp > 65535) tmp = 65535;
		if (tmp <     0) tmp = 0;
		Data[i] = tmp;
		 */
		Data[i]=Volt[i];
		//printf("PID_AO_VoltScale: Volt[%d]=%f,Data[%d]=%f -----------------\n",i,Volt[i],i,Data[i]);
	}
	//printf("PID_AO_VoltScale: Done -----------------\n");
	return NoError;
}

static I16 PID_MarkInput_IsUpdated(U16 DeviceId)
{
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];

	return dev->updates;
}

static I16 PID_MarkInput_Clear(U16 DeviceId)
{
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	dev->updates = 0;

	return NoError;
}


static int max_us_markinput;
static int max_us_markinput_count;
static I16 PID_MarkInput_Wait(U16 DeviceId, U16 Timeout)
{
	if(DEBUG)
		fprintf(fdbg,"PID_MarkInput_Wait: start---\n");
	struct dev_buff *dev;
	I16 tmp;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	/*
	pthread_spin_lock(&dev->datalock);
	tmp = dev->updates;
	if (tmp) dev->updates = 0;
	pthread_spin_unlock(&dev->datalock);
//	if(tmp>0)
	//	dev->rx_count++;

	 */
	if(DEBUG)
		fprintf(fdbg,"PID_MarkInput_Wait: dev-updates value:%d---\n",dev->updates);
	tmp = dev->updates;
	if (tmp) {
		if(DEBUG)
			fprintf(fdbg,"PID_MarkInput_Wait: to lock datalock\n");
		pthread_spin_lock(&dev->datalock);
		dev->updates = 0;
		pthread_spin_unlock(&dev->datalock);
		if(DEBUG)
			fprintf(fdbg,"PID_MarkInput_Wait:unlock datalock done...\n");
	}

	int xx=dev->tx_count;
	if(xx%INFO_PRINT_STEP==1)
		fprintf(fdbg,"NET%d updates=%d times=%d \n",DeviceId,tmp,dev->tx_count);
	if(tmp>1)
		fprintf(fdbg,"NET%d updates=%d times=%d \n",DeviceId,tmp,dev->tx_count);
	if (dev->mode == MODE_SEM)
		return sem_wait(&dev->rx_sem);

	return tmp;
	/*	while (1) {
		if (dev->updates) return dev->updates;
		usleep(10);
		tmp += 10;
		if (Timeout && tmp >= Timeout)
		return ErrorMarkInputWaitTimeout;
		}*/
}

static I16 PID_Update_NewData(U16 DeviceId)
{
	char str_dbg[8192];
	//sprintf(str_dbg,"PID_Update_NewData starts::\n");
	//fputs(str_dbg,fdbg);
	//fputs(str_dbg,stdout);
	//fflush(fdbg);


	//printf("PID_Update_NewData function start:\n");


	//only for debug
	if(0)
	{
		int length;
		U16 port_id = DeviceId/2;
		U16 channel_id = DeviceId%2;
		length=0;
		int i=0;
		U8 xx[5];
		for(;i<5; i++)
			xx[i]=i;

		void* data=xx;
		int ret = rte_fiber_generic_pkg(port_id, channel_id, data, length);

		if (ret != 1)
		{
			rte_wmb();
			ret = rte_fiber_tx_burst(port_id, channel_id, length);
		}
		else
		{
			//printf("Fatal error for no pkt to send !\n");
			sprintf(str_dbg,"Fatal error for no pkt to send !\n");
			fputs(str_dbg,fdbg);
			return -1;
		}
		return 0;
	}


	struct dev_buff *dev = &gs_dev[DeviceId];
	struct enet_frame *frame;
	struct sim_pkt *tx_sim_pkt;
	int i, err = 0;
	U32 cyc;

	//pthread_spin_lock(&dev->datalock);

	dev->updates++;
	//printf("PID_Update_NewData:dev->updates value:%d\n",dev->updates);
	dev->tx_count++;
	//printf("PID_Update_NewData:dev->tx_count value:%d\n",dev->tx_count);
	//pthread_spin_unlock(&dev->datalock);

	if(DEBUG_MUL_PACKETS)
	{
		dev->ao_data_count=60000;
		int x;
		for(x=0; x<dev->ao_data_count; x++)
		{
			dev->ao_data[x]=x%10;

		}
		dev->do_data_count=60000;
		for(x=0; x<dev->do_data_count; x++)
		{
			dev->do_data[x]=x%2;

		}

	}


	//if (dev->status == 0) return 0;

	//printf("PID_Update_NewData: ao data size:%d\n",dev->ao_data_count);
	int xx=dev->tx_count;
	if(xx%INFO_PRINT_STEP==1 ||DEBUG )
	{
		fprintf(fdbg,"PID_Update_NewData: ao data size:%d\n",dev->ao_data_count);

		//printf("PID_Update_NewData: do data size:%d\n",dev->do_data_count);
		fprintf(fdbg,"PID_Update_NewData: do data size:%d\n",dev->do_data_count);

	}

	if(dev->ao_data_count==0 && dev->do_data_count==0) return 0;

	// 发送DO/AO写,DI/AI读仿真报文
	//frame = (struct enet_frame *)dev->tx_frame.payload;
	//unsigned short* ps=frame->data;
	//unsigned short* ps= (unsigned short *)dev->tx_frame.payload;
	unsigned char* ps= dev->tx_frame.payload;

	if(! ps)
	{
		fprintf(fdbg,"dev->tx_frame.payload is null, return !\n");
		return 0;
	}

	U16* pi=(U16* )ps;


	unsigned int offset=0;//字节偏移

	pi[0]= dev->ao_data_count;
	swapu16(pi);
	offset=sizeof(U16);

	//printf("size of U16 :%d\n",sizeof(U16));
	//printf("size of float :%d\n",sizeof(float));

	//模拟量
	//printf("---------------1  offset value :%d-------------\n",offset);
	if( dev->ao_data_count>0 )
	{
		memcpy(ps+offset,dev->ao_data,sizeof(float)*dev->ao_data_count);

		//printf("---------------2  offset value :%d-------------\n",offset);
		if(DEBUG || dev->tx_count%INFO_PRINT_STEP == 1)
		{

			int xx;
			/*
			int step=1;
			if(dev->ao_data_count>10000)
				step=10000;
			else if(dev->ao_data_count>1000)
				step=1000;
			else if(dev->ao_data_count>100)
				step=100;
			else if(dev->ao_data_count>10)
				step=10;
			 */

			fprintf(fdbg,"Send AO data print start(only print first 100, dev->tx_count=%d------\n",dev->tx_count);
			for(xx=0;xx<dev->ao_data_count ;xx++)
			{
				//printf("PID_Update_NewData: analog data:%d @ offset:value:%d:%f\n",xx,offset,((float*)(ps+offset))[xx]);
				//fprintf(fdbg,"PID_Update_NewData: analog data:%d @ offset:value:%d:%20.15f\n",xx,offset,((float*)(ps+offset))[xx]);
				//fprintf(fdbg,"PID_Update_NewData: dev->ao_data:%d @ offset:value:%d:%20.15f\n",xx,offset,dev->ao_data[xx]);
				fprintf(fdbg,"Send AO data No:%d  value:%20.15f\n",xx+1,dev->ao_data[xx]);

			}
			fprintf(fdbg,"Send AO data No:%d  value:%20.15f\n",dev->ao_data_count,dev->ao_data[dev->ao_data_count-1]);
			fprintf(fdbg,"Send AO data print done----------------------------\n");


			/*
			sprintf(str_dbg,"PID_Update_NewData: print data before send :  \n");
			for (i = 0; i < 2+offset; i++)
				sprintf(str_dbg,"%02x ",*(ps+i));
			sprintf(str_dbg,"\n");
			fputs(str_dbg,fdbg);
			 */

		}
		int x=0;
		for(; x<dev->ao_data_count;x++)
		{
			swapu32(((float *)(ps+offset))+x);
		}
		offset=offset+sizeof(float)*(dev->ao_data_count);

		//printf("---------------3  offset value :%d-------------\n",offset);
	}

	//数字量个数
	pi=(U16 *)(ps+offset);


	//to be removed
	if(0)
	{
		dev->do_data_count=1000;
		int i=0;
		pi[0]=dev->do_data_count;
		for(;i<dev->do_data_count;i++)
		{
			dev->do_data[i]=i%2;
		}
	}
	pi[0]=dev->do_data_count;


	swapu16(pi);
	offset=offset+sizeof(U16);


	//printf("------- size of unsigned short----:%d\n",sizeof(unsigned short));
	//printf("------- size of unsigned char----:%d\n",sizeof(unsigned char));
	if( dev->do_data_count>0 )
	{
		//数字量；
		unsigned char* pd=ps+offset;
		memcpy(pd,dev->do_data,sizeof(unsigned char)*dev->do_data_count);
		offset=offset+sizeof(unsigned char)*dev->do_data_count;

		if(dev->tx_count%INFO_PRINT_STEP==1)
		{
			printf("PID_Update_NewData:di data size:%d\n   ：",dev->do_data_count);
			int x=0;
			for(;x<dev->do_data_count;x++)
				fprintf(fdbg,"PID_Update_NewData:di data[%d]=%d\n",x,dev->do_data[x]);
		}

	}

	//frame->byte_count=offset;

	dev->tx_frame.len=offset;


	if(DEBUG)
	{
		fprintf(fdbg,"PID_Update_NewData: offset:%d\n",offset);

	}

	if(0)
	{
		sprintf(str_dbg,"PID_Update_NewData: print data before send :  \n");
		for (i = 0; i < offset; i++)
			fprintf(fdbg,"%02x ",*(ps+i));
		fprintf(fdbg,"\n");


	}


	yjdnet_write(DeviceId, dev, 0);

	//reset dev data
	dev->do_data_count=0;
	dev->ao_data_count=0;
	dev->tx_frame.len=0;
	// sync_note
	if ((s_note.l_1 == 0) && (s_note.l_2 == 0)) {
		rtclock_gettime(&s_note.l_2);
		//rdtscll(s_note.l_2);
	}
	else {
		s_note.total++;
		s_note.l_1 = s_note.l_2;
		//rdtscll(s_note.l_2);
		rtclock_gettime(&s_note.l_2);
		cyc = s_note.l_2 - s_note.l_1;
		if (cyc < s_note.exceed_n30) {
			s_note.cyc_cnt1++;
		}
		else if ((cyc >= s_note.exceed_n30) && (cyc <= s_note.exceed_n20)) {
			s_note.cyc_cnt2++;
		}
		else if ((cyc >= s_note.exceed_n20) && (cyc <= s_note.exceed_n10)) {
			s_note.cyc_cnt3++;
		}
		else if ((cyc >= s_note.exceed_n10) && (cyc <= s_note.exceed_0)) {
			s_note.cyc_cnt4++;
		}
		else if ((cyc >= s_note.exceed_0) && (cyc <= s_note.exceed_p10)) {
			s_note.cyc_cnt5++;
		}
		else if ((cyc >= s_note.exceed_p10) && (cyc <= s_note.exceed_p20)) {
			s_note.cyc_cnt6++;
		}
		else if ((cyc >= s_note.exceed_p20) && (cyc <= s_note.exceed_p30)) {
			s_note.cyc_cnt7++;
		}
		else {
			s_note.cyc_cnt8++;
		}
		if (cyc > s_note.max_cyc) {
			s_note.max_cyc = cyc;
			s_note.max_cyc_pos = s_note.total;
		}
		if (cyc < s_note.min_cyc) {
			s_note.min_cyc = cyc;
			s_note.min_cyc_pos = s_note.total;
		}
	}

	//printf("PID_Update_NewData function finish successfully:\n");
	fflush(fdbg);
	return 0;
}


static I16 PID_Unit_Cmd(U16 DeviceId, U8 Cmd)
{
	/*
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;
	int i, offset;
	U16 *data;
	U32 sync_cyc_0;	
	U32 sync_cyc_n30;
	U32 sync_cyc_n20;
	U32 sync_cyc_n10;
	U32 sync_cyc_p10;
	U32 sync_cyc_p20;
	U32 sync_cyc_p30;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (Cmd < CMD_START_SIM || Cmd > CMD_STOP_SIM)
		return ErrorCmd;

	dev = &gs_dev[DeviceId];

	if (Cmd == CMD_START_SIM) {
		s_note.l_1 = 0;
		s_note.l_2 = 0;
		// CPU_Freq * SYNC_CYC
		s_note.exceed_0   = 50000;//s_note.cpu_freq * dev->sync_cyc / 1000;
		s_note.exceed_n30 = s_note.exceed_0 * 7  / 10;
		s_note.exceed_n20 = s_note.exceed_0 * 8  / 10;
		s_note.exceed_n10 = s_note.exceed_0 * 9  / 10;
		s_note.exceed_p10 = s_note.exceed_0 * 11 / 10;
		s_note.exceed_p20 = s_note.exceed_0 * 12 / 10;
		s_note.exceed_p30 = s_note.exceed_0 * 13 / 10;
		s_note.max_cyc     = 0;
		s_note.max_cyc_pos = 0;
		s_note.min_cyc     = 0xFFFFFFFF;
		s_note.min_cyc_pos = 0;
		s_note.cyc_cnt1 = 0;
		s_note.cyc_cnt2 = 0;
		s_note.cyc_cnt3 = 0;
		s_note.cyc_cnt4 = 0;
		s_note.cyc_cnt5 = 0;
		s_note.cyc_cnt6 = 0;
		s_note.cyc_cnt7 = 0;
		s_note.cyc_cnt8 = 0;
		s_note.total    = 0;

		max_us_markinput = 0;
		max_us_markinput_count = 0;
		pthread_spin_init(&dev->datalock,1);
//		dev->status = 1;
	}

	// 模拟主卡的功能卡0~9信息配置
	pthread_spin_lock(&dev->lock);
	if (Cmd == CMD_STOP_SIM) {
		for (i = 0; i < MAX_SLOTS; i++) {
			dev->card_mode[i] = 0;
		}
	}
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((MAX_SLOTS + 3) * 2) << 5) + 1;
	pkt->card     = SEL_MASTER;
	pkt->func     = FUNC_TYPE_WRITE;
	pkt->reg_addr = 0x0083;
	pkt->reg_cnt  = MAX_SLOTS;
	for (i = 0; i < MAX_SLOTS; i++) {
		pkt->data[i]  = (U16)dev->card_type[i] + (dev->card_mode[i] << 8);
	}
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((MAX_SLOTS + 4) * 2);

	if (yjdnet_write(DeviceId, dev, 0) != 1) {
		pthread_spin_unlock(&dev->lock);
		return ErrorCmd;
	}

	pthread_spin_unlock(&dev->lock);

	usleep(100000);

	// 组合模拟主卡及功能卡的工作模式配置
	pthread_spin_lock(&dev->lock);
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;

//	if (Cmd == CMD_START_SIM) dev->status = 1;
//	else                      
//	{		
//		dev->status = 0;
//	}
	pkt->card     = SEL_MASTER;								// 主卡mode配置
	pkt->func     = FUNC_TYPE_WRITE;
	pkt->reg_addr = 0x0080;
	pkt->reg_cnt  = 1;
	pkt->data[0]  = (Cmd == CMD_START_SIM) ? 1 : 0;
	offset = 1;
	for (i = 0; i < MAX_SLOTS; i++) {						// 功能卡mode配置
		pkt->data[offset + 0] = (FUNC_TYPE_WRITE << 8) | i;
		pkt->data[offset + 1] = 0x0080;
		pkt->data[offset + 2] = 1;
		pkt->data[offset + 3] = dev->card_mode[i];
		offset += 4;
	}
	pkt->frm_head = (((offset + 3) * 2) << 5) + 1;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((offset + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1) {
		pthread_spin_unlock(&dev->lock);
		return ErrorCmd;
	}
	pthread_spin_unlock(&dev->lock);

	usleep(100000);

	if (Cmd == CMD_START_SIM) dev->status = 1;
	else                      
	{		
		dev->status = 0;
	}

	if (Cmd == CMD_STOP_SIM) {
		sync_cyc_0   = dev->sync_cyc;
		sync_cyc_n30 = sync_cyc_0 * 7 / 10;
		sync_cyc_n20 = sync_cyc_0 * 8 / 10;
		sync_cyc_n10 = sync_cyc_0 * 9 / 10;
		sync_cyc_p10 = sync_cyc_0 * 11 / 10;
		sync_cyc_p20 = sync_cyc_0 * 12 / 10;
		sync_cyc_p30 = sync_cyc_0 * 13 / 10;
///*
		printf("Max Cycle           : %d\n", s_note.max_cyc/1000);
		printf("Max Cycle Position  : %d\n", s_note.max_cyc_pos);
		printf("Min Cycle           : %d\n", s_note.min_cyc/1000);
		printf("Min Cycle Position  : %d\n", s_note.min_cyc_pos);
		printf("Cycle Count <%d   : %d\n", sync_cyc_n30, s_note.cyc_cnt1);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_n30, sync_cyc_n20, s_note.cyc_cnt2);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_n20, sync_cyc_n10, s_note.cyc_cnt3);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_n10, sync_cyc_0, s_note.cyc_cnt4);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_0, sync_cyc_p10, s_note.cyc_cnt5);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_p10, sync_cyc_p20, s_note.cyc_cnt6);
		printf("Cycle Count %d~%d : %d\n", sync_cyc_p20, sync_cyc_p30, s_note.cyc_cnt7);
		printf("Cycle Count >%d   : %d\n", sync_cyc_p30, s_note.cyc_cnt8);
		printf("Total Count         : %d\n", s_note.total);
		printf("s_note.max_cyc      : %d\n", s_note.max_cyc);
		printf("s_note.min_cyc      : %d\n", s_note.min_cyc);
		printf("s_note.exceed_n30   : %d\n", s_note.exceed_n30);
		printf("s_note.exceed_n20   : %d\n", s_note.exceed_n20);
		printf("s_note.exceed_n10   : %d\n", s_note.exceed_n10);
		printf("s_note.exceed_0     : %d\n", s_note.exceed_0);
		printf("s_note.exceed_p10   : %d\n", s_note.exceed_p10);
		printf("s_note.exceed_p20   : %d\n", s_note.exceed_p20);
		printf("s_note.exceed_p30   : %d\n", s_note.exceed_p30);
//	*/
	//	}


	return NoError;
}

static I16 PID_Sem_Init(U16 DeviceId)
{
	struct dev_buff *dev;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	int ret = sem_destroy(&dev->rx_sem);
	if (ret < 0) return ret;
	return sem_init(&dev->rx_sem, 1, 0);
}

// psrt driver part-------------------------------------------------------------------------------

int rt_ioctl(int filp, unsigned int request, unsigned long l)
{
	pid_ioctl_t *p = (pid_ioctl_t *)l;

	switch (request) {
	case PID_REGISTER:	   return PID_Register(p->DeviceId, p->Mode);
	case PID_RELEASE:	   return PID_Release (p->DeviceId);
	//case PID_SYNC_READCONFIG:  return PID_Sync_ReadConfig (p->DeviceId, p->SyncFreq_rd, p->SM_rd);
	//case PID_SYNC_WRITECONFIG: return PID_Sync_WriteConfig(p->DeviceId, p->SyncFreq_wr, p->SM_wr);
	//case PID_SLOT_READINFO:	   return PID_Slot_ReadInfo(p->DeviceId, p->SlotId, p->CardInfo, p->Slp, p->Itcp);
	//case PID_SLOT_READCONFIG:  return PID_Slot_ReadConfig (p->DeviceId, p->SlotId, p->CardConfig_rd);
	//case PID_SLOT_WRITECONFIG: return PID_Slot_WriteConfig(p->DeviceId, p->SlotId, p->CardConfig_wr);
	//case PID_SLOT_READPARA:    return PID_Slot_ReadPara(p->DeviceId, p->SlotId, p->Slp, p->Itcp);
	//case PID_SLOT_WRITEPARA:   return PID_Slot_WritePara(p->DeviceId, p->SlotId, p->Slp, p->Itcp);
	//case PID_DI_GRPCONFIG:	   return PID_DI_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	//case PID_DO_GRPCONFIG:	   return PID_DO_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	//case PID_AI_GRPCONFIG:	   return PID_AI_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	//case PID_AO_GRPCONFIG:	   return PID_AO_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	case PID_DI_GRPREAD:	   return PID_DI_GrpRead  (p->DeviceId, p->GrpId, p->Data_dig,  p->Count);
	case PID_DO_GRPWRITE:	   return PID_DO_GrpWrite (p->DeviceId, p->GrpId, p->Data_dig,  p->Count);
	case PID_AI_GRPREAD:	   return PID_AI_GrpRead  (p->DeviceId, p->GrpId, p->Data_ana,  p->Count);
	case PID_AO_GRPWRITE: 	   return PID_AO_GrpWrite (p->DeviceId, p->GrpId, p->Data_ana,  p->Count);
	case PID_AI_VOLTSCALE:	   return PID_AI_VoltScale(p->Data_ana, p->Slp, p->Itcp, p->Volt, p->Count);
	case PID_AO_VOLTSCALE:	   return PID_AO_VoltScale(p->Volt, p->Slp, p->Itcp, p->Data_ana, p->Count);
	case PID_MARKINPUT_CLEAR:		return PID_MarkInput_Clear(p->DeviceId);
	case PID_MARKINPUT_WAIT:   return PID_MarkInput_Wait(p->DeviceId, p->Timeout);
	case PID_UPDATE_NEWDATA:		return PID_Update_NewData(p->DeviceId);
	//case PID_UNIT_CMD:	   return PID_Unit_Cmd(p->DeviceId, p->Cmd);
	case PID_SEM_INIT:	   return PID_Sem_Init(p->DeviceId);
	}
	return 0;
}

#if 0
int yjdnet_rtvm_open(struct rt_file *filp)
{
	return 0;
}

int yjdnet_rtvm_release(struct rt_file *filp)
{
	return 0;
}

static struct rt_file_operations yjdnet_rtvm_fops = {
		ioctl:	 yjdnet_rtvm_ioctl,
		open:	 yjdnet_rtvm_open,
		release: yjdnet_rtvm_release
};

/*int yjdnet_gpos_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	pid_ioctl_t *p = (pid_ioctl_t *)arg;

	switch (cmd) {
	case PID_REGISTER:	   return PID_Register(p->DeviceId);
	case PID_RELEASE:	   return PID_Release (p->DeviceId);
	case PID_SYNC_READCONFIG:  return PID_Sync_ReadConfig (p->DeviceId, p->SyncFreq_rd, p->SM_rd);
	case PID_SYNC_WRITECONFIG: return PID_Sync_WriteConfig(p->DeviceId, p->SyncFreq_wr, p->SM_wr);
	case PID_SLOT_READINFO:	   return PID_Slot_ReadInfo(p->DeviceId, p->SlotId, p->CardInfo, p->Slp, p->Itcp);
	case PID_SLOT_READCONFIG:  return PID_Slot_ReadConfig (p->DeviceId, p->SlotId, p->CardConfig_rd);
	case PID_SLOT_WRITECONFIG: return PID_Slot_WriteConfig(p->DeviceId, p->SlotId, p->CardConfig_wr);
	case PID_DI_GRPCONFIG:	   return PID_DI_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	case PID_DO_GRPCONFIG:	   return PID_DO_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	case PID_AI_GRPCONFIG:	   return PID_AI_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	case PID_AO_GRPCONFIG:	   return PID_AO_GrpConfig(p->DeviceId, p->GrpId, p->GrpConfig, p->Count);
	case PID_DI_GRPREAD:	   return PID_DI_GrpRead  (p->DeviceId, p->GrpId, p->Data_dig,  p->Count);
	case PID_DO_GRPWRITE:	   return PID_DO_GrpWrite (p->DeviceId, p->GrpId, p->Data_dig,  p->Count);
	case PID_AI_GRPREAD:	   return PID_AI_GrpRead  (p->DeviceId, p->GrpId, p->Data_ana,  p->Count);
	case PID_AO_GRPWRITE: 	   return PID_AO_GrpWrite (p->DeviceId, p->GrpId, p->Data_ana,  p->Count);
	case PID_AI_VOLTSCALE:	   return PID_AI_VoltScale(p->Data_ana, p->Slp, p->Itcp, p->Volt, p->Count);
	case PID_AO_VOLTSCALE:	   return PID_AO_VoltScale(p->Volt, p->Slp, p->Itcp, p->Data_ana, p->Count);
	case PID_MARKINPUT_WAIT:   return PID_MarkInput_Wait(p->DeviceId, p->Timeout);
	case PID_UNIT_CMD:	   return PID_Unit_Cmd(p->DeviceId, p->Cmd);
	}
	return -1;
}

int yjdnet_gpos_open(struct inode *inode, struct file *filp)
{
	printk("gpos_open\n");
	return 0;
}

int yjdnet_gpos_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct rt_gpos_file_operations yjdnet_gpos_fops = {
	ioctl:	 yjdnet_gpos_ioctl,
	open:	 yjdnet_gpos_open,
	release: yjdnet_gpos_release
};
 */
int main(int argc, char *argv[])
{
	int i;//, gpos_minor, rt_minor;
	char *name = "/dev/yjdnet";

	// get cpu freq kHz
	s_note.cpu_freq = get_cpu_freq();

	// register the handler for the RTVM side file
	if (rt_register_dev(name, &yjdnet_rtvm_fops, 0)) {
		printk("Unable to create RTVM device file\n");
		return -1;
	}
	/*
	rt_minor = rt_namei(name);
	rt_decr_dev_usage(rt_minor);

	// register the handler for the GPOS side file
	if ((gpos_minor = rt_gpos_register_dev(name, &yjdnet_gpos_fops, rt_minor)) < 0) {
		printk("Unable to register GPOS device (%d)\n", gpos_minor);
		rt_unregister_dev(name);
		return -1;
	}

	// register the GPOS side file
	if (rt_gpos_mknod(name, GPOS_S_IFCHR, GPOS_MAJOR, gpos_minor)) {
		printk("Could not create GPOS file\n");
		rt_gpos_unregister_dev(name);
		rt_unregister_dev(name);
		return -1;
	} 
	 */
	for (i = 0; i < MAX_DEVICES; i++)
		PID_Register(i, MODE_SEM);

	rt_main_wait();

	for (i = 0; i < MAX_DEVICES; i++)
		PID_Release(i);

	//	rt_gpos_unregister_dev(name);
	rt_unregister_dev(name);
	//	rt_gpos_unlink(name);

	return 0;
}
#endif
