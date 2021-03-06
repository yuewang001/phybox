//#define __RTVM_POLLUTED_APP__
//#define __KERNEL__

#include "yjdnet.h"
#include "yjdioctl.h"
#include <time.h>
#include <errno.h>
#include <pthread.h>
//#include <gpos_bridge/sys/gpos.h>



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

struct sim_pkt {
	U16 head[2];
	U16 reserved;
	U16 data_len;
	U16 stamp[4];
	U16 order;
	U16	frm_head;
	U16	data[512];
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

struct grp_cfg {
	U16	count;
	U8	slot[MAX_SLOTS * MAX_DI_CHANS];
	U8	chan[MAX_SLOTS * MAX_DI_CHANS];
};

struct dev_buff {
	I16 ai_data[MAX_SLOTS][MAX_AI_CHANS];
	I16	ao_data[MAX_SLOTS][MAX_AO_CHANS];
	U16	di_data[MAX_SLOTS][MAX_DI_CHANS];
	U16	do_data[MAX_SLOTS][MAX_DO_CHANS];

	struct grp_cfg	ai_grp_cfg[MAX_GRP_CFGS];
	struct grp_cfg	ao_grp_cfg[MAX_GRP_CFGS];
	struct grp_cfg	di_grp_cfg[MAX_GRP_CFGS];
	struct grp_cfg	do_grp_cfg[MAX_GRP_CFGS];

	U8 card_type[MAX_SLOTS];
	U8 card_mode[MAX_SLOTS];
	
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

static int dpdk_write(U16 DeviceId, struct dev_buff *dev, U16 DesPortId)
{
	// 注意:
	// 光纤网卡的发送帧从ctl_pkt或sim_pkt的order字段开始发送
	// 所以起始地址减去14字节以太网物理包头,再减去16字节光纤网卡包头
	unsigned char *data = dev->tx_frame.payload + (14 + 16);
	int length = dev->tx_frame.len - (14 + 16);
	int i;
	int ret;
	U16 crc;

	U16 port_id = DeviceId/2;
    U16 channel_id = DeviceId%2;

	// 如果光纤网卡底层不自带CRC,要求程序添加CRC
	// 即使添加吧CRC,下面IO卡还是检查出来CRC错误,这是因为实际AURORA帧添加的包头未能列入计算
	// IO卡要求v0.0.007以上版本,并且GPIO_SWITCH[5]的OFF位置（即忽略CRC正误）
	// 数据末尾添加16位CRC,0x1021多项式,与aurora v8.3的CRC计算要求一样
	// 把CRC数据填充到data末尾,并且length+2
	crc = calc_aurora_crc(data, length);
	data[length++] = (U8)crc;
	data[length++] = (U8)(crc >> 8);
	
#if 0
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

	return 1;
}

static struct rte_mbuf *dpdk_read(int DeviceId, struct dev_buff *dev)
{
	int ret = 0;
	int i;	
	struct rte_mbuf *pkt;
	struct rte_mbuf *pkts[MAX_PKT_BURST];
	
	U16 port_id = DeviceId/2;
    U16 channel_id = DeviceId%2;

	while(1)
	{
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

static I16 yjdnet_write(U16 DeviceId, struct dev_buff *dev,U16 DesPortId)
{
	int i, err;

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

struct _global_time currTime,startTime;
U16 global_us;


static void *__rx_event(__attribute__((unused)) void *arg)
{
	U16 DeviceId, dev_index;
	struct dev_buff *dev;
	struct rte_mbuf *pkt_rx;
	set_thread_affinity(pthread_rx_event_core);

	U16 tmp111[9];
	
	while (1)
	{
		for (dev_index = 0; dev_index < MAX_DEVICES; dev_index++)
		{			
			if (gs_dev_no[dev_index] == -1) continue;

			struct enet_frame *frame;
			struct ctl_pkt *pkt;
			struct sim_pkt *rx_sim_pkt;
			int i, err, offset = 0;
			
			DeviceId = gs_dev_no[dev_index];
			dev = &gs_dev[DeviceId];

			int rx_frm_len;
			U16 *data;
			U8  card, func;
			U16 reg_addr, reg_cnt;
			U32 cyc;


			pkt_rx = dpdk_read(DeviceId, dev);	
			
			if (pkt_rx	== NULL) continue;
			
			// rte_mbuf结构中的pkt下的data指向ctl_pkt或sim_pkt的stamp字段
			pkt = (struct ctl_pkt*)(pkt_rx->pkt.data - 8);
			dev->rx_frame.len = pkt_rx->pkt.data_len + (14 + 8);
			
			// 如果光纤网卡底层不自带CRC,则需要去掉发送时加的2字节CRC数据
			dev->rx_frame.len -= 2;

			if (dev->status == 0)
			{
#if 0
				// 打印控制帧,包括时间戳,数据,CRC
				printf("<-- %3.3d : ", pkt_rx->pkt.data_len);
				for (i = 0; i < pkt_rx->pkt.data_len; i++)
					printf("%2.2x ", *(U8 *)(pkt_rx->pkt.data + i));
				printf("\n");
#endif
				memcpy(&dev->ctl_pkt, pkt, dev->rx_frame.len - ENET_FRAME_HEAD_LEN);
			} 
			else 
			{		
/*				// sync_note
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
*/
			
				rx_sim_pkt = (struct sim_pkt *)pkt;
				rx_frm_len = rx_sim_pkt->frm_head >> 5;
				data = rx_sim_pkt->data;
				
				// 保存DI/AI仿真数据
				while (rx_frm_len > 3) 
				{
					card     = (U8)data[0];
					func     = (U8)(data[0] >> 8);
					reg_addr = data[1];
					reg_cnt  = data[2];
					
					if ((card < MAX_SLOTS) && (func == FUNC_TYPE_READ) && (reg_addr == 0x0180) && (reg_cnt <= MAX_DI_CHANS)) 
					{
						if (dev->card_type[card] == CARD_INFO_DI) 
						{
							memcpy(&dev->di_data[card][0], &data[3], reg_cnt*2);
						} 
						else if (dev->card_type[card] == CARD_INFO_AI) 
						{
							memcpy(&dev->ai_data[card][0], &data[3], reg_cnt*2);
/*
							tmp111[8] = tmp111[7];
							tmp111[7] = tmp111[6];
							tmp111[6] = tmp111[5];
							tmp111[5] = tmp111[4];
							tmp111[4] = tmp111[3];
							tmp111[3] = tmp111[2];
							tmp111[2] = tmp111[1];
							tmp111[1] = tmp111[0];
							tmp111[0] = dev->ai_data[card][0];
							if (tmp111[0] == tmp111[8])
							{
								printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x\n", tmp111[8], tmp111[7], tmp111[6], tmp111[5], tmp111[4], tmp111[3], tmp111[2], tmp111[1], tmp111[0]);
							}
*/							
						}
						dev->rx_count++;

					}
					data += 3 + reg_cnt;
					rx_frm_len -= (3 + reg_cnt) * 2;
				}
			}
			rte_pktmbuf_free(pkt_rx);
		}
	}
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


unsigned long long t3,t4;

static void *__gpsTimer_event(__attribute__((unused)) void *arg)
{
	u64 t_H=0,t_L=0;
	u64 i=0;
	U8 TAG=0;
	unsigned long long  curr_time,curr_time1;

	set_thread_affinity(pthread_gpsTimer_core);

	while(1)
	{
		t_H = rte_fiber_get_register(0,0,17);
		t_L = rte_fiber_get_register(0,0,18);
		
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

	struct sim_pkt *tx_sim_pkt;

	int DeviceId = DeviceId_in;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	dev->mode  = Mode;
	dev->tx_count = 0;//add by zhangy
	dev->rx_count = 0;//add by zhangy

	if (dev->opens) {
		dev->opens++;
		return NoError;
	}

	dev->id = DeviceId;

	if (!(dev->tx_buff = malloc(sizeof(struct enet_frame)))) {
		yjdnet_debug("rt_alloc_mem(%d) is falild\n", DeviceId);
		goto err_alloc_mem;
	}

	sem_init(&dev->rx_sem, 1, 0);
	pthread_spin_init(&dev->lock, 1);

	dev->tx_frame.payload = dev->tx_buff;
	frame = (struct enet_frame *)dev->tx_frame.payload;
	memset(frame, 0x00, sizeof(struct enet_frame));
	memset(frame->dst, 0xff, 6);
	frame->type = 0x0424;

	dev->opens++;

	tx_sim_pkt = (struct sim_pkt *)frame->data;
	tx_sim_pkt->order =0;

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


static I16 PID_Sync_ReadConfig(U16 DeviceId, U32 *SyncFreq, U16 *SM)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (10 << 5) + 1;
	pkt->card     = SEL_MASTER;
	pkt->func     = FUNC_TYPE_READ;
	pkt->reg_addr = 0x0081;
	pkt->reg_cnt  = 2;
	pkt->data[0]  = 0x0000;
	pkt->data[1]  = 0x0000;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + 6*2;
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorReadSyncConfig;

	usleep(100000);

	pkt = &dev->ctl_pkt;
	if (pkt->card     != SEL_MASTER		||
	    pkt->func     != FUNC_TYPE_READ	||
	    pkt->reg_addr != 0x0081			||
	    pkt->reg_cnt  != 2)
		return ErrorReadSyncConfig;

	if (SyncFreq) *SyncFreq = *(U32 *)pkt->data;
	if (SM)	      *SM       = 0;
	return NoError;
}


static I16 PID_Sync_WriteConfig(U16 DeviceId, U32 SyncFreq, U16 SM)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (10 << 5) + 1;
	pkt->card     = SEL_MASTER;
	pkt->func     = FUNC_TYPE_WRITE;
	pkt->reg_addr = 0x0081;
	pkt->reg_cnt  = 2;
	*(U32 *)pkt->data = SyncFreq;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + 6*2;
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorWriteSyncConfig;

	usleep(100000);

	//pkt = &dev->ctl_pkt;
	//if (pkt->card     != SEL_MASTER			||
	//    pkt->func     != FUNC_TYPE_WRITE	||
	//    pkt->reg_addr != 0x0081				||
	//    pkt->reg_cnt  != 2)
	//	return ErrorWriteSyncConfig;
	
	dev->sync_cyc = SyncFreq;
	
	return NoError;
}


static I16 PID_Slot_ReadInfo(U16 DeviceId, U8 SlotId, U16 *CardInfo, F64 *Slp, F64 *Itcp)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;
	int i;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (SlotId >= MAX_SLOTS)
		return ErrorReadSlotInfo;

	dev = &gs_dev[DeviceId];
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((7 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_READ;
	pkt->reg_addr = 0x0000;
	pkt->reg_cnt  = 7;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((7 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorReadSlotInfo;

	usleep(100000);

	pkt = &dev->ctl_pkt;
	if (pkt->card     != SlotId			||
	    pkt->func     != FUNC_TYPE_READ	||
	    pkt->reg_addr != 0x0000			||
	    pkt->reg_cnt  != 7)
		return ErrorReadSlotInfo;
	if (CardInfo) {
		CardInfo[0] = pkt->data[0];
		CardInfo[1] = pkt->data[1];
		CardInfo[2] = pkt->data[2];
		CardInfo[3] = pkt->data[3];
		CardInfo[4] = pkt->data[4];
		CardInfo[5] = pkt->data[5];
		CardInfo[6] = pkt->data[6];
		if (CardInfo[1] == CARD_INFO_AO) {
			for (i = 0; i < MAX_AO_CHANS; i++) {
				dev->ao_data[SlotId][i] = 0x8000;
			}
		}
		dev->card_type[SlotId] = (U8)CardInfo[1];
	}
	
	// read card parameter
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((96 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_READ;
	pkt->reg_addr = 0x0200;
	pkt->reg_cnt  = 96;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((96 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorReadSlotInfo;

	usleep(100000);

	pkt = &dev->ctl_pkt;
	if (pkt->card     != SlotId			||
	    pkt->func     != FUNC_TYPE_READ	||
	    pkt->reg_addr != 0x0200			||
	    pkt->reg_cnt  != 96)
		return ErrorReadSlotInfo;
	
	for (i = 0; i < MAX_AI_CHANS; i++) {
		if (Slp)  Slp[i]  = *(F32 *)&pkt->data[i * 4];
		if (Itcp) Itcp[i] = *(F32 *)&pkt->data[i * 4 + 2];
	}
	
	return NoError;
}

static I16 PID_Slot_ReadConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (SlotId >= MAX_SLOTS)
		return ErrorReadSlotConfig;

	dev = &gs_dev[DeviceId];
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((3 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_READ;
	pkt->reg_addr = 0x0080;
	pkt->reg_cnt  = 3;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((3 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorReadSlotConfig;

	usleep(100000);
	
	pkt = &dev->ctl_pkt;
	if (pkt->card     != SlotId			||
	    pkt->func     != FUNC_TYPE_READ	||
	    pkt->reg_addr != 0x0080			||
	    pkt->reg_cnt  != 3)
		return ErrorReadSlotConfig;

	if (CardConfig) {
		CardConfig[0] = pkt->data[0];
		CardConfig[1] = pkt->data[1];
		CardConfig[2] = pkt->data[2];
	}
	dev->card_mode[SlotId] = (U8)pkt->data[0];
	return NoError;
}


static I16 PID_Slot_WriteConfig(U16 DeviceId, U8 SlotId, U16 *CardConfig)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;
	int i;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (SlotId >= MAX_SLOTS)
		return ErrorWriteSlotConfig;
	
	dev = &gs_dev[DeviceId];
	
	// config card
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((3 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_WRITE;
	pkt->reg_addr = 0x0080;
	pkt->reg_cnt  = 3;
	pkt->data[0]  = 0;//CardConfig[0];
	pkt->data[1]  = CardConfig[1];
	pkt->data[2]  = CardConfig[2];
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((3 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1)
		return ErrorWriteSlotConfig;

	usleep(100000);

	dev->card_mode[SlotId] = (U8)CardConfig[0];

	return NoError;
}


static I16 PID_Slot_ReadPara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;
	int i;
	U8	status_bak;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (SlotId >= MAX_SLOTS)
		return ErrorReadSlotPara;

	dev = &gs_dev[DeviceId];
	
	// 仿真停止
	pthread_spin_lock(&dev->lock);
	status_bak = dev->status;
	dev->status = 0;
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
	pthread_spin_unlock(&dev->lock);
	if (status_bak) {
		pthread_spin_lock(&dev->lock);
		memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
		frame = (struct enet_frame *)dev->tx_frame.payload;
		pkt = (struct ctl_pkt *)frame->data;
		pkt->frm_head = (((1 + 3) * 2) << 5) + 1;
		pkt->card     = SEL_MASTER;
		pkt->func     = FUNC_TYPE_WRITE;
		pkt->reg_addr = 0x0080;
		pkt->reg_cnt  = 1;
		pkt->data[0]  = 0;
		dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((1 + 4) * 2);
		if (yjdnet_write(DeviceId, dev, 0) != 1) {
			dev->status = status_bak;
			pthread_spin_unlock(&dev->lock);
			return ErrorCmd;
		}
		pthread_spin_unlock(&dev->lock);

		usleep(100000);
	}
	
	// 读参数
	pthread_spin_lock(&dev->lock);
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((96 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_READ;
	pkt->reg_addr = 0x0200;
	pkt->reg_cnt  = 96;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((96 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1) {
		dev->status = status_bak;
		pthread_spin_unlock(&dev->lock);
		return ErrorReadSlotPara;
	}
	pthread_spin_unlock(&dev->lock);

	usleep(100000);

	pthread_spin_lock(&dev->lock);
	pkt = &dev->ctl_pkt;
	if (pkt->card     != SlotId			||
	    pkt->func     != FUNC_TYPE_READ	||
	    pkt->reg_addr != 0x0200			||
	    pkt->reg_cnt  != 96)
	{
		dev->status = status_bak;
		pthread_spin_unlock(&dev->lock);
		return ErrorReadSlotPara;
	}
	
	for (i = 0; i < MAX_AI_CHANS; i++) {
		if (Slp)  Slp[i]  = *(F32 *)&pkt->data[i * 4];
		if (Itcp) Itcp[i] = *(F32 *)&pkt->data[i * 4 + 2];
	}
	pthread_spin_unlock(&dev->lock);
	
	// 仿真恢复
	if (status_bak) {
		if (PID_Unit_Cmd(DeviceId, CMD_START_SIM) == NoError) {
			pthread_spin_lock(&dev->lock);
			dev->status = status_bak;
			pthread_spin_unlock(&dev->lock);
		}
		else {
			pthread_spin_lock(&dev->lock);
			dev->status = 0;
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
			pthread_spin_unlock(&dev->lock);
		}
			
	}
	
	return NoError;
}


static I16 PID_Slot_WritePara(U16 DeviceId, U8 SlotId, F64 *Slp, F64 *Itcp)
{
	struct dev_buff *dev;
	struct enet_frame *frame;
	struct ctl_pkt	*pkt;
	int i;
	U8	status_bak;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (SlotId >= MAX_SLOTS)
		return ErrorWriteSlotPara;

	dev = &gs_dev[DeviceId];
	
	// 仿真停止
	pthread_spin_lock(&dev->lock);
	status_bak = dev->status;
	dev->status = 0;
	pthread_spin_unlock(&dev->lock);
	if (status_bak) {
		pthread_spin_lock(&dev->lock);
		memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
		frame = (struct enet_frame *)dev->tx_frame.payload;
		pkt = (struct ctl_pkt *)frame->data;
		pkt->frm_head = (((1 + 3) * 2) << 5) + 1;
		pkt->card     = SEL_MASTER;
		pkt->func     = FUNC_TYPE_WRITE;
		pkt->reg_addr = 0x0080;
		pkt->reg_cnt  = 1;
		pkt->data[0]  = 0;
		dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((1 + 4) * 2);
		if (yjdnet_write(DeviceId, dev, 0) != 1) {
			dev->status = status_bak;
			pthread_spin_unlock(&dev->lock);
			return ErrorCmd;
		}
		pthread_spin_unlock(&dev->lock);

		usleep(100000);
	}
	
	// 写参数
	pthread_spin_lock(&dev->lock);
	memset(&dev->ctl_pkt, 0, sizeof(struct ctl_pkt));
	frame = (struct enet_frame *)dev->tx_frame.payload;
	pkt = (struct ctl_pkt *)frame->data;
	pkt->frm_head = (((96 + 3) * 2) << 5) + 1;
	pkt->card     = SlotId;
	pkt->func     = FUNC_TYPE_WRITE;
	pkt->reg_addr = 0x0200;
	pkt->reg_cnt  = 96;
	for (i = 0; i < MAX_AI_CHANS; i++) {
		if (Slp)  *(F32 *)&pkt->data[i * 4]     = Slp[i];
		if (Itcp) *(F32 *)&pkt->data[i * 4 + 2] = Itcp[i];
	}
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + ((96 + 4) * 2);
	if (yjdnet_write(DeviceId, dev, 0) != 1) {
		dev->status = status_bak;
		pthread_spin_unlock(&dev->lock);
		return ErrorWriteSlotPara;
	}
	pthread_spin_unlock(&dev->lock);

	usleep(100000);

	// 仿真恢复
	if (status_bak) {
		if (PID_Unit_Cmd(DeviceId, CMD_START_SIM) == NoError) {
			pthread_spin_lock(&dev->lock);
			dev->status = status_bak;
			pthread_spin_unlock(&dev->lock);
		}
		else {
			pthread_spin_lock(&dev->lock);
			dev->status = 0;
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
			pthread_spin_unlock(&dev->lock);
		}
		
	}
	
	return NoError;
}


static I16 PID_DI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	struct dev_buff *dev;
	struct grp_cfg *cfg, *tmp;
	U16 i, j, k;
	U8 slot, chan;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorDIGrpSlot;

	if (Count >= MAX_SLOTS * MAX_DI_CHANS)
		return ErrorDIGrpSlot;

	dev = &gs_dev[DeviceId];
	cfg = &dev->di_grp_cfg[GrpId];
	for (i = 0; i < Count; i++) {
		slot = (U8)(GrpConfig[i] >> 8);
		chan = (U8) GrpConfig[i];
		if (slot >= MAX_SLOTS || chan >= MAX_DI_CHANS)
			return ErrorDIGrpSlot;
		for (j = 0; j < MAX_GRP_CFGS; j++) {
			if (j == GrpId) continue;
			tmp = &dev->di_grp_cfg[j];
			for (k = 0; k < tmp->count; k++)
				if (slot == tmp->slot[k] && chan == tmp->chan[k])
					return ErrorDIGrpConflict;
		}
		cfg->slot[i] = slot;
		cfg->chan[i] = chan;
	}
	cfg->count = Count;

	return NoError;
}


static I16 PID_DO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	struct dev_buff *dev;
	struct grp_cfg *cfg, *tmp;
	U16 i, j, k;
	U8 slot, chan;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorDOGrpSlot;

	if (Count >= MAX_SLOTS * MAX_DO_CHANS)
		return ErrorDOGrpSlot;

	dev = &gs_dev[DeviceId];
	cfg = &dev->do_grp_cfg[GrpId];
	for (i = 0; i < Count; i++) {
		slot = (U8)(GrpConfig[i] >> 8);
		chan = (U8) GrpConfig[i];
		if (slot >= MAX_SLOTS || chan >= MAX_DO_CHANS)
			return ErrorDOGrpSlot;
		for (j = 0; j < MAX_GRP_CFGS; j++) {
			if (j == GrpId) continue;
			tmp = &dev->do_grp_cfg[j];
			for (k = 0; k < tmp->count; k++)
				if (slot == tmp->slot[k] && chan == tmp->chan[k])
					return ErrorDOGrpConflict;
		}
		cfg->slot[i] = slot;
		cfg->chan[i] = chan;
	}
	cfg->count = Count;

	return NoError;
}


static I16 PID_AI_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	struct dev_buff *dev;
	struct grp_cfg *cfg, *tmp;
	U16 i, j, k;
	U8 slot, chan;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorAIGrpSlot;

	if (Count >= MAX_SLOTS * MAX_AI_CHANS)
		return ErrorAIGrpSlot;

	dev = &gs_dev[DeviceId];
	cfg = &dev->ai_grp_cfg[GrpId];
	for (i = 0; i < Count; i++) {
		slot = (U8)(GrpConfig[i] >> 8);
		chan = (U8) GrpConfig[i];
		if (slot >= MAX_SLOTS || chan >= MAX_AI_CHANS)
			return ErrorAIGrpSlot;
		for (j = 0; j < MAX_GRP_CFGS; j++) {
			if (j == GrpId) continue;
			tmp = &dev->ai_grp_cfg[j];
			for (k = 0; k < tmp->count; k++)
				if (slot == tmp->slot[k] && chan == tmp->chan[k])
					return ErrorAIGrpConflict;
		}
		cfg->slot[i] = slot;
		cfg->chan[i] = chan;
	}
	cfg->count = Count;
	
	return NoError;
}


static I16 PID_AO_GrpConfig(U16 DeviceId, U16 GrpId, U16 *GrpConfig, U16 Count)
{
	struct dev_buff *dev;
	struct grp_cfg *cfg, *tmp;
	U16 i, j, k;
	U8 slot, chan;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorAOGrpSlot;

	if (Count >= MAX_SLOTS * MAX_AO_CHANS)
		return ErrorAOGrpSlot;

	dev = &gs_dev[DeviceId];
	cfg = &dev->ao_grp_cfg[GrpId];
	for (i = 0; i < Count; i++) {
		slot = (U8)(GrpConfig[i] >> 8);
		chan = (U8) GrpConfig[i];
		if (slot >= MAX_SLOTS || chan >= MAX_AO_CHANS)
			return ErrorAOGrpSlot;
		for (j = 0; j < MAX_GRP_CFGS; j++) {
			if (j == GrpId) continue;
			tmp = &dev->ao_grp_cfg[j];
			for (k = 0; k < tmp->count; k++)
				if (slot == tmp->slot[k] && chan == tmp->chan[k])
					return ErrorAOGrpConflict;
		}
		cfg->slot[i] = slot;
		cfg->chan[i] = chan;
	}
	cfg->count = Count;

	return NoError;
}


static I16 PID_DI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;
	struct grp_cfg	*cfg;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorDIGrpRead;

	dev = &gs_dev[DeviceId];
	cfg = &dev->di_grp_cfg[GrpId];

	if (Count != cfg->count)
		return ErrorDIGrpRead;

	for (i = 0; i < Count; i++)
		Data[i] = dev->di_data[ cfg->slot[i] ][ cfg->chan[i] ];

	return NoError;
}

static I16 PID_DO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;
	struct grp_cfg	*cfg;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorDOGrpWrite;

	dev = &gs_dev[DeviceId];
	cfg = &dev->do_grp_cfg[GrpId];

	if (Count != cfg->count)
		return ErrorDOGrpWrite;

	for (i = 0; i < Count; i++)
		dev->do_data[ cfg->slot[i] ][ cfg->chan[i] ] = Data[i];
	return NoError;
}

static I16 PID_AI_GrpRead(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;
	struct grp_cfg	*cfg;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorAIGrpRead;

	dev = &gs_dev[DeviceId];
	cfg = &dev->ai_grp_cfg[GrpId];

	if (Count != cfg->count)
		return ErrorAIGrpRead;

	for (i = 0; i < Count; i++)
		Data[i] = dev->ai_data[ cfg->slot[i] ][ cfg->chan[i] ];
	
	return NoError;
}

static I16 PID_AO_GrpWrite(U16 DeviceId, U16 GrpId, U16 *Data, U16 Count)
{
	int i;
	struct dev_buff *dev;
	struct grp_cfg	*cfg;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	if (GrpId >= MAX_GRP_CFGS)
		return ErrorAOGrpWrite;

	dev = &gs_dev[DeviceId];
	cfg = &dev->ao_grp_cfg[GrpId];

	if (Count != cfg->count)
		return ErrorAOGrpWrite;

	for (i = 0; i < Count; i++)
		dev->ao_data[ cfg->slot[i] ][ cfg->chan[i] ] = Data[i];

	return NoError;
}

static I16 PID_AI_VoltScale(U16 *Data, F64 *Slp, F64 *Itcp, F64 *Volt, U16 Count)
{
	int i;

	if (!Data || !Slp || !Itcp || !Volt)
		return ErrorAIVoltScale;

	for (i = 0; i < Count; i++)
		Volt[i] = (Data[i] - Itcp[i]) * Slp[i];

	return NoError;
}

static I16 PID_AO_VoltScale(F64 *Volt, F64 *Slp, F64 *Itcp, U16 *Data, U16 Count)
{
	int i, tmp;

	if (!Data || !Slp || !Itcp || !Volt)
		return ErrorAOVoltScale;

	for (i = 0; i < Count; i++) {
		tmp = (Volt[i] - Itcp[i]) * Slp[i];
		if (tmp > 65535) tmp = 65535;
		if (tmp <     0) tmp = 0;
		Data[i] = tmp;
	}
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
	struct dev_buff *dev;
	I16 tmp;

	if (DeviceId >= MAX_DEVICES)
		return ErrorUnknownMacAddr;

	dev = &gs_dev[DeviceId];
	pthread_spin_lock(&dev->datalock);
	tmp = dev->updates;
	if (tmp) dev->updates = 0;
	pthread_spin_unlock(&dev->datalock);
//	if(tmp>0)
	//	dev->rx_count++;
	if(tmp>1)
		printf("NET%d updates=%d times=%d \n",DeviceId,tmp,dev->tx_count);
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
	struct dev_buff *dev = &gs_dev[DeviceId];
	struct enet_frame *frame;
	struct sim_pkt *tx_sim_pkt;
	int i, err, offset = 0;
	U32 cyc;

	dev->updates++;
	dev->tx_count++;

	if (dev->status == 0) return 0;
	
	// 发送DO/AO写,DI/AI读仿真报文
	frame = (struct enet_frame *)dev->tx_frame.payload;
	tx_sim_pkt = (struct sim_pkt *)frame->data;
	tx_sim_pkt->order ++;

	offset = 0;
	for (i = 0; i < MAX_SLOTS; i++) 
	{
		if (dev->card_mode[i] != 0) 
		{
			if (dev->card_type[i] == CARD_INFO_DO) 
			{
				tx_sim_pkt->data[offset++] = 0x0100 + i;
				tx_sim_pkt->data[offset++] = 0x0180;
				tx_sim_pkt->data[offset++] = MAX_DO_CHANS;
				memcpy(&tx_sim_pkt->data[offset], &dev->do_data[i][0], MAX_DO_CHANS*2);
				offset += MAX_DO_CHANS;
			} 
			else if (dev->card_type[i] == CARD_INFO_AO) 
			{
				tx_sim_pkt->data[offset++] = 0x0100 + i;
				tx_sim_pkt->data[offset++] = 0x0180;
				tx_sim_pkt->data[offset++] = MAX_AO_CHANS;
				memcpy(&tx_sim_pkt->data[offset], &dev->ao_data[i][0], MAX_AO_CHANS*2);
				offset += MAX_AO_CHANS;
			}
			else if (dev->card_type[i] == CARD_INFO_DI) 
			{
				tx_sim_pkt->data[offset++] = 0x0000 + i;
				tx_sim_pkt->data[offset++] = 0x0180;
				tx_sim_pkt->data[offset++] = MAX_DI_CHANS;
				memcpy(&tx_sim_pkt->data[offset], &dev->di_data[i][0], MAX_DI_CHANS*2);
				offset += MAX_DI_CHANS;
			}
			else if (dev->card_type[i] == CARD_INFO_AI) 
			{
				tx_sim_pkt->data[offset++] = 0x0000 + i;
				tx_sim_pkt->data[offset++] = 0x0180;
				tx_sim_pkt->data[offset++] = MAX_AI_CHANS;
				memcpy(&tx_sim_pkt->data[offset], &dev->ai_data[i][0], MAX_AI_CHANS*2);
				offset += MAX_AI_CHANS;
			}
		}
	}
	tx_sim_pkt->frm_head = ((offset * 2) << 5) + 1;
	dev->tx_frame.len = ENET_FRAME_HEAD_LEN + 18 + 2 + (offset * 2);

		
	yjdnet_write(DeviceId, dev, 0);
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
	
	return 0;
}


static I16 PID_Unit_Cmd(U16 DeviceId, U8 Cmd)
{
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
	}

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
	case PID_SYNC_READCONFIG:  return PID_Sync_ReadConfig (p->DeviceId, p->SyncFreq_rd, p->SM_rd);
	case PID_SYNC_WRITECONFIG: return PID_Sync_WriteConfig(p->DeviceId, p->SyncFreq_wr, p->SM_wr);
	case PID_SLOT_READINFO:	   return PID_Slot_ReadInfo(p->DeviceId, p->SlotId, p->CardInfo, p->Slp, p->Itcp);
	case PID_SLOT_READCONFIG:  return PID_Slot_ReadConfig (p->DeviceId, p->SlotId, p->CardConfig_rd);
	case PID_SLOT_WRITECONFIG: return PID_Slot_WriteConfig(p->DeviceId, p->SlotId, p->CardConfig_wr);
	case PID_SLOT_READPARA:    return PID_Slot_ReadPara(p->DeviceId, p->SlotId, p->Slp, p->Itcp);
	case PID_SLOT_WRITEPARA:   return PID_Slot_WritePara(p->DeviceId, p->SlotId, p->Slp, p->Itcp);
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
	case PID_MARKINPUT_CLEAR:		return PID_MarkInput_Clear(p->DeviceId);
	case PID_MARKINPUT_WAIT:   return PID_MarkInput_Wait(p->DeviceId, p->Timeout);
	case PID_UPDATE_NEWDATA:		return PID_Update_NewData(p->DeviceId);
	case PID_UNIT_CMD:	   return PID_Unit_Cmd(p->DeviceId, p->Cmd);
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
